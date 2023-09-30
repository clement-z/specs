#pragma once

#include <systemc.h>
#include <fstream>
#include <map>
#include <set>

#include <optical_output_port.h>
#include <optical_signal.h>
#include "specs.h"
#include "spx_module.h"

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

    sc_signal<sc_logic> enable;
    bool m_trace_power;
    bool m_trace_modulus;
    bool m_trace_phase;
    bool m_trace_wavelength;

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
        sensitive << p_in;
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
};

class PowerProbe : public Probe {
public:
    virtual void on_port_in_changed();
};

class PhaseProbe : public Probe {
public:
    virtual void on_port_in_changed();
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

    sc_signal<double> m_trace_sig_power;
    sc_signal<double> m_trace_sig_modulus;
    sc_signal<double> m_trace_sig_phase;

    std::set<double> m_lambdas;
    std::map< double, std::vector< unique_ptr<sc_signal<double> > > > m_lambda_signals;

    void setTraceFile(sc_trace_file *Tf);
    void prepare();

    // Constructor
    MLambdaProbe(sc_module_name name, set<double> lambdas = {})
        : spx_module(name)
        , enable((string(this->name()) + "_enable").c_str(), sc_logic(1))
        , m_Tf(nullptr)
        , m_lambdas(lambdas)
    {
        SC_HAS_PROCESS(MLambdaProbe);

        SC_THREAD(on_port_in_changed);
        sensitive << p_in;
    }

    // TODO: overload the constructor for the case of not informing lambdas, need to check all components
};
