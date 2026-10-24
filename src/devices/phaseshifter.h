#pragma once

#include "specs.h"
#include <systemc.h>

#include "devices/spx_module.h"
#include "optical_output_port.h"
#include "optical_signal.h"
#include "utils/wavelength_map.h"
#include "utils/transmission_cache.h"

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

protected:
    // F-15: field transmission 10^(-dB/20), computed once before the simulation (the device
    // threads computed it when they started)
    double m_transmission_field = 1;
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
     * @sa OpticalOutputPort
     * */
    OpticalOutputPort m_out_writer;

    /** Memory of the current input value for all wavelengths. */
    WavelengthMap<OpticalSignal::field_type> m_memory_in;

    /** F-15: polar() only when the phase changes (on_port_in_changed()) */
    PolarCache m_in_transmission;

    // Processes
    /** Main process of the module.
     *
     * It copies the input to the output, after attenuation and delay.
     * Will use the current m_phaseshift_rad at that moment.
     *
     *   **SystemC type:** method
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
     *   **SystemC type:** method
     *
     *   **Sensitivity list:** v_in
     * */
    void on_port_vin_changed();
    /** --vci only: prints the parameters at initialization (a thread, so that the devices
     * print in the order their threads used to) */
    void print_initialization();

    virtual void start_of_simulation();

    /** Constructor for PhaseShifter
     *
     * @param name name of the module
     * */
    PhaseShifterUni(sc_module_name name, double attenuation_dB = 0)
        : PhaseShifterBase(name, attenuation_dB)
        , m_out_writer("out_delayed_writer", p_out)
    {
        SC_METHOD(on_port_in_changed);
        sensitive << p_in;
        dont_initialize();
        if (specsGlobalConfig.verbose_component_initialization)
            SC_THREAD(print_initialization);
        SC_METHOD(on_port_vin_changed);
        sensitive << p_vin;
        dont_initialize();
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
     * @sa OpticalOutputPort
     * */
    OpticalOutputPort m_p0_writer;
    OpticalOutputPort m_p1_writer;

    /** Memory of the current input value for all wavelengths. */
    WavelengthMap<OpticalSignal::field_type> m_memory_p0;
    WavelengthMap<OpticalSignal::field_type> m_memory_p1;

    /** F-15: polar() only when the phase changes (on_p0_in_changed(), on_p1_in_changed()) */
    PolarCache m_p0_transmission;
    PolarCache m_p1_transmission;

    // Processes
    /** Main process of the module.
     *
     * It copies the input to the output, after attenuation and delay.
     * Will use the current m_phaseshift_rad at that moment.
     *
     *   **SystemC type:** method
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
     *   **SystemC type:** method
     *
     *   **Sensitivity list:** v_in
     * */
    void on_port_vin_changed();
    /** --vci only: prints the parameters at initialization (a thread, so that the devices
     * print in the order their threads used to) */
    void print_initialization();

    virtual void start_of_simulation();

    /** Constructor for PhaseShifter
     *
     * @param name name of the module
     * */
    PhaseShifterBi(sc_module_name name, double attenuation_dB = 0)
        : PhaseShifterBase(name, attenuation_dB)
        , m_p0_writer("p0_out_delayed_writer", p0_out)
        , m_p1_writer("p1_out_delayed_writer", p1_out)
    {
        SC_METHOD(on_p0_in_changed);
        sensitive << p0_in;
        dont_initialize();
        if (specsGlobalConfig.verbose_component_initialization)
            SC_THREAD(print_initialization);
        SC_METHOD(on_p1_in_changed);
        sensitive << p1_in;
        dont_initialize();
        SC_METHOD(on_port_vin_changed);
        sensitive << p_vin;
        dont_initialize();
    }
};
