#ifndef TB_MEM_TEST_H
#define TB_MEM_TEST_H

#include <systemc.h>
#include "tb_driver_api.h"
#include "tb_memory.h"

//-------------------------------------------------------------
// tb_mem_test: Memory tester (using driver)
//-------------------------------------------------------------
class tb_mem_test: public sc_module, public tb_memory
{
public:
    //-------------------------------------------------------------
    // Interface I/O
    //-------------------------------------------------------------
    sc_in <bool>             clk_in;
    sc_in <bool>             rst_in;

    //-------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------
    SC_HAS_PROCESS(tb_mem_test);
    tb_mem_test(sc_module_name name, tb_driver_api *iface, int max_length): sc_module(name)
                                                                          , m_enabled("enabled", 0)
                                                                          , m_completed("completed", 0)
    { 
        SC_CTHREAD(process, clk_in.pos());
        m_driver     = iface;
        m_max_length = max_length;
    }

    // API
	void start(int iterations = -1) 
    {
    	m_iterations = iterations;
    	m_enabled.post(); 
    }

    void wait_complete(void) { m_completed.wait(); }

    void trace_access(bool en)
    {
        for (int i=0;i<TB_MEM_MAX_REGIONS;i++)
            if (m_mem[i])
                m_mem[i]->trace_access(en);
    }

    // Internal
protected:
    uint32_t     get_mem_address(int size, int alignment);
    void         process(void);

protected:
    sc_semaphore     m_enabled;
    sc_semaphore     m_completed;
	tb_driver_api *  m_driver;
	sc_signal <int>  m_iterations;
	int              m_max_length;
};

#endif