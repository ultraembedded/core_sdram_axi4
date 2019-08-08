
#include "sdram_axi.h"
#include "Vsdram_axi.h"

#if VM_TRACE
#include "verilated.h"
#include "verilated_vcd_sc.h"
#endif

//-------------------------------------------------------------
// Constructor
//-------------------------------------------------------------
sdram_axi::sdram_axi(sc_module_name name): sc_module(name)
{
    m_rtl = new Vsdram_axi("Vsdram_axi");
    m_rtl->clk_i(m_clk_in);
    m_rtl->rst_i(m_rst_in);
    m_rtl->inport_awvalid_i(m_inport_awvalid_in);
    m_rtl->inport_awaddr_i(m_inport_awaddr_in);
    m_rtl->inport_awid_i(m_inport_awid_in);
    m_rtl->inport_awlen_i(m_inport_awlen_in);
    m_rtl->inport_awburst_i(m_inport_awburst_in);
    m_rtl->inport_wvalid_i(m_inport_wvalid_in);
    m_rtl->inport_wdata_i(m_inport_wdata_in);
    m_rtl->inport_wstrb_i(m_inport_wstrb_in);
    m_rtl->inport_wlast_i(m_inport_wlast_in);
    m_rtl->inport_bready_i(m_inport_bready_in);
    m_rtl->inport_arvalid_i(m_inport_arvalid_in);
    m_rtl->inport_araddr_i(m_inport_araddr_in);
    m_rtl->inport_arid_i(m_inport_arid_in);
    m_rtl->inport_arlen_i(m_inport_arlen_in);
    m_rtl->inport_arburst_i(m_inport_arburst_in);
    m_rtl->inport_rready_i(m_inport_rready_in);
    m_rtl->sdram_data_input_i(m_sdram_data_input_in);
    m_rtl->inport_awready_o(m_inport_awready_out);
    m_rtl->inport_wready_o(m_inport_wready_out);
    m_rtl->inport_bvalid_o(m_inport_bvalid_out);
    m_rtl->inport_bresp_o(m_inport_bresp_out);
    m_rtl->inport_bid_o(m_inport_bid_out);
    m_rtl->inport_arready_o(m_inport_arready_out);
    m_rtl->inport_rvalid_o(m_inport_rvalid_out);
    m_rtl->inport_rdata_o(m_inport_rdata_out);
    m_rtl->inport_rresp_o(m_inport_rresp_out);
    m_rtl->inport_rid_o(m_inport_rid_out);
    m_rtl->inport_rlast_o(m_inport_rlast_out);
    m_rtl->sdram_clk_o(m_sdram_clk_out);
    m_rtl->sdram_cke_o(m_sdram_cke_out);
    m_rtl->sdram_cs_o(m_sdram_cs_out);
    m_rtl->sdram_ras_o(m_sdram_ras_out);
    m_rtl->sdram_cas_o(m_sdram_cas_out);
    m_rtl->sdram_we_o(m_sdram_we_out);
    m_rtl->sdram_dqm_o(m_sdram_dqm_out);
    m_rtl->sdram_addr_o(m_sdram_addr_out);
    m_rtl->sdram_ba_o(m_sdram_ba_out);
    m_rtl->sdram_data_output_o(m_sdram_data_output_out);
    m_rtl->sdram_data_out_en_o(m_sdram_data_out_en_out);

    SC_METHOD(async_outputs);
    sensitive << clk_in;
    sensitive << rst_in;
    sensitive << inport_in;
    sensitive << sdram_in;
    sensitive << m_inport_awready_out;
    sensitive << m_inport_wready_out;
    sensitive << m_inport_bvalid_out;
    sensitive << m_inport_bresp_out;
    sensitive << m_inport_bid_out;
    sensitive << m_inport_arready_out;
    sensitive << m_inport_rvalid_out;
    sensitive << m_inport_rdata_out;
    sensitive << m_inport_rresp_out;
    sensitive << m_inport_rid_out;
    sensitive << m_inport_rlast_out;
    sensitive << m_sdram_clk_out;
    sensitive << m_sdram_cke_out;
    sensitive << m_sdram_cs_out;
    sensitive << m_sdram_ras_out;
    sensitive << m_sdram_cas_out;
    sensitive << m_sdram_we_out;
    sensitive << m_sdram_dqm_out;
    sensitive << m_sdram_addr_out;
    sensitive << m_sdram_ba_out;
    sensitive << m_sdram_data_output_out;
    sensitive << m_sdram_data_out_en_out;

#if VM_TRACE
    m_vcd         = NULL;
    m_delay_waves = false;
#endif
}
//-------------------------------------------------------------
// trace_enable
//-------------------------------------------------------------
void sdram_axi::trace_enable(VerilatedVcdSc * p)
{
#if VM_TRACE
    m_vcd = p;
    m_rtl->trace (m_vcd, 99);
#endif
}
void sdram_axi::trace_enable(VerilatedVcdSc *p, sc_core::sc_time start_time)
{
#if VM_TRACE
    m_vcd = p;
    m_delay_waves = true;
    m_waves_start = start_time;
    //m_rtl->trace (m_vcd, 99);
#endif
}
//-------------------------------------------------------------
// async_outputs
//-------------------------------------------------------------
void sdram_axi::async_outputs(void)
{
    m_clk_in.write(clk_in.read());
    m_rst_in.write(rst_in.read());

    axi4_master inport_i = inport_in.read();
    m_inport_awvalid_in.write(inport_i.AWVALID); 
    m_inport_awaddr_in.write(inport_i.AWADDR); 
    m_inport_awid_in.write(inport_i.AWID); 
    m_inport_awlen_in.write(inport_i.AWLEN); 
    m_inport_awburst_in.write(inport_i.AWBURST); 
    m_inport_wvalid_in.write(inport_i.WVALID); 
    m_inport_wdata_in.write(inport_i.WDATA); 
    m_inport_wstrb_in.write(inport_i.WSTRB); 
    m_inport_wlast_in.write(inport_i.WLAST); 
    m_inport_bready_in.write(inport_i.BREADY); 
    m_inport_arvalid_in.write(inport_i.ARVALID); 
    m_inport_araddr_in.write(inport_i.ARADDR); 
    m_inport_arid_in.write(inport_i.ARID); 
    m_inport_arlen_in.write(inport_i.ARLEN); 
    m_inport_arburst_in.write(inport_i.ARBURST); 
    m_inport_rready_in.write(inport_i.RREADY); 


    axi4_slave inport_o;
    inport_o.AWREADY = m_inport_awready_out.read(); 
    inport_o.WREADY = m_inport_wready_out.read(); 
    inport_o.BVALID = m_inport_bvalid_out.read(); 
    inport_o.BRESP = m_inport_bresp_out.read(); 
    inport_o.BID = m_inport_bid_out.read(); 
    inport_o.ARREADY = m_inport_arready_out.read(); 
    inport_o.RVALID = m_inport_rvalid_out.read(); 
    inport_o.RDATA = m_inport_rdata_out.read(); 
    inport_o.RRESP = m_inport_rresp_out.read(); 
    inport_o.RID = m_inport_rid_out.read(); 
    inport_o.RLAST = m_inport_rlast_out.read(); 
    inport_out.write(inport_o);
    sdram_io_slave sdram_i = sdram_in.read();
    m_sdram_data_input_in.write(sdram_i.DATA_INPUT); 


    sdram_io_master sdram_o;
    sdram_o.CLK = m_sdram_clk_out.read(); 
    sdram_o.CKE = m_sdram_cke_out.read(); 
    sdram_o.CS = m_sdram_cs_out.read(); 
    sdram_o.RAS = m_sdram_ras_out.read(); 
    sdram_o.CAS = m_sdram_cas_out.read(); 
    sdram_o.WE = m_sdram_we_out.read(); 
    sdram_o.DQM = m_sdram_dqm_out.read(); 
    sdram_o.ADDR = m_sdram_addr_out.read(); 
    sdram_o.BA = m_sdram_ba_out.read(); 
    sdram_o.DATA_OUTPUT = m_sdram_data_output_out.read(); 
    sdram_o.DATA_OUT_EN = m_sdram_data_out_en_out.read(); 
    sdram_out.write(sdram_o);

}
