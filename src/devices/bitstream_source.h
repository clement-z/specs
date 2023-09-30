#pragma once

#include <systemc.h>
#include <fstream>

#include <optical_output_port.h>
#include <optical_signal.h>

using namespace std;

class BitstreamSource : public sc_module {
public:
    // Set it as typedef for now (we may very well never need to change it)
    typedef unsigned long operand_type;

public:
    // Ports
    sc_port<sc_signal_in_if<bool>> p_enable;
    sc_in<bool> p_clk;
    sc_port<sc_signal_out_if<OpticalSignal>> p_out;

    // Timed ports writers
    OpticalOutputPort m_out_writer;

    // Member variables
    static unsigned int m_bits_per_value;

    vector<operand_type> m_values;
    operand_type m_current_value = 0;

    OpticalSignal m_signal_on;
    OpticalSignal m_signal_off;

    // Processes
    void runner();

    // 
    inline void setWavelength(const double &wl)
    { 
        m_signal_on.m_wavelength_id = m_signal_on.getIDFromWavelength(wl);
        m_signal_off.m_wavelength_id = m_signal_off.getIDFromWavelength(wl);
    }
    
    // 
    inline void setPower(const double &on, const double &off = 0.0)
    { 
        m_signal_on.m_field = sqrt(on);
        m_signal_off.m_field = sqrt(off);
    }

    // Constructor
    BitstreamSource(sc_module_name name)
        : sc_module(name)
        , m_out_writer("out_delayed_writer", p_out)
    {
        SC_HAS_PROCESS(BitstreamSource);

        SC_THREAD(runner);
        sensitive << p_clk.pos() << p_clk.neg();
    }

    BitstreamSource(sc_module_name name,
                     const vector<operand_type> &values,
                     const OpticalSignal &signal_on,
                     const OpticalSignal &signal_off = OpticalSignal(0))
        : sc_module(name)
        , m_out_writer("out_delayed_writer", p_out)
        , m_values(values)
        , m_current_value(values.empty() ? 0 : values[0])
        , m_signal_on(signal_on)
        , m_signal_off(signal_off)
    {
        SC_HAS_PROCESS(BitstreamSource);

        SC_THREAD(runner);
        sensitive << p_clk.pos() << p_clk.neg();
    }

    BitstreamSource(sc_module_name name,
                     const std::string &values_file,
                     const OpticalSignal &signal_on,
                     const OpticalSignal &signal_off = OpticalSignal(0))
        : sc_module(name)
        , m_out_writer("out_delayed_writer", p_out)
        , m_signal_on(signal_on)
        , m_signal_off(signal_off)
    {
        // Read values from file
        std::ifstream i(values_file);
        
        int value;
        m_values.clear();
        while ( i.good() ) {
            i >> value;
            m_values.push_back(value);
        }

        m_current_value = (m_values.empty() ? 0 : m_values[0]);

        SC_HAS_PROCESS(BitstreamSource);
        
        SC_THREAD(runner);
        sensitive << p_clk.pos() << p_clk.neg();
    }
};
