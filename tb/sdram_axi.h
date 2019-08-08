
#ifndef SDRAM_AXI_H
#define SDRAM_AXI_H
#include <systemc.h>

#include "axi4.h"
#include "sdram_io.h"

class Vsdram_axi;
class VerilatedVcdSc;

//-------------------------------------------------------------
// sdram_axi: RTL wrapper class
//-------------------------------------------------------------
class sdram_axi: public sc_module
{
public:
    sc_in <bool> clk_in;
    sc_in <bool> rst_in;

    sc_in  <axi4_master>  inport_in;
    sc_out <axi4_slave> inport_out;
    sc_in  <sdram_io_slave>  sdram_in;
    sc_out <sdram_io_master> sdram_out;

    //-------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------
    SC_HAS_PROCESS(sdram_axi);
    sdram_axi(sc_module_name name);

    //-------------------------------------------------------------
    // Trace
    //-------------------------------------------------------------
    virtual void add_trace(sc_trace_file *vcd, std::string prefix)
    {
        #undef  TRACE_SIGNAL
        #define TRACE_SIGNAL(s) sc_trace(vcd,s,prefix + #s)

        TRACE_SIGNAL(clk_in);
        TRACE_SIGNAL(rst_in);
        TRACE_SIGNAL(inport_in);
        TRACE_SIGNAL(inport_out);
        TRACE_SIGNAL(sdram_in);
        TRACE_SIGNAL(sdram_out);

        #undef  TRACE_SIGNAL
    }

    void async_outputs(void);
    void trace_rtl(void);
    void trace_enable(VerilatedVcdSc *p);
    void trace_enable(VerilatedVcdSc *p, sc_core::sc_time start_time);

    //-------------------------------------------------------------
    // Signals
    //-------------------------------------------------------------
private:
    sc_signal <bool> m_clk_in;
    sc_signal <bool> m_rst_in;
    sc_signal <bool> m_inport_awvalid_in;
    sc_signal <sc_uint<32> > m_inport_awaddr_in;
    sc_signal <sc_uint<4> > m_inport_awid_in;
    sc_signal <sc_uint<8> > m_inport_awlen_in;
    sc_signal <sc_uint<2> > m_inport_awburst_in;
    sc_signal <bool> m_inport_wvalid_in;
    sc_signal <sc_uint<32> > m_inport_wdata_in;
    sc_signal <sc_uint<4> > m_inport_wstrb_in;
    sc_signal <bool> m_inport_wlast_in;
    sc_signal <bool> m_inport_bready_in;
    sc_signal <bool> m_inport_arvalid_in;
    sc_signal <sc_uint<32> > m_inport_araddr_in;
    sc_signal <sc_uint<4> > m_inport_arid_in;
    sc_signal <sc_uint<8> > m_inport_arlen_in;
    sc_signal <sc_uint<2> > m_inport_arburst_in;
    sc_signal <bool> m_inport_rready_in;
    sc_signal <sc_uint<16> > m_sdram_data_input_in;

    sc_signal <bool> m_inport_awready_out;
    sc_signal <bool> m_inport_wready_out;
    sc_signal <bool> m_inport_bvalid_out;
    sc_signal <sc_uint<2> > m_inport_bresp_out;
    sc_signal <sc_uint<4> > m_inport_bid_out;
    sc_signal <bool> m_inport_arready_out;
    sc_signal <bool> m_inport_rvalid_out;
    sc_signal <sc_uint<32> > m_inport_rdata_out;
    sc_signal <sc_uint<2> > m_inport_rresp_out;
    sc_signal <sc_uint<4> > m_inport_rid_out;
    sc_signal <bool> m_inport_rlast_out;
    sc_signal <bool> m_sdram_clk_out;
    sc_signal <bool> m_sdram_cke_out;
    sc_signal <bool> m_sdram_cs_out;
    sc_signal <bool> m_sdram_ras_out;
    sc_signal <bool> m_sdram_cas_out;
    sc_signal <bool> m_sdram_we_out;
    sc_signal <sc_uint<2> > m_sdram_dqm_out;
    sc_signal <sc_uint<13> > m_sdram_addr_out;
    sc_signal <sc_uint<2> > m_sdram_ba_out;
    sc_signal <sc_uint<16> > m_sdram_data_output_out;
    sc_signal <bool> m_sdram_data_out_en_out;

public:
    Vsdram_axi *m_rtl;
#if VM_TRACE
    VerilatedVcdSc * m_vcd;
    bool             m_delay_waves;
    sc_core::sc_time m_waves_start;
#endif 
};

#endif