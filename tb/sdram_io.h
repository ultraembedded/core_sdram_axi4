#ifndef SDRAM_IO_H
#define SDRAM_IO_H

#include <systemc.h>

//----------------------------------------------------------------
// Interface (master)
//----------------------------------------------------------------
class sdram_io_master
{
public:
    // Members
    sc_uint <1> CLK;
    sc_uint <1> CKE;
    sc_uint <1> CS;
    sc_uint <1> RAS;
    sc_uint <1> CAS;
    sc_uint <1> WE;
    sc_uint <2> DQM;
    sc_uint <13> ADDR;
    sc_uint <2> BA;
    sc_uint <16> DATA_OUTPUT;
    sc_uint <1> DATA_OUT_EN;

    // Construction
    sdram_io_master() { init(); }

    void init(void)
    {
        CLK = 0;
        CKE = 0;
        CS = 0;
        RAS = 0;
        CAS = 0;
        WE = 0;
        DQM = 0;
        ADDR = 0;
        BA = 0;
        DATA_OUTPUT = 0;
        DATA_OUT_EN = 0;
    }

    bool operator == (const sdram_io_master & v) const
    {
        bool eq = true;
        eq &= (CLK == v.CLK);
        eq &= (CKE == v.CKE);
        eq &= (CS == v.CS);
        eq &= (RAS == v.RAS);
        eq &= (CAS == v.CAS);
        eq &= (WE == v.WE);
        eq &= (DQM == v.DQM);
        eq &= (ADDR == v.ADDR);
        eq &= (BA == v.BA);
        eq &= (DATA_OUTPUT == v.DATA_OUTPUT);
        eq &= (DATA_OUT_EN == v.DATA_OUT_EN);
        return eq;
    }

    friend void sc_trace(sc_trace_file *tf, const sdram_io_master & v, const std::string & path)
    {
        sc_trace(tf,v.CLK, path + "/clk");
        sc_trace(tf,v.CKE, path + "/cke");
        sc_trace(tf,v.CS, path + "/cs");
        sc_trace(tf,v.RAS, path + "/ras");
        sc_trace(tf,v.CAS, path + "/cas");
        sc_trace(tf,v.WE, path + "/we");
        sc_trace(tf,v.DQM, path + "/dqm");
        sc_trace(tf,v.ADDR, path + "/addr");
        sc_trace(tf,v.BA, path + "/ba");
        sc_trace(tf,v.DATA_OUTPUT, path + "/data_output");
        sc_trace(tf,v.DATA_OUT_EN, path + "/data_out_en");
    }

    friend ostream& operator << (ostream& os, sdram_io_master const & v)
    {
        os << hex << "CLK: " << v.CLK << " ";
        os << hex << "CKE: " << v.CKE << " ";
        os << hex << "CS: " << v.CS << " ";
        os << hex << "RAS: " << v.RAS << " ";
        os << hex << "CAS: " << v.CAS << " ";
        os << hex << "WE: " << v.WE << " ";
        os << hex << "DQM: " << v.DQM << " ";
        os << hex << "ADDR: " << v.ADDR << " ";
        os << hex << "BA: " << v.BA << " ";
        os << hex << "DATA_OUTPUT: " << v.DATA_OUTPUT << " ";
        os << hex << "DATA_OUT_EN: " << v.DATA_OUT_EN << " ";
        return os;
    }

    friend istream& operator >> ( istream& is, sdram_io_master & val)
    {
        // Not implemented
        return is;
    }
};

#define MEMBER_COPY_SDRAM_IO_MASTER(s,d) do { \
    s.CLK = d.CLK; \
    s.CKE = d.CKE; \
    s.CS = d.CS; \
    s.RAS = d.RAS; \
    s.CAS = d.CAS; \
    s.WE = d.WE; \
    s.DQM = d.DQM; \
    s.ADDR = d.ADDR; \
    s.BA = d.BA; \
    s.DATA_OUTPUT = d.DATA_OUTPUT; \
    s.DATA_OUT_EN = d.DATA_OUT_EN; \
    } while (0)

//----------------------------------------------------------------
// Interface (slave)
//----------------------------------------------------------------
class sdram_io_slave
{
public:
    // Members
    sc_uint <16> DATA_INPUT;

    // Construction
    sdram_io_slave() { init(); }

    void init(void)
    {
        DATA_INPUT = 0;
    }

    bool operator == (const sdram_io_slave & v) const
    {
        bool eq = true;
        eq &= (DATA_INPUT == v.DATA_INPUT);
        return eq;
    }

    friend void sc_trace(sc_trace_file *tf, const sdram_io_slave & v, const std::string & path)
    {
        sc_trace(tf,v.DATA_INPUT, path + "/data_input");
    }

    friend ostream& operator << (ostream& os, sdram_io_slave const & v)
    {
        os << hex << "DATA_INPUT: " << v.DATA_INPUT << " ";
        return os;
    }

    friend istream& operator >> ( istream& is, sdram_io_slave & val)
    {
        // Not implemented
        return is;
    }
};

#define MEMBER_COPY_SDRAM_IO_SLAVE(s,d) do { \
    s.DATA_INPUT = d.DATA_INPUT; \
    } while (0)


#endif
