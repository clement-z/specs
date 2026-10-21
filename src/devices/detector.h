#pragma once

#include <systemc.h>

#include "optical_output_port.h"
#include "optical_signal.h"
#include "specs.h"
#include "devices/spx_module.h"
#include "utils/rng.h"

// TODO: rename to photodetector
//
// Event-driven readout (B-049, B-050). While enabled, the readout is computed and written to
// p_readout (and to the traced m_cur_readout*):
// - on every input event;
// - when the detector becomes enabled (posedge of `enable`), from the fields already stored;
// - while >= 2 wavelengths with a non-zero field are stored (they beat, so the readout depends on
//   time), also every m_sampling_time, but these periodic samples are only written when they differ
//   from the last written readout by more than max(abstol, reltol * |last written|), with
//   specsGlobalConfig.default_abstol / default_reltol.
// With fewer than 2 non-zero wavelengths the readout is R * sum_k |E_k|^2 and nothing is scheduled,
// so a quiet circuit ends an open-ended .tran (a permanent beat keeps it running, as it should).
// A disabled detector keeps updating its input memory but never writes.
//
// Random draws (noise, dark field) come from m_rng, the stream
// spx::rng_for(specsGlobalConfig.seed, name()) taken in start_of_simulation() (F-01, B-055), so they
// are reproducible and do not depend on the other devices. Each computed readout draws, in order:
// the dark-field value (only if the total field is 0 and m_dark_field_injection is set), then one
// normal value for readout and one for readout_no_interf (also with m_noiseBypass, multiplied by 0).
class Detector : public spx_module {
public:
    // Ports
    spx::oa_port_in_type p_in;
    spx::ea_port_out_type p_readout; // TODO: divide into cathode and anode
    // Last written readouts (traced)
    double m_cur_readout = 0;
    double m_cur_readout_no_interf = 0;

    // Input memory for multi-wavelength purposes
    std::map<uint32_t,OpticalSignal::field_type> m_memory_in;

    // Detector enable signal. Enabled by default (testbenches, C++ users such as OctaneCell); the
    // .op and .dc analyses disable all detectors and .tran enables them after its operating point.
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
    // Dark field (B-056): when the total field is exactly 0, use a field of 1e-20 * N(0,1) sqrt(W)
    // instead (readout about 1e-40 A). On by default (the historical behaviour); off, a dark
    // detector reads exactly the dark current (plus noise). Netlist keyword `darkfield`
    bool m_dark_field_injection = true;

    // Random stream of this detector (seeded in start_of_simulation())
    spx::Rng m_rng;

    sc_event m_event_manual_trigger;
    bool m_input_changed = false; // input memory updated since the last computed readout

    // Seed m_rng from (specsGlobalConfig.seed, name()), once per simulation: SystemC calls it for
    // every module before the first sc_start(), after the CLI and .options were applied
    virtual void start_of_simulation();

    // Function that generates the noise applied to the output of this module
    double noise_gen(const double &noiseless_readout);

    // True when >= 2 wavelengths with a non-zero field are stored (time-dependent beat)
    bool is_beating() const;

    // Compute the readouts at the current time from the input memory (draws noise)
    void compute_readout(double &readout, double &readout_no_interf);

    // Store the readouts as the last written ones and write the readout to p_readout
    void write_readout(const double &readout, const double &readout_no_interf);

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
    {
        SC_HAS_PROCESS(Detector);
        enable = sc_logic(1);

        SC_THREAD(on_port_in_changed);
        sensitive << p_in;

        SC_THREAD(on_time_tick);
    }
};
