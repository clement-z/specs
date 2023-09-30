#pragma once

#include <vector>

#include <systemc.h>

#include <spx_module.h>
#include <optical_output_port.h>
#include <optical_signal.h>

#include "specs.h"
#include "spx_module.h"

class PCMElement : public spx_module {
public:
    // Ports
    spx::oa_port_in_type p_in;
    spx::oa_port_out_type p_out;

    // Timed ports writers
    OpticalOutputPort m_out_writer;

    // Auxiliary struct - stores tuples of value-time
    typedef pair<double,double> pulse_sample_t;

    // Member variables
    double m_meltEnergy;
    int m_state;
    int m_nStates;
    double m_transmission = 1;
    double influence_time_ns = 1;

    /** Current information about the optical input, before any attenuation and phase shift. */
    double m_last_pulse_power;
    std::map<uint32_t,OpticalSignal::field_type> m_memory_in;


    // Processes
    void on_input_changed();
    bool phase_change(const vector<pulse_sample_t> &vec, const bool &local);
    void update_transmission_local();


    // Constructor
    PCMElement(sc_module_name name,
               double meltEnergy = 0,
               int nStates = 0,
               int state = 0)
        : spx_module(name)
        , m_out_writer("out_delayed_writer", p_out)
        , m_meltEnergy(meltEnergy)
        , m_state(state)
        , m_nStates(nStates)
    {
        SC_HAS_PROCESS(PCMElement);

        SC_THREAD(on_input_changed);
        sensitive << p_in;
    }
};
