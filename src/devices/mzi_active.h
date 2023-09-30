#pragma once

#include "specs.h"
#include <systemc.h>
#include <waveguide.h>
#include <phaseshifter.h>
#include <directional_coupler.h>
#include <optical_output_port.h>
#include <optical_signal.h>

//TODO: pass element's characteristics as argument, for instance, pass an example of waveguide,
// or an example of DC such that it copies. Requires: constructors in each element to build
// by copying.

class MZIActiveBase : public spx_module {
public:
    // Member variables
    double m_ps_sens_rad_v;
    double m_length_cm;
    double m_length_ref_cm;
    double m_attenuation_dB_cm;
    double m_neff;
    double m_ng;
    double m_dc_loss_dB; // loss of the splitter/merger
    double m_ps_loss_dB; // loss of the phase shifter

    virtual void init() = 0;
    virtual void connect_submodules() = 0;

    // Member submodules
    shared_ptr<WaveguideBase> m_wg1, m_wg2, m_wg3;
    shared_ptr<DirectionalCouplerBase> m_dc1, m_dc2;
    shared_ptr<PhaseShifterBase> m_ps;

    /** Constructor for Waveguide
     *
     * @param name name of the module
     * @param m_ps_sens_rad_v sensitivity of phase shifter
     * @param m_length_cm length of the phase-shifter arm
     * @param m_length_ref_cm length of the reference arm
     * @param m_attenuation_dB_cm loss per cm of wgs
     * @param m_neff neff of wgs
     * @param m_ng ng of wgs
     * @param m_dc_loss_dB insertion loss of the y-junctions
     * @param m_ps_loss_dB insertion loss of the phase shifter
     * */
    MZIActiveBase(sc_module_name name,
                double ps_sens_rad_v = 1,
                double length_cm = 0, double length_ref_cm = 0,
                double attenuation_dB_cm = 0,
                double neff = 2.2, double ng = 2.6, double dc_loss_dB = 0,
                double ps_loss_dB = 0)
        : spx_module(name)
        , m_ps_sens_rad_v(ps_sens_rad_v)
        , m_length_cm(length_cm)
        , m_length_ref_cm(length_ref_cm)
        , m_attenuation_dB_cm(attenuation_dB_cm)
        , m_neff(neff)
        , m_ng(ng)
        , m_dc_loss_dB(dc_loss_dB)
        , m_ps_loss_dB(ps_loss_dB)
    {
    }
};

class MZIActiveUni : public MZIActiveBase {
public:
    // Ports
    /** The optical input ports. */
    spx::oa_port_in_type p_in1, p_in2;
    /** The optical output ports. */
    spx::oa_port_out_type p_out1, p_out2;
    /** The electrical input ports */
    spx::ea_port_in_type p_vin;

    // Wires
    shared_ptr<spx::oa_signal_type> m_dc1_o1, m_dc1_o2, m_dc2_i1, m_dc2_i2;
    shared_ptr<spx::oa_signal_type> m_ps_in, m_ps_out;

    virtual void init();
    virtual void connect_submodules();

    /** Constructor for Waveguide
     *
     * @param name name of the module
     * @param m_ps_sens_rad_v sensitivity of phase shifter
     * @param m_length_cm length of the phase-shifter arm
     * @param m_length_ref_cm length of the reference arm
     * @param m_attenuation_dB_cm loss per cm of wgs
     * @param m_neff neff of wgs
     * @param m_ng ng of wgs
     * @param m_dc_loss_dB insertion loss of the y-junctions
     * @param m_ps_loss_dB insertion loss of the phase shifter
     * */
    MZIActiveUni(sc_module_name name,
                 double ps_sens_rad_v = 1,
                 double length_cm = 0,
                 double length_ref_cm = 0,
                 double attenuation_dB_cm = 0,
                 double neff = 2.2,
                 double ng = 2.6,
                 double dc_loss_dB = 0,
                 double ps_loss_dB = 0)
        : MZIActiveBase(name,
                        ps_sens_rad_v,
                        length_cm,
                        length_ref_cm,
                        attenuation_dB_cm,
                        neff,
                        ng,
                        dc_loss_dB,
                        ps_loss_dB)
    {

    }
};

typedef MZIActiveUni MZIActive;

class MZIActiveBi : public MZIActiveBase {
public:
    // Ports
    /** The optical input ports. */
    spx::oa_port_in_type p0_in;
    spx::oa_port_in_type p1_in;
    spx::oa_port_in_type p2_in;
    spx::oa_port_in_type p3_in;

    /** The optical output ports. */
    spx::oa_port_out_type p0_out;
    spx::oa_port_out_type p1_out;
    spx::oa_port_out_type p2_out;
    spx::oa_port_out_type p3_out;

    /** The electrical input ports */
    spx::ea_port_in_type p_vin;

    // Wires
    shared_ptr<spx::oa_signal_type> m_dc1_p2_out, m_dc1_p3_out, m_dc2_p0_in, m_dc2_p1_in;
    shared_ptr<spx::oa_signal_type> m_dc1_p2_in, m_dc1_p3_in, m_dc2_p0_out, m_dc2_p1_out;
    shared_ptr<spx::oa_signal_type> m_ps_p0_in, m_ps_p1_in, m_ps_p0_out, m_ps_p1_out;

    virtual void init();
    virtual void connect_submodules();

    /** Constructor for Waveguide
     *
     * @param name name of the module
     * @param m_ps_sens_rad_v sensitivity of phase shifter
     * @param m_length_cm length of the phase-shifter arm
     * @param m_length_ref_cm length of the reference arm
     * @param m_attenuation_dB_cm loss per cm of wgs
     * @param m_neff neff of wgs
     * @param m_ng ng of wgs
     * @param m_dc_loss_dB insertion loss of the y-junctions
     * @param m_ps_loss_dB insertion loss of the phase shifter
     * */
    MZIActiveBi(sc_module_name name,
                 double ps_sens_rad_v = 1,
                 double length_cm = 0,
                 double length_ref_cm = 0,
                 double attenuation_dB_cm = 0,
                 double neff = 2.2,
                 double ng = 2.6,
                 double dc_loss_dB = 0,
                 double ps_loss_dB = 0)
        : MZIActiveBase(name,
                        ps_sens_rad_v,
                        length_cm,
                        length_ref_cm,
                        attenuation_dB_cm,
                        neff,
                        ng,
                        dc_loss_dB,
                        ps_loss_dB)
    {

    }
};
