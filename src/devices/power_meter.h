#pragma once

#include <systemc.h>
#include <fstream>
#include <random>

#include <optical_output_port.h>
#include <optical_signal.h>
#include "specs.h"
#include "spx_module.h"

/* Power meter (DC component only) */
class PowerMeter : public spx_module {
public:
    // Ports
    spx::oa_port_in_type p_in;
    double m_cur_power;

    // Input memory for multi-wavelength purposes
    std::map<uint32_t,OpticalSignal::field_type> m_memory_in;

    // Processes
    void on_port_in_changed();

    virtual void trace(sc_trace_file *Tf) const;

    // Constructor
    PowerMeter(sc_module_name name)
        : spx_module(name)
    {
        SC_HAS_PROCESS(PowerMeter);

        SC_THREAD(on_port_in_changed);
        sensitive << p_in;
    }
};
