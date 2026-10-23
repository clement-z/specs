#pragma once

#include <systemc.h>
#include <fstream>
#include <limits>
#include <map>
#include <set>

#include "optical_output_port.h"
#include "optical_signal.h"
#include "specs.h"
#include "devices/spx_module.h"

class TraceTable;

/*
This class defines an ideal optical probe. It will sample the signal at its input and
write it in a file. Any OpticalSignal can be its input, without caring for optical
splitting, as it is completely ideal.
*/
class Probe : public spx_module {
public:
    // Ports
    spx::oa_port_in_type p_in;

    // Processes
    virtual void on_port_in_changed();

    // Member variables

    // If given a valid trace file as argument
    // Will not save to txt and rather use VCD tracing
    sc_trace_file *m_Tf;
    sc_signal<double> m_trace_sig_power;
    sc_signal<double> m_trace_sig_modulus;
    sc_signal<double> m_trace_sig_phase;
    sc_signal<double> m_trace_sig_wavelength;
    // Table export (F-05, specs --format csv|npz): every traced sample is also a row of m_table
    TraceTable *m_table = nullptr;
    uint32_t m_table_signal = 0;

    sc_signal<sc_logic> enable;
    bool m_trace_power;
    bool m_trace_modulus;
    bool m_trace_phase;
    bool m_trace_wavelength;

    // Wavelength latch (PD5): on a net that carries several wavelengths, a probe run by a netlist
    // analysis traces one of them. The mode is set by the analyses (SPECSConfig::runOPAnalysis(),
    // runDCAnalysis(), runTRANAnalysis()); the testbenches run none, so their probes trace every event
    enum LatchMode {
        LATCH_OFF,           // every event of the net
        LATCH_PER_TIME_STEP, // .op, .dc: the smallest wavelength of each time step (analysis point)
        LATCH_ONCE,          // .tran: the smallest wavelength when the probe is enabled (the values
                             // kept while it was disabled), else of the first time step that brings any
    };
    LatchMode m_latch_mode = LATCH_OFF;
    // PROBE{<net>} probes (traceall, .save): their warnings are summarised in one line
    bool m_automatic = false;

    // Latch state. While disabled, the last value of each wavelength (traced at the enable)
    std::map<double, OpticalSignal> m_disabled_samples;
    double m_latched_wavelength_m = std::numeric_limits<double>::quiet_NaN(); // NaN: none yet
    uint32_t m_latched_wavelength_id = 0;
    sc_time m_latch_time;      // time step of the latch
    bool m_latch_final = false; // latched at the enable: no smaller wavelength replaces it
    // The first wavelength ignored (NaN: none), when, and the one traced instead, for the warning
    double m_ignored_wavelength_m = std::numeric_limits<double>::quiet_NaN();
    sc_time m_ignored_time;
    double m_traced_when_ignored_m = std::numeric_limits<double>::quiet_NaN();

    // Constructor
    Probe(sc_module_name name, const bool &trace_power = true, const bool &trace_modulus = true
    , const bool &trace_phase = true, const bool &trace_wavelength = true)
        : spx_module(name)
        , m_Tf(nullptr)
        , enable((string(this->name()) + "_enable").c_str(), sc_logic(1))
        , m_trace_power(trace_power)
        , m_trace_modulus(trace_modulus)
        , m_trace_phase(trace_phase)
        , m_trace_wavelength(trace_wavelength)
    {
        SC_HAS_PROCESS(Probe);

        SC_THREAD(on_port_in_changed);
        // Also woken when the probe is enabled, to sample the current value of its net (B-048)
        sensitive << p_in << enable.posedge_event();
    }

    // Constructor overloading
    Probe(sc_module_name name, sc_trace_file *Tf, const bool &trace_power = true, const bool &trace_modulus = true
    , const bool &trace_phase = true, const bool &trace_wavelength = true)
        : Probe(name, trace_power, trace_modulus, trace_phase, trace_wavelength)
    {
        setTraceFile(Tf);
    }

    void setTraceFile(sc_trace_file *Tf)
    {
        if (m_Tf == Tf)
            return;
        m_Tf = Tf;
        if (m_Tf)
        {
            if (m_trace_power) sc_trace(m_Tf, m_trace_sig_power, (string(this->name()) + ".power").c_str());
            if (m_trace_modulus) sc_trace(m_Tf, m_trace_sig_modulus, (string(this->name()) + ".abs").c_str());
            if (m_trace_phase) sc_trace(m_Tf, m_trace_sig_phase, (string(this->name()) + ".phase").c_str());
            if (m_trace_wavelength) sc_trace(m_Tf, m_trace_sig_wavelength, (string(this->name()) + ".wavelength").c_str());
        }
    }

    void setTraceTable(TraceTable *table);

    // Whether a sample is traced (the latch); and its tracing
    bool accept(const OpticalSignal &s);
    void latchAtEnable();
    void traceSample(const OpticalSignal &s);
    void latch(const OpticalSignal &s);
    void noteIgnored(double wavelength_m, double traced_m);
};

class MLambdaProbe : public spx_module {
public:
    // Ports
    spx::oa_port_in_type p_in;

    // Processes
    virtual void on_port_in_changed();

    // Member variables

    // If given a valid trace file as argument
    // Will not save to txt and rather use VCD tracing
    sc_signal<sc_logic> enable;
    sc_trace_file *m_Tf;
    // Table export (F-05): every traced sample (listed wavelengths only) is also a row of m_table
    TraceTable *m_table = nullptr;
    uint32_t m_table_signal = 0;

    std::set<double> m_lambdas;
    std::map< double, std::vector< unique_ptr<sc_signal<double> > > > m_lambda_signals;
    // While disabled (the OP of a .tran), the last value of each listed wavelength, traced when the
    // probe is enabled: the net only holds the last value written to it, of any wavelength (B-184)
    std::map<double, OpticalSignal> m_disabled_samples;

    void setTraceFile(sc_trace_file *Tf);
    void setTraceTable(TraceTable *table);
    // Traces s, of a listed wavelength
    void traceSample(const OpticalSignal &s);

    // Constructor
    MLambdaProbe(sc_module_name name, set<double> lambdas = {})
        : spx_module(name)
        , enable((string(this->name()) + "_enable").c_str(), sc_logic(1))
        , m_Tf(nullptr)
        , m_lambdas(lambdas)
    {
        SC_HAS_PROCESS(MLambdaProbe);

        SC_THREAD(on_port_in_changed);
        // Also woken when the probe is enabled, to sample the current value of its net (B-048)
        sensitive << p_in << enable.posedge_event();
    }

    // TODO: overload the constructor for the case of not informing lambdas, need to check all components
};
