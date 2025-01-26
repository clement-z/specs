#pragma once

#include <systemc.h>

#include "devices/waveguide.h"
#include "devices/directional_coupler.h"
#include "optical_output_port.h"
#include "optical_signal.h"

/** A unidirectional waveguide module. */
class Ring : public sc_module {
public:
    // Ports
    /** The optical input port. */
    sc_port<sc_signal_in_if<OpticalSignal>> p_in;
    /** The optical output port. */
    sc_port<sc_signal_out_if<OpticalSignal>> p_out;

    // Timed ports writers
    /** Timed writer to the output port.
     *
     * @sa DelayedWriter
     * */
    OpticalOutputPort m_out_writer;

    sc_signal<OpticalSignal> sig_in;
    sc_signal<OpticalSignal> sig_internal_0;
    sc_signal<OpticalSignal> sig_internal_1;
    sc_signal<OpticalSignal> sig_out;
    DirectionalCoupler dc;
    Waveguide wg;

    // Member variables
    /** The attenuation inside the waveguide, in dB/cm. */
    double m_attenuation_dB_cm = 0.2;
    /** The effective index of the waveguide */
    double m_neff = 2.2111;
    /** The group index of the waveguide */
    double m_ng = 2.2637;

    /** The coupling from waveguide to ring, in dB. */
    double m_cross_power_dB = 10*std::log10(0.5) - 0.15;
    double m_through_power_dB = 10*std::log10(0.5) - 0.15;
    double m_cross_phase_rad = M_PI / 2;
    double m_through_phase_rad = M_PI / 2;

    /** The radius of the ring in um. */
    double m_radius_um = 0;

    // Processes
    /** Main process of the module.
     *
     * It copies the input to the output, after attenuation and delay.
     *
     *   **SystemC type:** thread
     *
     *   **Sensitivity list:** p_in
     * */
    void on_port_in_changed();

    /** Constructor for Waveguide
     *
     * @param name name of the module
     * @param length_cm length of the waveguide in cm
     * */
    Ring(sc_module_name name, double length_cm = 30e-6)
        : sc_module(name)
        , m_out_writer("out_delayed_writer", p_out)
        , dc((std::string(name) + ".DC1").c_str())
        , wg((std::string(name) + ".WG1").c_str())
    {
        dc.p_in1(sig_in);
        dc.p_in2(sig_internal_1);
        dc.p_out1(sig_out);
        dc.p_out2(sig_internal_0);

        wg.p_in(sig_internal_0);
        wg.p_out(sig_internal_1);

        p_in(sig_in);
        p_out(sig_out);

        setLength(length_cm);
        
        SC_HAS_PROCESS(Waveguide);

        SC_THREAD(on_port_in_changed);
        sensitive << p_in;
    }

    void setRadius(double radius_um) {
        wg.setLength(2 * M_PI * radius_um * 1e-4);
        m_radius_um = radius_um;
    }

    void setLength(double length_cm) {
        setRadius(length_cm * 1e4 / 2 / M_PI);
    }
};
