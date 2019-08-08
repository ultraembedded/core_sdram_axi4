#include "tb_axi4_driver.h"
#include <queue>

#define BURSTABLE(addr, length, burst_size)  (!(addr & (burst_size-1)) && length >= burst_size)

typedef struct axi_resp_s
{
    uint32_t addr;
    uint32_t size;
    uint32_t id;
    uint32_t last;
} axi_resp_t;

//-----------------------------------------------------------------
// write_internal: Write a block to a target
//-----------------------------------------------------------------
void tb_axi4_driver::write_internal(uint32_t addr, uint8_t *data, int length, uint8_t initial_mask)
{
    std::queue <axi4_master> req_q;
    std::queue <axi4_master> resp_q;

    sc_assert(initial_mask == 0xF || length == 4);

    // Build request queue
    while (length > 0)
    {
        int chunk = 1;

        if (BURSTABLE(addr, length, 32))
            chunk = 32;
        else if (BURSTABLE(addr, length, 16))
            chunk = 16;
        else if (BURSTABLE(addr, length, 8))
            chunk = 8;
        else if (BURSTABLE(addr, length, 4) && length > 4)
            chunk = 4;
        else
            chunk = 1;

        if (chunk == 1 || !m_enable_bursts)
        {
            axi4_master req;
            sc_uint <AXI4_ID_W> id = get_rand_id();

            uint32_t addr_offset = addr & 3;
            int size = (4 - addr_offset);
            if (size > length)
                size = length;

            sc_uint <32> word_data = 0;
            sc_uint <4>  word_mask = 0;

            for (int x=0;x<size;x++)
            {
                word_data.range(((addr_offset + x)*8)+7, ((addr_offset + x)*8)) = *data++;
                word_mask[addr_offset + x] = (initial_mask >> x) & 1;
            }

            req.AWVALID = true;
            req.AWADDR  = addr & ~3;
            req.AWID    = id;
            req.AWLEN   = 1 - 1;
            req.WVALID  = true;
            req.WDATA   = word_data;
            req.WSTRB   = word_mask;
            req.WLAST   = true;
            req.BREADY  = true;

            req_q.push(req);
            resp_q.push(req);

            addr += size;
            length -= size;
        }
        else
        {
            axi4_master req;
            sc_uint <AXI4_ID_W> id = get_rand_id();

            uint32_t word_data = 0;
            for (int x=0;x<4;x++)
                word_data |= (((uint32_t)*data++) << (8 *x));

            req.AWVALID = true;
            req.AWADDR  = addr;
            req.AWID    = id;
            req.AWBURST = AXI4_BURST_INCR;
            req.AWLEN   = (chunk / 4) - 1;
            req.WVALID  = true;
            req.WDATA   = word_data;
            req.WSTRB   = 0xF;
            req.WLAST   = (chunk == 4);
            req.BREADY  = true;

            req_q.push(req);

            if (req.WLAST)
                resp_q.push(req);

            req.init();

            // Additional data
            for (int i=1;i<(chunk / 4);i++)
            {
                req.init();

                word_data = 0;
                for (int x=0;x<4;x++)
                    word_data |= (((uint32_t)*data++) << (8 *x));

                req.WVALID  = true;
                req.WDATA   = word_data;
                req.WSTRB   = 0xF;
                req.AWID    = id;
                req.WLAST   = (i == ((chunk / 4)-1));

                req_q.push(req);

                if (req.WLAST)
                    resp_q.push(req);
            }

            addr += chunk;
            length -= chunk;
        }
    }

    bool split_addr_data = false;

    // Issue requests, wait for responses
    while (req_q.size() > 0 || resp_q.size() > 0)
    {
        axi4_master axi_o = axi_out.read();
        axi4_slave  axi_i = axi_in.read();

        // Write response
        if (axi_i.BVALID && axi_o.BREADY)
        {
            sc_assert(resp_q.size() > 0);

            axi4_master resp = resp_q.front();
            resp_q.pop();

            sc_assert(axi_i.BID == resp.AWID);
            sc_assert(axi_i.BRESP == AXI4_RESP_OKAY);
            sc_assert(m_resp_pending > 0);
            m_resp_pending -= 1;
        }

        // Write command issued
        if (axi_o.AWVALID && axi_i.AWREADY)
        {
            m_resp_pending+= 1;
            axi_o.AWVALID = false;
        }

        // Write data issued
        if (axi_o.WVALID && axi_i.WREADY)
            axi_o.WVALID = false;

        // Delayed data...
        if (split_addr_data)
        {
            bool address_accepted = !axi_o.AWVALID;
            if (!delay_cycle())
            {
                axi_o = req_q.front();

                if (address_accepted)
                {
                    axi_o.AWVALID = false;
                    axi_o.AWID    = 0;
                    axi_o.AWADDR  = 0;
                    axi_o.AWLEN   = 0;
                }

                req_q.pop();

                split_addr_data = false;
            }
        }
        // Issue new address, data cycle?
        else if (!axi_o.AWVALID && !axi_o.WVALID && req_q.size() > 0 && !delay_cycle())
        {
            axi_o = req_q.front();

            // Delay first tick of data randomly
            if (axi_o.AWVALID && delay_cycle())
            {
                split_addr_data = true;
                axi_o.WVALID = false;
                axi_o.WDATA  = 0;
                axi_o.WSTRB  = 0;
                axi_o.WLAST  = 0;
            }
            else
                req_q.pop();
        }

        axi_o.BREADY = !delay_cycle();
        axi_out.write(axi_o);

        wait();
    }
}
//-----------------------------------------------------------------
// write: Write a block to a target
//-----------------------------------------------------------------
void tb_axi4_driver::write(uint32_t addr, uint8_t *data, int length)
{
    write_internal(addr, data, length, 0xF);
}
//-----------------------------------------------------------------
// read: Read a block to a target
//-----------------------------------------------------------------
void tb_axi4_driver::read(uint32_t addr, uint8_t *data, int length)
{
    std::queue <axi4_master> req_q;
    std::queue <axi_resp_t>  resp_q;    

    // Generate read requests
    while (length > 0)
    {
        int chunk = 1;

        if (BURSTABLE(addr, length, 32))
            chunk = 32;
        else if (BURSTABLE(addr, length, 16))
            chunk = 16;
        else if (BURSTABLE(addr, length, 8))
            chunk = 8;
        else if (BURSTABLE(addr, length, 4))
            chunk = 4;
        else
            chunk = 1;

        if (chunk == 1 || !m_enable_bursts)
        {
            uint32_t resp_data = 0;

            uint32_t addr_offset = addr & 3;
            int size = (4 - addr_offset);
            if (size > length)
                size = length;

            axi4_master req;
            sc_uint <AXI4_ID_W> id = get_rand_id();

            req.ARVALID = true;
            req.ARADDR  = addr & ~3;
            req.ARID    = id;
            req.ARLEN   = 1 - 1;

            req_q.push(req);

            // Expected response details
            axi_resp_t resp;
            resp.addr = addr;
            resp.size = size;
            resp.id   = id;
            resp.last = true;
            resp_q.push(resp);

            addr += size;
            length -= size;
        }
        else
        {
            axi4_master req;
            sc_uint <AXI4_ID_W> id = get_rand_id();

            req.ARVALID = true;
            req.ARADDR  = addr & ~3;
            req.ARID    = id;
            req.ARBURST = AXI4_BURST_INCR;
            req.ARLEN   = (chunk / 4) - 1;
            
            req_q.push(req);

            for (int i=0;i<(chunk / 4);i++)
            {
                // Expected response details
                axi_resp_t resp;
                resp.addr = addr;
                resp.size = 4;
                resp.id   = id;
                resp.last = (i + 1) == (chunk / 4);
                resp_q.push(resp);

                addr += 4;
                length -= 4;
            }
        }
    }

    // Issue AXI transactions, wait for responses
    while (req_q.size() > 0 || resp_q.size() > 0)
    {
        axi4_master axi_o = axi_out.read();
        axi4_slave  axi_i = axi_in.read();

        // Read response
        if (axi_i.RVALID && axi_o.RREADY)
        {
            sc_assert(resp_q.size() > 0);

            axi_resp_t resp = resp_q.front();
            resp_q.pop();

            sc_assert(axi_i.RID == resp.id);
            sc_assert(axi_i.RRESP == AXI4_RESP_OKAY);
            sc_assert(axi_i.RLAST == resp.last);

            uint32_t addr_offset = resp.addr & 3;
            uint32_t resp_data = (uint32_t)axi_i.RDATA;
            for (int x=0;x<resp.size;x++)
               *data++ = resp_data >> (8 * (addr_offset + x));

           if (axi_i.RLAST)
           {
                sc_assert(m_resp_pending > 0);
                m_resp_pending -= 1;
           }
        }

        // Read command issued
        if (axi_o.ARVALID && axi_i.ARREADY)
        {
            axi_o.ARVALID = false;
            m_resp_pending+= 1;
        }

        // Issue new request cycle?
        if (!axi_o.ARVALID && req_q.size() > 0 && !delay_cycle())
        {
            axi_o = req_q.front();
            req_q.pop();
        }

        axi_o.RREADY = !delay_cycle();
        axi_out.write(axi_o);

        wait();      
    }    
}
//-----------------------------------------------------------------
// write32: Write a 32-bit word (must be aligned)
//-----------------------------------------------------------------
void tb_axi4_driver::write32(uint32_t addr, uint32_t data)
{
    uint8_t arr[4];

    for (int i=0;i<4;i++)
        arr[i] = (data >> (i*8)) & 0xFF;

    sc_assert(!(addr & 3));
    write(addr, arr, 4);
}
//-----------------------------------------------------------------
// write32: Write a 32-bit word (must be aligned)
//-----------------------------------------------------------------
void tb_axi4_driver::write32(uint32_t addr, uint32_t data, uint8_t mask)
{
    uint8_t arr[4];

    for (int i=0;i<4;i++)
        arr[i] = (data >> (i*8)) & 0xFF;

    sc_assert(!(addr & 3));
    write_internal(addr, arr, 4, mask);
}
//-----------------------------------------------------------------
// read32: Read a 32-bit word (must be aligned)
//-----------------------------------------------------------------
uint32_t tb_axi4_driver::read32(uint32_t addr)
{
    uint8_t data[4];
    uint32_t resp_word = 0;

    sc_assert(!(addr & 3));
    read(addr, data, 4);

    resp_word = data[3];
    resp_word<<=8;
    resp_word|= data[2];
    resp_word<<=8;
    resp_word|= data[1];
    resp_word<<=8;
    resp_word|= data[0];

    return resp_word;
}
//-----------------------------------------------------------------
// write: Write a byte
//-----------------------------------------------------------------
void tb_axi4_driver::write(uint32_t addr, uint8_t data)
{
    write(addr, &data, 1);
}
//-----------------------------------------------------------------
// read: Read a byte
//-----------------------------------------------------------------
uint8_t tb_axi4_driver::read(uint32_t addr)
{
    uint8_t data = 0;
    read(addr, &data, 1);
    return data;
}