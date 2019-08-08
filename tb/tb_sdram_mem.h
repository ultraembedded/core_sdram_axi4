#ifndef TB_SDRAM_MEM_H
#define TB_SDRAM_MEM_H

#include "sdram_io.h"
#include "tb_memory.h"

//-------------------------------------------------------------
// tb_sdram_mem: SDRAM testbench memory
//-------------------------------------------------------------
class tb_sdram_mem: public sc_module, public tb_memory
{
public:
    //-------------------------------------------------------------
    // Interface I/O
    //-------------------------------------------------------------
    sc_in <bool>             clk_in;
    sc_in <bool>             rst_in;

    sc_in <sdram_io_master>  sdram_in;
    sc_out <sdram_io_slave>  sdram_out;

    //-------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------
    SC_HAS_PROCESS(tb_sdram_mem);
    tb_sdram_mem(sc_module_name name): sc_module(name) 
    { 
        SC_CTHREAD(process, clk_in.pos());
        m_enable_delays = true;

        m_configured = false;

        for (unsigned i=0;i<NUM_BANKS;i++)
            m_active_row[i] = -1;

        m_burst_write     = 0;
        m_burst_read      = 0;
    }

    //-------------------------------------------------------------
    // Trace
    //-------------------------------------------------------------
    void add_trace(sc_trace_file *vcd, std::string prefix)
    {
        #undef  TRACE_SIGNAL
        #define TRACE_SIGNAL(s) sc_trace(vcd,s,prefix + #s)

        TRACE_SIGNAL(sdram_out);
        TRACE_SIGNAL(sdram_in);

        #undef  TRACE_SIGNAL
    }

    //-------------------------------------------------------------
    // API
    //-------------------------------------------------------------
    void         enable_delays(bool enable) { m_enable_delays = enable; }
    void         write(uint32_t addr, uint8_t data);
    uint8_t      read(uint32_t addr);
    void         write32(uint32_t addr, uint32_t data, uint8_t strb = 0xF);
    uint32_t     read32(uint32_t addr);

    void         process(void);
    bool         delay_cycle(void) { return m_enable_delays ? rand() & 1 : 0; }

protected:
    bool         m_enable_delays;
    

    bool         m_configured;

    typedef enum eBurstType
    {
        BURST_TYPE_SEQUENTIAL,
        BURST_TYPE_INTERLEAVED
    } tBurstType;

    tBurstType   m_burst_type;

    typedef enum eBurstLength
    {
        BURST_LEN_1,
        BURST_LEN_2,
        BURST_LEN_4,
        BURST_LEN_8
    } tBurstLength;

    tBurstLength m_burst_length;

    bool         m_write_burst_en;
    int          m_cas_latency;

    static const uint32_t NUM_BANKS = 4;
    int          m_active_row[NUM_BANKS];
    sc_time      m_activate_time[NUM_BANKS];

    sc_time      m_last_refresh;
    uint32_t     m_refresh_cnt;

    int          m_burst_write;
    int          m_burst_read;
    bool         m_burst_close_row[NUM_BANKS];
    int          m_burst_offset;
};

#endif