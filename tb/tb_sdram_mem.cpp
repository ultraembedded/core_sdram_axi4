#include "tb_sdram_mem.h"
#include <queue>

#define SDRAM_COL_W   9
#define SDRAM_BANK_W  2
#define SDRAM_ROW_W   13
#define NUM_ROWS      (1 << SDRAM_ROW_W)

// REF: https://www.micron.com/~/media/documents/products/data-sheet/dram/128mb_x4x8x16_ait-aat_sdram.pdf

#define MAX_ROW_OPEN_TIME     sc_time(35, SC_US)
#define MIN_ACTIVE_TO_ACTIVE  sc_time(60, SC_NS)
#define MIN_ACTIVE_TO_ACCESS  sc_time(15, SC_NS)
#define MAX_ROW_REFRESH_TIME  (sc_time((64000000 / NUM_ROWS), SC_NS) + sc_time(200, SC_NS)) // Add some slack (FIXME) 

#define DPRINTF //printf

//-----------------------------------------------------------------
// process: Handle requests
//-----------------------------------------------------------------
void tb_sdram_mem::process(void)
{
    typedef enum
    {
        SDRAM_CMD_INHIBIT = 0,
        SDRAM_CMD_NOP,
        SDRAM_CMD_ACTIVE,
        SDRAM_CMD_READ,
        SDRAM_CMD_WRITE,
        SDRAM_CMD_BURST_TERM,
        SDRAM_CMD_PRECHARGE,
        SDRAM_CMD_REFRESH,
        SDRAM_CMD_LOAD_MODE
    } t_sdram_cmd;

    sc_uint <SDRAM_COL_W>  col = 0;
    sc_uint <SDRAM_ROW_W>  row = 0;
    sc_uint <SDRAM_BANK_W> bank = 0;
    sc_uint <32>           addr = 0;

    uint16_t resp_data[3];

    // Clear response pipeline
    for (int i=0;i<sizeof(resp_data)/sizeof(resp_data[0]);i++)
        resp_data[i] = 0;

    for (unsigned b=0;b<NUM_BANKS;b++)
        m_activate_time[b] = sc_time_stamp();

    m_refresh_cnt = 0;
    while (1)
    {
        sdram_io_master sdram_i = sdram_in.read();
        sdram_io_slave  sdram_o = sdram_out.read();

        t_sdram_cmd new_cmd;

        // Command decoder
        if (sdram_i.CS)
            new_cmd = SDRAM_CMD_INHIBIT;
        else
        {
            if (sdram_i.RAS && sdram_i.CAS && sdram_i.WE)
                new_cmd = SDRAM_CMD_NOP;
            else if (!sdram_i.RAS && sdram_i.CAS && sdram_i.WE)
                new_cmd = SDRAM_CMD_ACTIVE;
            else if (sdram_i.RAS && !sdram_i.CAS && sdram_i.WE)
                new_cmd = SDRAM_CMD_READ;
            else if (sdram_i.RAS && !sdram_i.CAS && !sdram_i.WE)
                new_cmd = SDRAM_CMD_WRITE;
            else if (sdram_i.RAS && sdram_i.CAS && !sdram_i.WE)
                new_cmd = SDRAM_CMD_BURST_TERM;
            else if (!sdram_i.RAS && sdram_i.CAS && !sdram_i.WE)
                new_cmd = SDRAM_CMD_PRECHARGE;
            else if (!sdram_i.RAS && !sdram_i.CAS && sdram_i.WE)
                new_cmd = SDRAM_CMD_REFRESH;
            else if (!sdram_i.RAS && !sdram_i.CAS && !sdram_i.WE)
                new_cmd = SDRAM_CMD_LOAD_MODE;
            else
                sc_assert(0); // NOT SURE...
        }

        // Check row open time...
        for (unsigned b = 0;b < NUM_BANKS;b++)
            if (m_active_row[b] != -1 && (sc_time_stamp() - m_activate_time[b]) > MAX_ROW_OPEN_TIME)
            {
                sc_assert(!"Row open too long...");
            }

        // Configure SDRAM
        if (new_cmd == SDRAM_CMD_LOAD_MODE)
        {
            m_configured      = true;
            m_burst_type      = (tBurstType)(int)sdram_i.ADDR[3];
            m_write_burst_en  = (bool)!sdram_i.ADDR[9];
            m_burst_length    = (tBurstLength)(int)sdram_i.ADDR.range(2,0);
            m_cas_latency     = (int)sdram_i.ADDR.range(6,4);

            DPRINTF("SDRAM: MODE - write burst %d, burst len %d, CAS latency %d\n", m_write_burst_en, m_burst_length, m_cas_latency);

            sc_assert(m_burst_type == BURST_TYPE_SEQUENTIAL);
        }
        // Auto refresh
        else if (new_cmd == SDRAM_CMD_REFRESH)
        {
            //cout << "SDRAM: REFRESH @ " << sc_time_stamp() << " - delta = " << (sc_time_stamp() - m_last_refresh) << " - allowed " << MAX_ROW_REFRESH_TIME <<endl;

            // Check no rows open..
            for (unsigned b = 0;b < NUM_BANKS;b++)
            {
                sc_assert(m_active_row[b] == -1);
            }

            // Once init sequence complete, check for auto-refresh period...
            if (m_refresh_cnt > 2)
            {
                sc_assert((sc_time_stamp() - m_last_refresh) < MAX_ROW_REFRESH_TIME);
            }

            m_last_refresh = sc_time_stamp();

            if (m_refresh_cnt < 0xFFFFFFFF)
                m_refresh_cnt += 1;
        }
        // Row is activated and copied into the row buffer of the bank
        else if (new_cmd == SDRAM_CMD_ACTIVE)
        {
            sc_assert(m_configured);
            sc_assert(m_refresh_cnt >= 2);

            bank = sdram_i.BA;
            row  = sdram_i.ADDR;

            DPRINTF("SDRAM: ACTIVATE Row=%x, Bank=%x\n", (unsigned)row, (unsigned)bank);

            // A row should not be open
            sc_assert(m_active_row[bank] == -1);

            // ACTIVATE periods long enough...
            sc_assert((sc_time_stamp() - m_activate_time[bank]) > MIN_ACTIVE_TO_ACTIVE);

            // Mark row as open
            m_active_row[bank]    = row;
            m_activate_time[bank] = sc_time_stamp();
        }
        // Read command
        else if (new_cmd == SDRAM_CMD_READ)
        {
            sc_assert(m_configured);

            bool en_ap = sdram_i.ADDR[SDRAM_COL_W];
            col        = sdram_i.ADDR;
            bank       = sdram_i.BA;
            row        = m_active_row[bank];

            // A row should be open
            sc_assert(m_active_row[bank] != -1);

            // DQM expected to be low
            sc_assert(sdram_i.DQM == 0x0);

            // Check row activate timing
            sc_assert((sc_time_stamp() - m_activate_time[bank]) > MIN_ACTIVE_TO_ACCESS);

            // Address = RBC
            addr.range(SDRAM_COL_W, 2)                                       = col.range(SDRAM_COL_W-1, 1);
            addr.range(SDRAM_COL_W+SDRAM_BANK_W, SDRAM_COL_W+SDRAM_BANK_W-1) = bank;
            addr.range(31, SDRAM_COL_W+SDRAM_BANK_W+1) = row;

            m_burst_offset = 0;

            uint32_t data = read32((uint32_t)addr);
            DPRINTF("SDRAM: READ %08x = %08x [Row=%x, Bank=%x, Col=%x]\n", (uint32_t)addr, data, (unsigned)row, (unsigned)bank, (unsigned)col);

            resp_data[m_cas_latency-2] = data >> (m_burst_offset * 8);
            m_burst_offset += 2;

            switch (m_burst_length)
            {
                default:
                case BURST_LEN_1:
                    m_burst_read = 1-1;
                    break;
                case BURST_LEN_2:
                    m_burst_read = 2-1;
                    break;
                case BURST_LEN_4:
                    m_burst_read = 4-1;
                    break;
                case BURST_LEN_8:
                    m_burst_read = 8-1;
                    break;
            }

            m_burst_close_row[bank] = en_ap;
        }
        // Write command
        else if (new_cmd == SDRAM_CMD_WRITE)
        {
            sc_assert(m_configured);

            bool en_ap = sdram_i.ADDR[SDRAM_COL_W];
            col        = sdram_i.ADDR;
            bank       = sdram_i.BA;
            row        = m_active_row[bank];

            // A row should be open
            sc_assert(m_active_row[bank] != -1);

            // Check row activate timing
            sc_assert((sc_time_stamp() - m_activate_time[bank]) > MIN_ACTIVE_TO_ACCESS);

            // Address = RBC
            addr.range(SDRAM_COL_W, 2)                                       = col.range(SDRAM_COL_W-1, 1);
            addr.range(SDRAM_COL_W+SDRAM_BANK_W, SDRAM_COL_W+SDRAM_BANK_W-1) = bank;
            addr.range(31, SDRAM_COL_W+SDRAM_BANK_W+1) = row;

            uint32_t data = (uint32_t)sdram_i.DATA_OUTPUT;
            uint8_t  mask = 0;
            
            m_burst_offset = 0;

            data <<= (m_burst_offset * 8);
            mask = 0x3 << (m_burst_offset);

            // Lower byte - disabled
            if (sdram_i.DQM[0])
            {
                data &= ~(0xFF << ((m_burst_offset + 0) * 8));
                mask &= ~(1 << (m_burst_offset + 0));
            }

            // Upper byte disabled
            if (sdram_i.DQM[1])
            {
                data &= ~(0xFF << ((m_burst_offset + 1) * 8));
                mask &= ~(1 << (m_burst_offset + 1));
            }
 
            DPRINTF("SDRAM: WRITE %08x = %08x MASK=%x [Row=%x, Bank=%x, Col=%x]\n", (uint32_t)addr, data, mask, (unsigned)row, (unsigned)bank, (unsigned)col);
            write32((uint32_t)addr, ((uint32_t)data) << 0, mask);
            m_burst_offset += 2;

            // Configure remaining burst length
            if (m_write_burst_en)
            {
                switch (m_burst_length)
                {
                    default:
                    case BURST_LEN_1:
                        m_burst_write = 1-1;
                        break;
                    case BURST_LEN_2:
                        m_burst_write = 2-1;
                        break;
                    case BURST_LEN_4:
                        m_burst_write = 4-1;
                        break;
                    case BURST_LEN_8:
                        m_burst_write = 8-1;
                        break;
                }
            }
            else
                m_burst_write = 0;

            m_burst_close_row[bank] = en_ap;
        }
        // Row is precharged and stored back into the memory array
        else if (new_cmd == SDRAM_CMD_PRECHARGE)
        {
            sc_assert(m_configured);

            // All banks
            if (sdram_i.ADDR[10])
            {
                // Close rows
                for (unsigned i=0;i<NUM_BANKS;i++)
                    m_active_row[i] = -1;

                DPRINTF("SDRAM: PRECHARGE - all banks\n");

            }
            // Specified bank
            else
            {
                bank       = sdram_i.BA;

                DPRINTF("SDRAM: PRECHARGE Bank=%x, Active Row=%x\n", (unsigned)bank, (unsigned)m_active_row[bank]);

                // Close specific row
                m_active_row[bank] = -1;
            }
        }
        // Terminate read or write burst
        else if (new_cmd == SDRAM_CMD_BURST_TERM)
        {
            m_burst_write = 0;
            m_burst_read  = 0;

            DPRINTF("SDRAM: Burst terminate\n");
        }

        // WRITE: Burst continuation...
        if (m_burst_write > 0 && new_cmd == SDRAM_CMD_NOP)
        {
            uint32_t data = (uint32_t)sdram_i.DATA_OUTPUT;
            uint8_t  mask = 0;

            data <<= (m_burst_offset * 8);
            mask = 0x3 << (m_burst_offset);

            // Lower byte - disabled
            if (sdram_i.DQM[0])
            {
                data &= ~(0xFF << ((m_burst_offset + 0) * 8));
                mask &= ~(1 << (m_burst_offset + 0));
            }

            // Upper byte disabled
            if (sdram_i.DQM[1])
            {
                data &= ~(0xFF << ((m_burst_offset + 1) * 8));
                mask &= ~(1 << (m_burst_offset + 1));
            }
 
            DPRINTF("SDRAM: WRITE %08x = %08x MASK=%x [Row=%x, Bank=%x, Col=%x]\n", (uint32_t)addr, data, mask, (unsigned)row, (unsigned)bank, (unsigned)col);
            write32((uint32_t)addr, ((uint32_t)data) << 0, mask);
            m_burst_offset += 2;

            // Continue...
            if (m_burst_offset == 4)
            {
                m_burst_offset = 0;
                addr += 4;
            }

            m_burst_write -= 1;

            if (m_burst_write == 0 && m_burst_close_row[bank])
            {
                // Close specific row
                m_active_row[bank] = -1;
            }
        }
        // READ: Burst continuation
        else if (m_burst_read > 0 && new_cmd == SDRAM_CMD_NOP)
        {
            uint32_t data = read32((uint32_t)addr);
            DPRINTF("SDRAM: READ %08x = %08x [Row=%x, Bank=%x, Col=%x]\n", (uint32_t)addr, data, (unsigned)row, (unsigned)bank, (unsigned)col);

            resp_data[m_cas_latency-2] = data >> (m_burst_offset * 8);
            m_burst_offset += 2;

            // Continue...
            if (m_burst_offset == 4)
            {
                m_burst_offset = 0;
                addr += 4;
            }

            m_burst_read -= 1;

            if (m_burst_read == 0 && m_burst_close_row[bank])
            {
                // Close specific row
                m_active_row[bank] = -1;
            }
        }

        sdram_o.DATA_INPUT = resp_data[0];

        // Shuffle read data
        for (int i=1;i<sizeof(resp_data)/sizeof(resp_data[0]);i++)
            resp_data[i-1] = resp_data[i];

        sdram_out.write(sdram_o);
        wait();
    }
}
//-----------------------------------------------------------------
// write32: Write a 32-bit word to memory
//-----------------------------------------------------------------
void tb_sdram_mem::write32(uint32_t addr, uint32_t data, uint8_t strb)
{
    for (int i=0;i<4;i++)
        if (strb & (1 << i))
            tb_memory::write(addr + i,data >> (i*8));
}
//-----------------------------------------------------------------
// read32: Read a 32-bit word from memory
//-----------------------------------------------------------------
uint32_t tb_sdram_mem::read32(uint32_t addr)
{
    uint32_t data = 0;
    for (int i=0;i<4;i++)
        data |= ((uint32_t)tb_memory::read(addr + i)) << (i*8);
    return data;
}
//-----------------------------------------------------------------
// write: Byte write
//-----------------------------------------------------------------
void tb_sdram_mem::write(uint32_t addr, uint8_t data)
{
    tb_memory::write(addr, data);
}
//-----------------------------------------------------------------
// read: Byte read
//-----------------------------------------------------------------
uint8_t tb_sdram_mem::read(uint32_t addr)
{
    return tb_memory::read(addr);
}
