#include "devices/mzi_active.h"

void MZIActiveUni::init()
{
    m_dc1_o1 = make_shared<spx::oa_signal_type>((string(name()) + "/DC1_out1").c_str());
    m_dc1_o2 = make_shared<spx::oa_signal_type>((string(name()) + "/DC1_out2").c_str());
    m_dc2_i1 = make_shared<spx::oa_signal_type>((string(name()) + "/DC2_in1").c_str());
    m_dc2_i2 = make_shared<spx::oa_signal_type>((string(name()) + "/DC2_in2").c_str());
    m_ps_in = make_shared<spx::oa_signal_type>((string(name()) + "/PS1_in").c_str());
    m_ps_out = make_shared<spx::oa_signal_type>((string(name()) + "/PS1_out").c_str());

    m_wg1 = make_shared<WaveguideUni>((string(name()) + "/WG1").c_str());
    m_wg2 = make_shared<WaveguideUni>((string(name()) + "/WG2").c_str());
    m_wg3 = make_shared<WaveguideUni>((string(name()) + "/WG3").c_str());
    m_dc1 = make_shared<DirectionalCouplerUni>((string(name()) + "/DC1").c_str());
    m_dc2 = make_shared<DirectionalCouplerUni>((string(name()) + "/DC2").c_str());
    m_ps = make_shared<PhaseShifterUni>((string(name()) + "/PS1").c_str());

    connect_submodules();
}

void MZIActiveUni::connect_submodules()
{
    // Parametrizing the instances

    // cast as unidirectional devices
    WaveguideUni * const wg1 = dynamic_cast<WaveguideUni *>(m_wg1.get());
    WaveguideUni * const wg2 = dynamic_cast<WaveguideUni *>(m_wg2.get());
    WaveguideUni * const wg3 = dynamic_cast<WaveguideUni *>(m_wg3.get());
    DirectionalCouplerUni * const dc1 = dynamic_cast<DirectionalCouplerUni *>(m_dc1.get());
    DirectionalCouplerUni * const dc2 = dynamic_cast<DirectionalCouplerUni *>(m_dc2.get());
    PhaseShifterUni * const ps = dynamic_cast<PhaseShifterUni *>(m_ps.get());


    // Top waveguides are divided by two to preserve
    // the same length in the arms
    wg1->m_length_cm = m_length_cm/2;
    wg1->m_attenuation_dB_cm = m_attenuation_dB_cm;
    wg1->m_neff = m_neff;
    wg1->m_ng = m_ng;

    wg2->m_length_cm = m_length_cm/2;
    wg2->m_attenuation_dB_cm = m_attenuation_dB_cm;
    wg2->m_neff = m_neff;
    wg2->m_ng = m_ng;

    wg3->m_length_cm = m_length_ref_cm;
    wg3->m_attenuation_dB_cm = m_attenuation_dB_cm;
    wg3->m_neff = m_neff;
    wg3->m_ng = m_ng;

    dc1->m_dc_loss = m_dc_loss_dB;
    dc2->m_dc_loss = m_dc_loss_dB;

    ps->m_sensitivity = m_ps_sens_rad_v;
    ps->m_attenuation_dB = m_ps_loss_dB;

    // Connecting the blocks

    dc1->p_in1(p_in1);
    dc1->p_in2(p_in2);
    dc1->p_out1(*m_dc1_o1);
    dc1->p_out2(*m_dc1_o2);

    // Top arm
    wg1->p_in(*m_dc1_o1);
    wg1->p_out(*m_ps_in);

    ps->p_in(*m_ps_in);
    ps->p_out(*m_ps_out);
    ps->p_vin(p_vin);

    wg2->p_in(*m_ps_out);
    wg2->p_out(*m_dc2_i1);

    // Bot arm
    wg3->p_in(*m_dc1_o2);
    wg3->p_out(*m_dc2_i2);

    dc2->p_in1(*m_dc2_i1);
    dc2->p_in2(*m_dc2_i2);
    dc2->p_out1(p_out1);
    dc2->p_out2(p_out2);
}

