#include "devices/octane_cell.h"

void OctaneCell::init()
{
    if(m_isCompact)
        init_compact();
    else
        init_full();
}

void OctaneCell::init_full()
{
    // Physical parameters of the components
    double lambda = 1550e-9; // the wavelength used by the circuit
    double neff = 2.2;
    double ng = 2.2;
    double loss_per_cm = 0; // in waveguide
    double loss_merger = 0;
    double length_for_2pi = 100*lambda/(neff);

    // Constructing the instances
    m_wg1 = make_unique<Waveguide>(spx_child_name(this, "_WG_", 0).c_str(), length_for_2pi, loss_per_cm, neff, ng);
    m_wg2 = make_unique<Waveguide>(spx_child_name(this, "_WG_", 1).c_str(), length_for_2pi, loss_per_cm, neff, ng);
    m_wg3 = make_unique<Waveguide>(spx_child_name(this, "_WG_", 2).c_str(), length_for_2pi, loss_per_cm, neff, ng);
    m_wg4 = make_unique<Waveguide>(spx_child_name(this, "_WG_", 3).c_str(), length_for_2pi, loss_per_cm, neff, ng);

    m_merger = make_unique<Merger>(spx_child_name(this, "_Y_", 0).c_str(), loss_merger);

    m_pcm = make_unique<PCMElement>(spx_child_name(this, "_PCM_", 0).c_str(), m_meltEnergy, m_nStates, 0);

    m_pd = make_unique<Detector>(spx_child_name(this, "_PD_", 0).c_str());

    // Connecting
    m_wg1->p_in(p_in_r);
    m_wg1->p_out(m_merg_in1);

    m_wg2->p_in(p_in_c);
    m_wg2->p_out(m_merg_in2);

    m_merger->p_in1(m_merg_in1);
    m_merger->p_in2(m_merg_in2);
    m_merger->p_out(m_merg_out);

    m_wg3->p_in(m_merg_out);
    m_wg3->p_out(m_pcm_in);

    m_pcm->p_in(m_pcm_in);
    m_pcm->p_out(m_pcm_out);

    // m_wg4->p_in(m_pcm_in);
    m_wg4->p_in(m_pcm_out);
    m_wg4->p_out(m_pd_in);

    m_pd->p_in(m_pd_in);
    m_pd->p_readout(p_readout);
}

void OctaneCell::init_compact()
{
    // Physical parameters of the components
    double lambda = 1550e-9; // the wavelength used by the circuit
    double neff = 2.2;
    double ng = 2.2;
    double loss_per_cm = 0; // in waveguide
    double loss_merger = 0;
    double length_for_2pi = 100*lambda/(neff);

    (void)ng;
    (void)loss_per_cm;
    (void)length_for_2pi;

    m_merger = make_unique<Merger>(spx_child_name(this, "_Y_", 0).c_str(), loss_merger);

    m_pcm = make_unique<PCMElement>(spx_child_name(this, "_PCM_", 0).c_str(), m_meltEnergy, m_nStates, 0);

    m_pd = make_unique<Detector>(spx_child_name(this, "_PD_", 0).c_str());

    // Connecting
    m_merger->p_in1(p_in_r);
    m_merger->p_in2(p_in_c);
    m_merger->p_out(m_merg_out);

    m_pcm->p_in(m_merg_out);
    m_pcm->p_out(m_pcm_out);

    m_pd->p_in(m_pcm_out);
    m_pd->p_readout(p_readout);
}