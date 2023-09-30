#include "clements.h"

#define __modname(SUFFIX, IDX) \
    ((""s + this->name() + "_" + SUFFIX + to_string(IDX)).c_str())

void Clements::init_ports()
{
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    // Initializing optical ports
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    p_in.clear();
    p_out.clear();
    for(size_t i = 0; i < m_N; i++)
    {
        p_in.push_back(make_unique<spx::oa_port_in_type>(__modname("IN_", i)));
        p_out.push_back(make_unique<spx::oa_port_out_type>(__modname("OUT_", i)));
    }

    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    // Initializing electrical input ports
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    p_vphi.clear();
    p_vtheta.clear();

    size_t num_of_electrical_ports = (m_N * (m_N - 1) / 2);
    for(size_t i = 0; i < num_of_electrical_ports; i++)
    {
        p_vphi.push_back(make_unique<sc_in<double>>(__modname("VPHI_", i)));
        p_vtheta.push_back(make_unique<sc_in<double>>(__modname("VTHETA_", i)));
    }    
}

void Clements::init()
{
    m_optical_connect.clear();
    m_mesh_cols.clear();

    size_t num_cols = (m_N != 2) ? m_N : 1;
    size_t num_wires = m_N * (num_cols - 1);

    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    // Instantiating submodules
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    for(size_t j = 0; j < num_cols; j++)
    {
        m_mesh_cols.push_back(make_unique<MeshCol>(__modname("COL_", j),
                            m_N, j, m_length_cm, m_attenuation_wg_dB_cm,
                            m_attenuation_coupler_dB, m_attenuation_ps_dB,
                            m_neff, m_ng));
        m_mesh_cols[j]->init();
    }

    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    // Instantiating optical wires
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    for(size_t i = 0; i < num_wires; i++)
        m_optical_connect.push_back(make_unique<spx::oa_signal_type>(__modname("W_", i)));
    
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    // Connecting modules (optical)
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    if (m_N == 2)
    {
        m_mesh_cols[0]->p_in[0]->bind(*p_in[0]);
        m_mesh_cols[0]->p_in[1]->bind(*p_in[1]);
        m_mesh_cols[0]->p_out[0]->bind(*p_out[0]);
        m_mesh_cols[0]->p_out[1]->bind(*p_out[1]);
    }
    else
    {
        // first col has inputs that go to ports,
        // and outputs going to the first m_N wires
        for(size_t i = 0; i < m_N; i++)
        {
            m_mesh_cols[0]->p_in[i]->bind(*p_in[i]);
            m_mesh_cols[0]->p_out[i]->bind(*m_optical_connect[i]);
        }

        size_t in_index;
        size_t out_index;
        for(size_t j = 1; j < num_cols - 1; j++)
        {
            in_index = (j - 1) * m_N;
            out_index = j * m_N;
            for(size_t i = 0; i < m_N; i++)
            {
                m_mesh_cols[j]->p_in[i]->bind(*m_optical_connect[in_index + i]);
                m_mesh_cols[j]->p_out[i]->bind(*m_optical_connect[out_index + i]);            
            }
        }

        // last col has outputs that go to ports,
        // and inputs coming from the last m_N wires
        in_index = (num_cols - 2) * m_N;
        for(size_t i = 0; i < m_N; i++)
        {
            m_mesh_cols[num_cols - 1]->p_in[i]->bind(*m_optical_connect[in_index + i]);
            m_mesh_cols[num_cols - 1]->p_out[i]->bind(*p_out[i]);            
        }
    }

    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    // Connecting modules (electrical)
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//

    // The index of the electrical port advances top to bottom, and
    // then moves to the next column

    size_t ports_in_col;
    size_t cur_port = 0;
    for(size_t j = 0; j < num_cols; j++)
    {
        ports_in_col = m_N/2;
        if ((m_N % 2 == 0) && !(j % 2 == 0)) // if even-odd, there is one less MZI
            ports_in_col = m_N/2 - 1;
            
        for(size_t i = 0; i < ports_in_col; i++)
        {
            m_mesh_cols[j]->p_vphi[i]->bind(*p_vphi[cur_port]);
            m_mesh_cols[j]->p_vtheta[i]->bind(*p_vtheta[cur_port]);   
            
            cur_port ++;         
        }
    }
}