void MZIActiveBi::init()
{
    // forward
    m_dc1_p2_out = make_shared<spx::oa_signal_type>((string(name()) + "/DC1_out1_0").c_str());
    m_dc1_p3_out = make_shared<spx::oa_signal_type>((string(name()) + "/DC1_out2_0").c_str());
    m_dc2_p0_in = make_shared<spx::oa_signal_type>((string(name()) + "/DC2_in1_0").c_str());
    m_dc2_p1_in = make_shared<spx::oa_signal_type>((string(name()) + "/DC2_in2_0").c_str());
    m_ps_p0_in = make_shared<spx::oa_signal_type>((string(name()) + "/PS1_in_0").c_str());
    m_ps_p1_out = make_shared<spx::oa_signal_type>((string(name()) + "/PS1_out_0").c_str());

    // backward
    m_dc1_p2_in = make_shared<spx::oa_signal_type>((string(name()) + "/DC1_out1_1").c_str());
    m_dc1_p3_in = make_shared<spx::oa_signal_type>((string(name()) + "/DC1_out2_1").c_str());
    m_dc2_p0_out = make_shared<spx::oa_signal_type>((string(name()) + "/DC2_in1_1").c_str());
    m_dc2_p1_out = make_shared<spx::oa_signal_type>((string(name()) + "/DC1_in2_1").c_str());
    m_ps_p0_out = make_shared<spx::oa_signal_type>((string(name()) + "/PS1_in_1").c_str());
    m_ps_p1_in = make_shared<spx::oa_signal_type>((string(name()) + "/PS1_out_1").c_str());

    // devices
    m_wg1 = make_shared<WaveguideBi>((string(name()) + "/WG1").c_str());
    m_wg2 = make_shared<WaveguideBi>((string(name()) + "/WG2").c_str());
    m_wg3 = make_shared<WaveguideBi>((string(name()) + "/WG3").c_str());
    m_dc1 = make_shared<DirectionalCouplerBi>((string(name()) + "/DC1").c_str());
    m_dc2 = make_shared<DirectionalCouplerBi>((string(name()) + "/DC2").c_str());
    m_ps = make_shared<PhaseShifterBi>((string(name()) + "/PS1").c_str());

    connect_submodules();
}

void MZIActiveBi::connect_submodules()
{
    // Parametrizing the instances

    // cast as unidirectional devices
    WaveguideBi * const wg1 = dynamic_cast<WaveguideBi *>(m_wg1.get());
    WaveguideBi * const wg2 = dynamic_cast<WaveguideBi *>(m_wg2.get());
    WaveguideBi * const wg3 = dynamic_cast<WaveguideBi *>(m_wg3.get());
    DirectionalCouplerBi * const dc1 = dynamic_cast<DirectionalCouplerBi *>(m_dc1.get());
    DirectionalCouplerBi * const dc2 = dynamic_cast<DirectionalCouplerBi *>(m_dc2.get());
    PhaseShifterBi * const ps = dynamic_cast<PhaseShifterBi *>(m_ps.get());


    // Top waveguides are divided by two to preserve
    // the same length in the arms
    wg1->m_length_cm = m_length_cm/2;
    wg1->m_attenuation_dB_cm = m_attenuation_dB_cm;
    wg1->m_neff = m_neff;
    wg1->m_ng = m_ng;

    wg2->m_length_cm = m_length_cm/2;
    wg2->m_attenuation_dB_cm = m_attenuation_dB_cm;
    wg2->m_neff = m_neff;
    wg2->m_ng = m_ng;

    wg3->m_length_cm = m_length_ref_cm;
    wg3->m_attenuation_dB_cm = m_attenuation_dB_cm;
    wg3->m_neff = m_neff;
    wg3->m_ng = m_ng;

    dc1->m_dc_loss = m_dc_loss_dB;
    dc2->m_dc_loss = m_dc_loss_dB;

    ps->m_sensitivity = m_ps_sens_rad_v;
    ps->m_attenuation_dB = m_ps_loss_dB;

    // Connecting the blocks

    { // forward
        dc1->p0_in(p0_in);
        dc1->p1_in(p1_in);
        dc1->p2_out(*m_dc1_p2_out);
        dc1->p3_out(*m_dc1_p3_out);

        // Top arm
        wg1->p0_in(*m_dc1_p2_out);
        wg1->p1_out(*m_ps_p0_in);

        ps->p0_in(*m_ps_p0_in);
        ps->p1_out(*m_ps_p1_out);
        ps->p_vin(p_vin);

        wg2->p0_in(*m_ps_p1_out);
        wg2->p1_out(*m_dc2_p0_in);

        // Bot arm
        wg3->p0_in(*m_dc1_p3_out);
        wg3->p1_out(*m_ps_p1_out);

        dc2->p0_in(*m_dc2_p0_in);
        dc2->p1_in(*m_ps_p1_out);
        dc2->p2_out(p2_out);
        dc2->p3_out(p3_out);
    }

    { // backward
        dc1->p0_out(p0_out);
        dc1->p1_out(p1_out);
        dc1->p2_in(*m_dc1_p2_in);
        dc1->p3_in(*m_dc1_p3_in);

        // Top arm
        wg1->p0_out(*m_dc1_p2_in);
        wg1->p1_in(*m_ps_p0_out);

        ps->p0_out(*m_ps_p0_out);
        ps->p1_in(*m_ps_p1_in);
        ps->p_vin(p_vin);

        wg2->p0_out(*m_ps_p1_in);
        wg2->p1_in(*m_dc2_p0_out);

        // Bot arm
        wg3->p0_out(*m_dc1_p3_in);
        wg3->p1_in(*m_ps_p1_in);

        dc2->p0_out(*m_dc2_p0_out);
        dc2->p1_out(*m_ps_p1_in);
        dc2->p2_in(p2_in);
        dc2->p3_in(p3_in);
    }
}