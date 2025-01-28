#pragma once

#include <systemc.h>

#include "optical_output_port.h"
#include "optical_signal.h"
#include "specs.h"
#include "devices/spx_module.h"

/** The base class for the waveguide module */
class WaveguideBase : public spx_module {
public:
    // Member variables

    /** The length of the waveguide in cm. */
    double m_length_cm;
    /** The attenuation of the waveguide in dB. */
    double m_attenuation_dB;
    /** The attenuation inside the waveguide, in dB/cm */
    double m_attenuation_dB_cm;

    /** The effective index of the waveguide */
    double m_neff;
    /** The group index of the waveguide */
    double m_ng;
    /** The group velocity dispersion Dlambda of the waveguide in [s/m/m] */
    double m_D;

    /** Constructor for WaveguideBase
     *
     * @param name name of the module
     * @param length_cm length of the waveguide in cm
     * */
    WaveguideBase(sc_module_name name, double length_cm = 1e-2,
            double attenuation_dB_cm = 0, double neff = 2.2111,
            double ng = 2.2637, double D = 0)
        : spx_module(name)
        , m_length_cm(length_cm)
        , m_attenuation_dB_cm(attenuation_dB_cm)
        , m_neff(neff)
        , m_ng(ng)
        , m_D(D)
    {
        // nothing to do
    }

    inline void setLength(double length_cm) {
        if (length_cm < 0) {
            std::cerr << "Error: waveguide length < 0" << std::endl;
        }
        m_length_cm = length_cm;
    }
};


/** A unidirectional waveguide module */
class WaveguideUni : public WaveguideBase {
public:
    // Ports
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
    WaveguideUni(sc_module_name name, double length_cm = 1e-2,
            double attenuation_dB_cm = 0, double neff = 2.2111,
            double ng = 2.2637, double D = 0)
        : WaveguideBase(name, length_cm, attenuation_dB_cm, neff, ng, D)
        , m_out_writer("out_delayed_writer", p_out)
    {
        SC_HAS_PROCESS(WaveguideUni);

        SC_THREAD(on_port_in_changed);
        sensitive << p_in;
    }
};

typedef WaveguideUni Waveguide;

/** A bidirectional waveguide module */
class WaveguideBi : public WaveguideBase {
public:
    // Ports
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
    OpticalOutputPort m_p0_out_writer;
    OpticalOutputPort m_p1_out_writer;

    // Processes
    /** Main process of the module.
     *
     * It copies the input to the output, after attenuation and delay.
     *
     *   **SystemC type:** thread
     *
     *   **Sensitivity list:** p0_in or p1_in
     * */
    void on_p0_in_changed();
    void on_p1_in_changed();

    /** Constructor for Waveguide
     *
     * @param name name of the module
     * @param length_cm length of the waveguide in cm
     * */
    WaveguideBi(sc_module_name name, double length_cm = 1e-2,
            double attenuation_dB_cm = 0, double neff = 2.2111,
            double ng = 2.2637, double D = 0)
        : WaveguideBase(name, length_cm, attenuation_dB_cm, neff, ng, D)
        , m_p0_out_writer("p0_out_delayed_writer", p0_out)
        , m_p1_out_writer("p1_out_delayed_writer", p1_out)
    {
        SC_HAS_PROCESS(WaveguideBi);

        SC_THREAD(on_p0_in_changed);
        sensitive << p0_in;
        SC_THREAD(on_p1_in_changed);
        sensitive << p1_in;
    }
};
