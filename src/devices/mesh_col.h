#pragma once

#include <systemc.h>
#include <specs.h>
#include <spx_module.h>
#include <mzi.h>
#include <waveguide.h>

/**
 * This class implements a single column of an MZI mesh
 * specified in the work: Optimal design for universal multiport interferometers
 * by Clements et al. (2016).
*/

class MeshCol : public spx_module{
public:
    // Ports
    vector<unique_ptr<spx::oa_port_in_type>> p_in;
    vector<unique_ptr<spx::oa_port_out_type>> p_out;
    vector<unique_ptr<sc_in<double>>> p_vphi;
    vector<unique_ptr<sc_in<double>>> p_vtheta;

    // Member variables
    size_t m_N; // represents the number of inputs of the mesh (rows)
    size_t m_j; // represents the current row being evaluated

    // Parameters for the internal MZIs
    double m_length_cm;
    double m_attenuation_wg_dB_cm;
    double m_attenuation_coupler_dB;
    double m_attenuation_ps_dB;
    double m_neff;
    double m_ng;

    // Submodules
    vector<unique_ptr<MZI>> m_mzi;
    vector<unique_ptr<Waveguide>> m_wg;


    // Initialization functions
    void init_ports();
    void init();

    void init_N_even_j_even();
    void init_N_even_j_odd();

    void init_N_odd_j_even();
    void init_N_odd_j_odd();

    /** Constructor for MZI Mesh Column
     *
     * @param name name of the module
     * @param N number of rows contained in the module
     * @param j current column being created, from 0 to N-1
     * */
    MeshCol(sc_module_name name, const size_t &N, const size_t &j,
            const double &length_cm = 1e-2,
            const double &attenuation_wg_dB_cm = 0, const double &attenuation_coupler_dB = 0,
            const double &attenuation_ps_dB = 0,
            const double &neff = 2.2111, const double &ng = 2.2637)
        : spx_module(name)
        , m_N(N)
        , m_j(j)
        , m_length_cm(length_cm)
        , m_attenuation_wg_dB_cm(attenuation_wg_dB_cm)
        , m_attenuation_coupler_dB(attenuation_coupler_dB)
        , m_attenuation_ps_dB(attenuation_ps_dB)
        , m_neff(neff)
        , m_ng(ng)
    {
        assert(N > 1);
        assert(j < N);

        init_ports();
    }
};