#pragma once

#include <systemc.h>

#include <optical_output_port.h>
#include <optical_signal.h>
#include "specs.h"
#include "spx_module.h"

#include <directional_coupler.h>
#include <phaseshifter.h>
#include <waveguide.h>

/** A unidirectional waveguide module. */
class MZI : public spx_module {
public:
    // Ports
    /** The optical input ports. */
    spx::oa_port_in_type p_in1, p_in2;
    /** The optical output ports. */
    spx::oa_port_out_type p_out1, p_out2;
    /** The electrical input ports */
    spx::ea_port_in_type p_vphi, p_vtheta;

    // Member variables

    /** Waveguide-related. */
    double m_length_cm;
    double m_attenuation_dB_cm;
    double m_neff;
    double m_ng;

    /** DC-related */
    double m_attenuation_coupler_dB;

    /** PhaseShifter-related */
    double m_attenuation_ps_dB;

    // Member submodules and wires
    spx::oa_signal_type w1, w2, w3, w4, w5, w6;

    unique_ptr<PhaseShifter> PS1, PS2;
    unique_ptr<Waveguide> WG1, WG2, WG3;
    unique_ptr<DirectionalCoupler> DC1,DC2;

    virtual void init();

    /** Constructor for MZI
     *
     * @param name name of the module
     * @param length_cm internal branch length in cm
     * */
    MZI(sc_module_name name, const double &length_cm = 1e-2,
            const double &attenuation_wg_dB_cm = 0, const double &attenuation_coupler_dB = 0,
            const double &attenuation_ps_dB = 0,
            const double &neff = 2.2111, const double &ng = 2.2637)
        : spx_module(name)
        , m_length_cm(length_cm)
        , m_attenuation_dB_cm(attenuation_wg_dB_cm)
        , m_neff(neff)
        , m_ng(ng)
        , m_attenuation_coupler_dB(attenuation_coupler_dB)
        , m_attenuation_ps_dB(attenuation_ps_dB)
    {

    }
};
