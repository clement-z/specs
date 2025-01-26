#include "devices/mzi.h"

#define __modname(SUFFIX, IDX) \
    ((""s + this->name() + "_" + SUFFIX + to_string(IDX)).c_str())

using namespace std;

void MZI::init()
{
    DC1.reset();
    DC2.reset();
    PS1.reset();
    PS2.reset();
    WG1.reset();
    WG2.reset();

    // Creating instances
    WG1 = make_unique<Waveguide>((string(name()) + "/WG1").c_str(),
            m_length_cm, m_attenuation_dB_cm, m_neff, m_ng);
    WG2 = make_unique<Waveguide>((string(name()) + "/WG2").c_str(),
            m_length_cm, m_attenuation_dB_cm, m_neff, m_ng);
    DC1 = make_unique<DirectionalCoupler>((string(name()) + "/DC1").c_str(), 0.5, m_attenuation_coupler_dB);
    DC2 = make_unique<DirectionalCoupler>((string(name()) + "/DC2").c_str(), 0.5, m_attenuation_coupler_dB);
    PS1 = make_unique<PhaseShifter>((string(name()) + "/PS1").c_str(), m_attenuation_ps_dB);
    PS2 = make_unique<PhaseShifter>((string(name()) + "/PS2").c_str(), m_attenuation_ps_dB);

    // Connecting

    PS1->p_in(p_in1);
    PS1->p_out(w1);
    PS1->p_vin(p_vphi);

    DC1->p_in1(w1);
    DC1->p_in2(p_in2);
    DC1->p_out1(w2);
    DC1->p_out2(w5);

    // Upper branch
    PS2->p_in(w2);
    PS2->p_out(w3);
    PS2->p_vin(p_vtheta);

    WG1->p_in(w3);
    WG1->p_out(w4);

    // Bottom branch
    WG2->p_in(w5);
    WG2->p_out(w6);

    // End
    DC2->p_in1(w4);
    DC2->p_in2(w6);
    DC2->p_out1(p_out1);
    DC2->p_out2(p_out2);
}