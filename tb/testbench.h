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

        m_sequencer->start(50000);
        m_sequencer->wait_complete();
        sc_stop();
    }

    SC_HAS_PROCESS(testbench);
    testbench(sc_module_name name): testbench_vbase(name)
    {    
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
