#include <systemc.h>
#include "testbench_vbase.h"

#include "tb_memory.h"
#include "tb_axi4_driver.h"
#include "tb_sdram_mem.h"
#include "tb_mem_test.h"

#include "sdram_axi.h"

#define MEM_BASE 0x00000000
#define MEM_SIZE (512 * 1024)

//-----------------------------------------------------------------
// Module
//-----------------------------------------------------------------
class testbench: public testbench_vbase
{
public:
    tb_axi4_driver           *m_driver;
    tb_mem_test              *m_sequencer;
    sdram_axi                *m_dut;
    tb_sdram_mem             *m_mem;

    sc_signal <axi4_master>    axi_m;
    sc_signal <axi4_slave>     axi_s;

    sc_signal <sdram_io_master>    sdram_io_m;
    sc_signal <sdram_io_slave>     sdram_io_s;

    //-----------------------------------------------------------------
    // test_zero_strobes: Masked beats must complete without changing memory
    //-----------------------------------------------------------------
    void test_zero_strobes(void)
    {
        for (int testcase=0;testcase<7;testcase++)
        {
            int beats = (testcase == 6) ? 1 : 4;
            int burst = testcase % 3;
            uint32_t base = MEM_BASE + 0x100 + testcase * 32;
            uint32_t addr = base + ((burst == AXI4_BURST_WRAP) ? 8 : 0);
            uint32_t expected[4];
            for (int i=0;i<4;i++)
                expected[i] = m_driver->read32(base + i * 4);

            axi4_master req;
            req.AWVALID = true;
            req.AWADDR  = addr;
            req.AWID    = testcase + 1;
            req.AWLEN   = beats - 1;
            req.AWBURST = burst;
            req.WVALID  = (testcase < 3);
            req.WDATA   = 0xA5C37E19u;
            req.WSTRB   = 0;

            int sent = 0;
            int gap = 0;
            int response_wait = 0;
            bool done = false;
            m_driver->axi_out.write(req);
            wait();

            for (int cycles=0;cycles<20000 && !done;cycles++)
            {
                axi4_slave resp = axi_s.read();
                if (req.AWVALID && resp.AWREADY)
                    req.AWVALID = false;
                if (req.WVALID && resp.WREADY)
                {
                    int word = (addr - base) / 4;
                    for (int byte=0;byte<4;byte++)
                        if (req.WSTRB[byte])
                        {
                            uint32_t mask = 0xFFu << (byte * 8);
                            expected[word] = (expected[word] & ~mask) | ((uint32_t)req.WDATA & mask);
                        }
                    if (burst == AXI4_BURST_INCR)
                        addr += 4;
                    else if (burst == AXI4_BURST_WRAP)
                        addr = base + ((addr - base + 4) & 15);
                    sent++;
                    req.WVALID = false;
                    gap = 2;
                }

                if (response_wait)
                    sc_assert(resp.BVALID);
                if (resp.BVALID)
                {
                    sc_assert(!req.AWVALID && sent == beats);
                    sc_assert(resp.BID == req.AWID);
                    sc_assert(resp.BRESP == AXI4_RESP_OKAY);
                    done = req.BREADY;
                    req.BREADY = (++response_wait >= 5);
                }

                // Test simultaneous AW/W and AW arriving seven clocks before W.
                if (!req.WVALID && sent < beats && cycles >= ((testcase < 3) ? 0 : 7) && gap-- <= 0)
                {
                    req.WVALID = true;
                    req.WDATA  = 0xA5C37E19u + sent;
                    req.WSTRB  = (testcase < 3 && sent == 1) ? 0x5 : 0;
                    req.WLAST  = (sent == beats - 1);
                }
                m_driver->axi_out.write(req);
                wait();
            }
            sc_assert(done && "Zero-strobe write timed out");

            req.init();
            m_driver->axi_out.write(req);
            for (int i=0;i<10;i++)
            {
                wait();
                sc_assert(!axi_s.read().BVALID && !axi_s.read().RVALID);
            }
            for (int i=0;i<4;i++)
                sc_assert(m_driver->read32(base + i * 4) == expected[i]);
        }
        printf("PASS: zero-strobe writes, FIXED/INCR/WRAP, response backpressure and memory preservation\n");
    }

    //-----------------------------------------------------------------
    // process: Drive input sequence
    //-----------------------------------------------------------------
    void process(void) 
    {
        wait();

        m_driver->enable_delays(true);

        // Allocate some memory
        m_mem->add_region(MEM_BASE, MEM_SIZE);

        // Allocate some memory
        m_sequencer->add_region(MEM_BASE, MEM_SIZE);
        m_sequencer->trace_access(true);

        // Initialise to memory known value
        for (int i=0;i<MEM_SIZE;i++)
        {
             m_sequencer->write(MEM_BASE + i, i);
             m_mem->write(MEM_BASE + i, i);
        }

        if (m_testcase == 0)
        {
            test_zero_strobes();
            sc_stop();
            return;
        }

        m_sequencer->start(50000);
        m_sequencer->wait_complete();
        sc_stop();
    }

    void set_testcase(int testcase) { m_testcase = testcase; }

    void monitor(void)
    {
        while (true)
        {
            if (m_testcase == 0)
                sc_assert(sc_time_stamp() < sc_time(2, SC_MS));
            wait();
        }
    }

    int m_testcase;

    SC_HAS_PROCESS(testbench);
    testbench(sc_module_name name): testbench_vbase(name)
    {    
        m_testcase = -1;

        m_driver = new tb_axi4_driver("DRIVER");
        m_driver->axi_out(axi_m);
        m_driver->axi_in(axi_s);

        m_sequencer = new tb_mem_test("SEQ", m_driver, 32);
        m_sequencer->clk_in(clk);
        m_sequencer->rst_in(rst);

        m_dut = new sdram_axi("MEM");
        m_dut->clk_in(clk);
        m_dut->rst_in(rst);
        m_dut->inport_in(axi_m);
        m_dut->inport_out(axi_s);
        m_dut->sdram_out(sdram_io_m);
        m_dut->sdram_in(sdram_io_s);

        m_mem = new tb_sdram_mem("TB_MEM");
        m_mem->clk_in(clk);
        m_mem->rst_in(rst);
        m_mem->sdram_in(sdram_io_m);
        m_mem->sdram_out(sdram_io_s);

        verilator_trace_enable("verilator.vcd", m_dut);
    }
};
