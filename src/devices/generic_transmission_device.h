#pragma once

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include <systemc.h>
#include <vector>
#include <valarray>
#include "devices/spx_module.h"
#include "optical_signal.h"
#include "optical_output_port.h"

using std::vector;
using std::valarray;
using std::string;
using std::cout;
using std::endl;
using std::cerr;

class TransmissionMatrix {
public:
    typedef TransmissionMatrix this_type;
    typedef double wavelength_type;
    typedef double param_type;
    typedef valarray<param_type> taylor_series_type;
    typedef valarray<taylor_series_type> matrix_type;
    typedef struct{ double alpha, phi, tau; } paramset_type;

    size_t N; // Number of ports (= Nrow = Ncol) in the matrix
    // Arrays of matrices describing alpha, phi and the group delay via their taylor coefficients (starting from alpha0)
    matrix_type Malpha;
    matrix_type Mphi;
    matrix_type Mtau;
    valarray<bool> Mactive;
    wavelength_type lambda0; // Wavelength at which the taylor coefficients were calculated

    paramset_type operator()(const size_t &i, const size_t &j, const wavelength_type &lambda, const size_t max_order=2) const;
    inline bool isActive(const size_t &i, const size_t &j) const
    { return Mactive[i * N + j]; }
    inline bool isInputActive(const size_t &i) const
    { 
        bool ret = false;
        for (size_t j = 0; j < N; ++j)
            ret |= Mactive[i * N + j];
        return ret;
    }

    void clear()
    {
        N = 0;
        lambda0 = 0;
        Malpha.resize(0);
        Mphi.resize(0);
        Mtau.resize(0);
        Mactive.resize(0);
    }

    void resize(size_t nports)
    {
        N = nports;
        Malpha.resize(nports*nports, {0});
        Mphi.resize(nports*nports, {0});
        Mtau.resize(nports*nports, {0});
        Mactive.resize(nports*nports, false);
    }
};

class GenericTransmissionDevice : public spx_module {
public:
    typedef GenericTransmissionDevice this_type;

    /* ------------------------ */
    size_t nports;
    vector<shared_ptr<port_in_type>> ports_in;
    vector<shared_ptr<port_out_type>> ports_out;
    vector<shared_ptr<OpticalOutputPort>> ports_out_writers;
    TransmissionMatrix TM;

    /* ------------------------ */
    virtual void pre_init();
    virtual void init();
    virtual string describe() const;
    virtual void prepareTM() = 0;

    // Process input on i
    void input_on_i(size_t i);
    void input_on_i_output_on_j(size_t i, size_t j);

    GenericTransmissionDevice(sc_module_name name, size_t N = 1)
    : spx_module(name)
    , nports(N)
    {
        pre_init();
    }
};