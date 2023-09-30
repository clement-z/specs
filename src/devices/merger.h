#pragma once

#include <systemc.h>
#include <map>

#include <spx_module.h>
#include <optical_output_port.h>
#include <optical_signal.h>

#include "specs.h"
#include "spx_module.h"

// A symmetric no-delay merger
class Merger : public spx_module {
public:
    // Ports
    spx::oa_port_in_type p_in1;
    spx::oa_port_in_type p_in2;
    spx::oa_port_out_type p_out;

    // Timed ports writers
    OpticalOutputPort m_out_writer;

    // Member variables
    double m_attenuation_dB;

    // Memory for multi-wavelength purposes
    // maybe with vector it has better performance
    std::map<uint32_t,OpticalSignal::field_type> m_memory_in1;
    std::map<uint32_t,OpticalSignal::field_type> m_memory_in2;

    // Processes
    void on_port_in1_changed();
    void on_port_in2_changed();

    // Constructor
    Merger(sc_module_name name,
           double attenuation_dB = 0)
        : spx_module(name)
        , m_out_writer("out_delayed_writer", p_out)
        , m_attenuation_dB(attenuation_dB)
    {
        SC_HAS_PROCESS(Merger);

        SC_THREAD(on_port_in1_changed);
        sensitive << p_in1;

        SC_THREAD(on_port_in2_changed);
        sensitive << p_in2;
    }
};
