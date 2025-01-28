#pragma once

#include <vector>

#include <systemc.h>

#include "devices/spx_module.h"
#include "optical_output_port.h"
#include "optical_signal.h"

#include "specs.h"

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
    double m_Tc = 0; /* Transmission (power) in initial (fully crystalline) state */
    double m_Ta = 1; /* Transmission (power) in final (fully amorphous) state */
    double m_meltEnergy; /* Programming threshold*/
    int m_nStates; /* Total number of states */
    double m_influence_time_ns = 1; /* Duration for which energy is maintained (hard threshold for representing thermal losses)*/
    double m_speed = 3; /* Parameter affecting the transmission curve and how fast transmission saturates */

    int m_stateCurrent = 0; /* Current state */
    double m_Tcurrent = 0; /* Current transmission (power) */
    double m_Tcurrent_field = 0; /* Current transmission (field) */

    /** Current information about the optical input, before any attenuation and phase shift. */
    double m_last_pulse_power = 0;
    std::map<uint32_t,OpticalSignal::field_type> m_memory_in;

    // Processes
    void on_input_changed();

    // Member functions
    bool phase_change(const vector<pulse_sample_t> &vec, const bool &local);
    void update_transmission_local();

    // Constructor
    PCMElement(sc_module_name name,
               double meltEnergy = 0,
               int nStates = 0,
               int state = 0,
               double Tc = 0,
               double Ta = 0,
               double influence_window_ns = 1,
               double speed = 3)
        : spx_module(name)
        , m_out_writer("out_delayed_writer", p_out)
        , m_Tc(Tc)
        , m_Ta(Ta)
        , m_meltEnergy(meltEnergy)
        , m_nStates(nStates)
        , m_influence_time_ns(influence_window_ns)
        , m_speed(speed)
        , m_stateCurrent(state)
    {
        SC_HAS_PROCESS(PCMElement);

        SC_THREAD(on_input_changed);
        sensitive << p_in;
    }
};
