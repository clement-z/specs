#pragma once

#include <specs.h>
#include <spx_module.h>
#include <mesh_col.h>

/* Clements MZI Mesh

The mesh has a total of N(N-1)/2 MZIs, as specified in
Optimal design for universal multiport interferometers by Clements et al. (2016).

Connection guide (above the X is electrical address of phi/theta):
           0             3
        0__| __ ______ __| __ ______0
           \/     2      \/     5
        1__/\__ __| __ __/\__ __| __1
           1      \/     4      \/
        2__| __ __/\__ __| __ __/\__2
           \/            \/     
        3__/\__ ______ __/\__ ______3
*/

class Clements : public spx_module{
public:
    vector<unique_ptr<spx::oa_port_in_type>> p_in;
    vector<unique_ptr<spx::oa_port_out_type>> p_out;
    vector<unique_ptr<sc_in<double>>> p_vphi;
    vector<unique_ptr<sc_in<double>>> p_vtheta;

    // Member variables
    size_t m_N; // represents the number of inputs of the mesh (rows)

    // Parameters for the internal MZIs
    double m_length_cm;
    double m_attenuation_wg_dB_cm;
    double m_attenuation_coupler_dB;
    double m_attenuation_ps_dB;
    double m_neff;
    double m_ng;

    // Submodules and wires
    vector<unique_ptr<spx::oa_signal_type>> m_optical_connect;
    vector<unique_ptr<MeshCol>> m_mesh_cols;

    // Initialization functions
    void init_ports();
    void init();

    Clements(sc_module_name name, const size_t &N,
            const double &length_cm = 1e-2,
            const double &attenuation_wg_dB_cm = 0, const double &attenuation_coupler_dB = 0,
            const double &attenuation_ps_dB = 0,
            const double &neff = 2.2111, const double &ng = 2.2637)
        : spx_module(name)
        , m_N(N)
        , m_length_cm(length_cm)
        , m_attenuation_wg_dB_cm(attenuation_wg_dB_cm)
        , m_attenuation_coupler_dB(attenuation_coupler_dB)
        , m_attenuation_ps_dB(attenuation_ps_dB)
        , m_neff(neff)
        , m_ng(ng)
    {
        assert(N > 1);
        init_ports();
    }
};