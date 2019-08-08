#ifndef TB_AXI4_DRIVER_H
#define TB_AXI4_DRIVER_H

#include "axi4_defines.h"
#include "axi4.h"
#include "tb_driver_api.h"

//-------------------------------------------------------------
// tb_axi4_driver: AXI4 driver interface
//-------------------------------------------------------------
class tb_axi4_driver: public sc_module, public tb_driver_api
{
public:
    //-------------------------------------------------------------
    // Interface I/O
    //-------------------------------------------------------------
    sc_out <axi4_master> axi_out;
    sc_in  <axi4_slave>  axi_in;

    //-------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------
    SC_HAS_PROCESS(tb_axi4_driver);
    tb_axi4_driver(sc_module_name name): sc_module(name) 
    {
        m_enable_delays = true;
        m_enable_bursts = true;
        m_min_id        = 0;
        m_max_id        = 15;
        m_resp_pending  = 0;
    }

    //-------------------------------------------------------------
    // Trace
    //-------------------------------------------------------------
    void add_trace(sc_trace_file *vcd, std::string prefix)
    {
        #undef  TRACE_SIGNAL
        #define TRACE_SIGNAL(s) sc_trace(vcd,s,prefix + #s)

        TRACE_SIGNAL(axi_out);
        TRACE_SIGNAL(axi_in);
        TRACE_SIGNAL(m_resp_pending);

        #undef  TRACE_SIGNAL
    }

    //-------------------------------------------------------------
    // API
    //-------------------------------------------------------------
    void         enable_delays(bool enable) { m_enable_delays = enable; }
    void         enable_bursts(bool enable) { m_enable_bursts = enable; }

    // ID control
    int          get_rand_id(void)
    {
        if ((m_max_id - m_min_id) > 0)
            return m_min_id + rand() % (m_max_id - m_min_id);
        else
            return m_min_id;
    }
    void         set_id_range(int min_id, int max_id) 
    {
        m_min_id = min_id;
        m_max_id = max_id;
    }

    void         write(uint32_t, uint8_t data);
    uint8_t      read(uint32_t addr);

    void         write32(uint32_t addr, uint32_t data);
    void         write32(uint32_t addr, uint32_t data, uint8_t mask);
    uint32_t     read32(uint32_t addr);

    void         write(uint32_t addr, uint8_t *data, int length);
    void         read(uint32_t addr, uint8_t *data, int length);

    bool         delay_cycle(void) { return m_enable_delays ? rand() & 1 : 0; }

protected:
    void         write_internal(uint32_t addr, uint8_t *data, int length, uint8_t initial_mask);

    //-------------------------------------------------------------
    // Members
    //-------------------------------------------------------------
    bool m_enable_delays;
    bool m_enable_bursts;
    int  m_min_id;
    int  m_max_id;

    uint32_t m_resp_pending;
};

#endif