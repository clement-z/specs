#include "devices/mesh_col.h"

#define __modname(SUFFIX, IDX) \
    ((""s + this->name() + "_" + SUFFIX + to_string(IDX)).c_str())

void MeshCol::init_ports()
{
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    // Initializing optical input ports
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//

    p_in.clear();
    for(size_t i = 0; i < m_N; i++)
    {
        p_in.push_back(make_unique<spx::oa_port_in_type>(__modname("IN_", i)));
    }

    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    // Initializing optical output ports
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//

    p_out.clear();
    for(size_t i = 0; i < m_N; i++)
    {
        p_out.push_back(make_unique<spx::oa_port_out_type>(__modname("OUT_", i)));
    }

    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    // Initializing electrical input ports
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//

    p_vphi.clear();
    p_vtheta.clear();

    size_t num_of_electrical_ports = m_N/2;
    if ((m_N % 2 == 0) && !(m_j % 2 == 0)) // if even-odd, there is one less MZI
        num_of_electrical_ports = m_N/2 - 1;


    for(size_t i = 0; i < num_of_electrical_ports; i++)
    {
        p_vphi.push_back(make_unique<sc_in<double>>(__modname("VPHI_", i)));
        p_vtheta.push_back(make_unique<sc_in<double>>(__modname("VTHETA_", i)));
    }
}

void MeshCol::init()
{
    if (m_N % 2 == 0)
    {
        if (m_j % 2 == 0)
            init_N_even_j_even();
        else
            init_N_even_j_odd();
    }
    else
    {
        if (m_j % 2 == 0)
            init_N_odd_j_even();
        else
            init_N_odd_j_odd();
    }
}

/* 
    For N=4 that's how it looks
        0__  __0
           \/
        1__/\__1

        2__  __2
           \/
        3__/\__3

    TODO: snap to 2pi phase instead of setting neff = 0
*/
void MeshCol::init_N_even_j_even()
{
    for(size_t i = 0; i < m_N/2; i++)
    {
        m_mzi.push_back(make_unique<MZI>(__modname("MZI_", i),
                m_length_cm, m_attenuation_wg_dB_cm,
                m_attenuation_coupler_dB, m_attenuation_ps_dB,
                m_neff, m_ng));
                
        m_mzi[i]->p_in1(*p_in[2*i]);
        m_mzi[i]->p_in2(*p_in[2*i+1]);
        m_mzi[i]->p_out1(*p_out[2*i]);
        m_mzi[i]->p_out2(*p_out[2*i+1]);
        m_mzi[i]->p_vphi(*p_vphi[i]);
        m_mzi[i]->p_vtheta(*p_vtheta[i]);
        m_mzi[i]->init();
    }
}


/* For N=4 that's how it looks

        0______0

        1__  __1
           \/
        2__/\__2

        3______3

    The empty rows have a waveguide that adds zero phase
    but can have loss and time delay.

    TODO: snap to 2pi phase instead of setting neff = 0

*/
void MeshCol::init_N_even_j_odd()
{
    assert(m_N >= 4);

    // Zero neff so it doesn't add any phase
    m_wg.push_back(make_unique<Waveguide>(__modname("WG_", 0),
                m_length_cm, m_attenuation_wg_dB_cm,
                m_neff, m_ng));
    m_wg[0]->p_in(*p_in[0]);
    m_wg[0]->p_out(*p_out[0]);

    for(size_t i = 1; i <= m_N/2 - 1; i++)
    {
        m_mzi.push_back(make_unique<MZI>(__modname("MZI_", i),
                m_length_cm, m_attenuation_wg_dB_cm,
                m_attenuation_coupler_dB, m_attenuation_ps_dB,
                m_neff, m_ng));
                
        m_mzi[i - 1]->p_in1(*p_in[2 * i - 1]);
        m_mzi[i - 1]->p_in2(*p_in[2 * i]);
        m_mzi[i - 1]->p_out1(*p_out[2 * i - 1]);
        m_mzi[i - 1]->p_out2(*p_out[2 * i]);
        m_mzi[i - 1]->p_vphi(*p_vphi[i - 1]);
        m_mzi[i - 1]->p_vtheta(*p_vtheta[i - 1]);
        m_mzi[i - 1]->init();

    }

    // Zero neff so it doesn't add any phase
    m_wg.push_back(make_unique<Waveguide>(__modname("WG_", 1),
                m_length_cm, m_attenuation_wg_dB_cm,
                m_neff, m_ng));
    m_wg[1]->p_in(*p_in[m_N-1]);
    m_wg[1]->p_out(*p_out[m_N-1]);

}

/* For N=5 that's how it looks

        0__  __0
           \/
        1__/\__1

        2__  __2
           \/
        3__/\__3

        4______4

    The empty rows have a waveguide that adds zero phase
    but can have loss and time delay.

    TODO: snap to 2pi phase instead of setting neff = 0

*/
void MeshCol::init_N_odd_j_even()
{
    cout << "odd-even" << endl;
    for(size_t i = 0; i < m_N/2; i++)
    {
        m_mzi.push_back(make_unique<MZI>(__modname("MZI_", i),
                m_length_cm, m_attenuation_wg_dB_cm,
                m_attenuation_coupler_dB, m_attenuation_ps_dB,
                m_neff, m_ng));
                
        m_mzi[i]->p_in1(*p_in[2*i]);
        m_mzi[i]->p_in2(*p_in[2*i+1]);
        m_mzi[i]->p_out1(*p_out[2*i]);
        m_mzi[i]->p_out2(*p_out[2*i+1]);
        m_mzi[i]->p_vphi(*p_vphi[i]);
        m_mzi[i]->p_vtheta(*p_vtheta[i]);
        m_mzi[i]->init();
    }
    // Zero neff so it doesn't add any phase
    m_wg.push_back(make_unique<Waveguide>(__modname("WG_", 0),
                m_length_cm, m_attenuation_wg_dB_cm,
                m_neff, m_ng));
    m_wg[0]->p_in(*p_in[m_N-1]);
    m_wg[0]->p_out(*p_out[m_N-1]);
}

/* For N=5 that's how it looks
        
        0______0

        1__  __1
           \/
        2__/\__2

        3__  __3
           \/
        4__/\__4

    The empty rows have a waveguide that adds zero phase
    but can have loss and time delay.

    TODO: snap to 2pi phase instead of setting neff = 0

*/
void MeshCol::init_N_odd_j_odd()
{
    cout << "odd-odd" << endl;
    // Zero neff so it doesn't add any phase
    m_wg.push_back(make_unique<Waveguide>(__modname("WG_", 0),
                m_length_cm, m_attenuation_wg_dB_cm,
                m_neff, m_ng));
    m_wg[0]->p_in(*p_in[0]);
    m_wg[0]->p_out(*p_out[0]);

    for(size_t i = 1; i <= m_N/2; i++)
    {
        m_mzi.push_back(make_unique<MZI>(__modname("MZI_", i),
                m_length_cm, m_attenuation_wg_dB_cm,
                m_attenuation_coupler_dB, m_attenuation_ps_dB,
                m_neff, m_ng));
                
        m_mzi[i - 1]->p_in1(*p_in[2 * i - 1]);
        m_mzi[i - 1]->p_in2(*p_in[2 * i]);
        m_mzi[i - 1]->p_out1(*p_out[2 * i - 1]);
        m_mzi[i - 1]->p_out2(*p_out[2 * i]);
        m_mzi[i - 1]->p_vphi(*p_vphi[i - 1]);
        m_mzi[i - 1]->p_vtheta(*p_vtheta[i - 1]);
        m_mzi[i - 1]->init();
    }
}