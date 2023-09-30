#pragma once

#include "specs.h"
#include <systemc.h>

#include <spx_module.h>
#include <optical_output_port.h>
#include <optical_signal.h>

/** An electrically-controllable phase shifter. */
class PhaseShifterBase : public spx_module {
public:
    // Member variables
    /** The phaseshift applied to the pulse, in rad. */
    double m_phaseshift_rad = 0;

    /** The attenuation of the device in dB. */
    double m_attenuation_dB;

    /** The responsivity of the phase-shifter in rad/V. */
    double m_sensitivity = 1;

    /** Constructor for PhaseShifter
     *
     * @param name name of the module
     * */
    PhaseShifterBase(sc_module_name name, double attenuation_dB = 0)
        : spx_module(name)
        , m_phaseshift_rad(0)
        , m_attenuation_dB(attenuation_dB)
    {}
};

class PhaseShifterUni : public PhaseShifterBase {
public:
    // Ports
    /** The electrical input port. */
    spx::ea_port_in_type p_vin;
    /** The optical input port. */
    spx::oa_port_in_type p_in;
    /** The optical output port. */
    spx::oa_port_out_type p_out;

    // Timed ports writers
    /** Timed writer to the output port.
     *
     * @sa DelayedWriter
     * */
    OpticalOutputPort m_out_writer;

    /** Memory of the current input value for all wavelengths. */
    std::map<uint32_t,OpticalSignal::field_type> m_memory_in;

    // Processes
    /** Main process of the module.
     *
     * It copies the input to the output, after attenuation and delay.
     * Will use the current m_phaseshift_rad at that moment.
     *
     *   **SystemC type:** thread
     *
     *   **Sensitivity list:** p_in
     * */
    void on_port_in_changed();

    /** Auxiliary process of the module.
     *
     * Updates @param m_phaseshift_rad and creates a new event
     * using the current optical input, sending it to the output
     * after zero time.
     *
     *   **SystemC type:** thread
     *
     *   **Sensitivity list:** v_in
     * */
    void on_port_vin_changed();

    /** Constructor for PhaseShifter
     *
     * @param name name of the module
     * */
    PhaseShifterUni(sc_module_name name, double attenuation_dB = 0)
        : PhaseShifterBase(name, attenuation_dB)
        , m_out_writer("out_delayed_writer", p_out)
    {
        SC_HAS_PROCESS(PhaseShifterUni);

        SC_THREAD(on_port_in_changed);
        sensitive << p_in;
        SC_THREAD(on_port_vin_changed);
        sensitive << p_vin;
    }
};

typedef PhaseShifterUni PhaseShifter;

class PhaseShifterBi : public PhaseShifterBase {
public:
    // Ports
    /** The electrical input port. */
    spx::ea_port_in_type p_vin;

    /** The optical input ports. */
    spx::oa_port_in_type p0_in;
    spx::oa_port_in_type p1_in;

    /** The optical output ports. */
    spx::oa_port_out_type p0_out;
    spx::oa_port_out_type p1_out;

    // Timed ports writers
    /** Timed writer to the output port.
     *
     * @sa DelayedWriter
     * */
    OpticalOutputPort m_p0_writer;
    OpticalOutputPort m_p1_writer;

    /** Memory of the current input value for all wavelengths. */
    std::map<uint32_t,OpticalSignal::field_type> m_memory_p0;
    std::map<uint32_t,OpticalSignal::field_type> m_memory_p1;

    // Processes
    /** Main process of the module.
     *
     * It copies the input to the output, after attenuation and delay.
     * Will use the current m_phaseshift_rad at that moment.
     *
     *   **SystemC type:** thread
     *
     *   **Sensitivity list:** p0_in
     * */
    void on_p0_in_changed();
    void on_p1_in_changed();

    /** Auxiliary process of the module.
     *
     * Updates @param m_phaseshift_rad and creates a new event
     * using the current optical input, sending it to the output
     * after zero time.
     *
     *   **SystemC type:** thread
     *
     *   **Sensitivity list:** v_in
     * */
    void on_port_vin_changed();

    /** Constructor for PhaseShifter
     *
     * @param name name of the module
     * */
    PhaseShifterBi(sc_module_name name, double attenuation_dB = 0)
        : PhaseShifterBase(name, attenuation_dB)
        , m_p0_writer("p0_out_delayed_writer", p0_out)
        , m_p1_writer("p1_out_delayed_writer", p1_out)
    {
        SC_HAS_PROCESS(PhaseShifterBi);

        SC_THREAD(on_p0_in_changed);
        sensitive << p0_in;
        SC_THREAD(on_p1_in_changed);
        sensitive << p1_in;
        SC_THREAD(on_port_vin_changed);
        sensitive << p_vin;
    }
};
