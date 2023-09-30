#pragma once

#include <systemc.h>
#include <fstream>
#include <random>

#include <optical_output_port.h>
#include <optical_signal.h>
#include "specs.h"
#include "spx_module.h"

// TODO: rename to photodetector
class Detector : public spx_module {
public:
    // Ports
    spx::oa_port_in_type p_in;
    spx::ea_port_out_type p_readout; // TODO: divide into cathode and anode
    double m_cur_readout;
    double m_cur_readout_no_interf;

    // Input memory for multi-wavelength purposes
    std::map<uint32_t,OpticalSignal::field_type> m_memory_in;

    // Detector enable signal
    spx::ed_signal_type enable;

    // Member variables
    double m_responsivity_A_W;
    double m_darkCurrent_A;
    double m_opFreq_Hz;
    double m_temp_K;
    double m_equivR_Ohm;
    double m_iTIA; // A/sqrt(Hz)
    bool m_noiseBypass;
    double m_sampling_time;

    // if each photodiode has an independent RNG device
    std::default_random_engine m_rngGen;
    std::normal_distribution<double> m_rngDist;

    sc_event m_event_manual_trigger;

    // Init all parameters
    void init();

    // Function that generates the noise applied to the output of this module
    double noise_gen(const double &noiseless_readout);
    double wavelength_dependent_responsivity(const double &wavelength);

    // Processes
    void on_port_in_changed();
    void on_time_tick();

    virtual void trace(sc_trace_file *Tf) const
    {
        sc_trace(Tf, m_cur_readout, (string(name()) + ".readout").c_str());
        sc_trace(Tf, m_cur_readout_no_interf, (string(name()) + ".readout_no_interference").c_str());
    }

    // Constructor
    Detector(sc_module_name name,
             double responsivity_A_W = 1,
             double darkCurrent_A = 0, // 100e-12 is an OK value
             bool noiseBypass = true,
             double opFreq_Hz = 1e9,
             double temp_K = 300,
             double equivR_Ohm = 400,
             double iTIA = 10e-12,
             double sampling_time = 1e-12)
        : spx_module(name)
        , m_responsivity_A_W(responsivity_A_W)
        , m_darkCurrent_A(darkCurrent_A)
        , m_opFreq_Hz(opFreq_Hz)
        , m_temp_K(temp_K)
        , m_equivR_Ohm(equivR_Ohm)
        , m_iTIA(iTIA)
        , m_noiseBypass(noiseBypass)
        , m_sampling_time(sampling_time)
        , m_rngDist(0,1)
    {
        SC_HAS_PROCESS(Detector);
        enable = sc_logic(0);

        SC_THREAD(on_port_in_changed);
        sensitive << p_in;

        SC_THREAD(on_time_tick);
    }
};
