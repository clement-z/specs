#pragma once

#include <systemc.h>
#include <map>

#include "devices/spx_module.h"
#include "optical_output_port.h"
#include "optical_signal.h"

#include "specs.h"

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

    // F-15: field transmission, computed once before the simulation (the device threads
    // computed it when they started)
    double m_transmission = 1;

    // Processes (methods)
    void on_port_in1_changed();
    void on_port_in2_changed();
    // --vci only: prints the parameters at initialization (a thread, so that the devices print
    // in the order their threads used to)
    void print_initialization();

    virtual void start_of_simulation();

    // Constructor
    Merger(sc_module_name name,
           double attenuation_dB = 0)
        : spx_module(name)
        , m_out_writer("out_delayed_writer", p_out)
        , m_attenuation_dB(attenuation_dB)
    {
        SC_METHOD(on_port_in1_changed);
        sensitive << p_in1;
        dont_initialize();
        if (specsGlobalConfig.verbose_component_initialization)
            SC_THREAD(print_initialization);

        SC_METHOD(on_port_in2_changed);
        sensitive << p_in2;
        dont_initialize();
    }
};
