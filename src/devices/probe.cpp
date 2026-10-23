#include <ios>
#include "devices/probe.h"
#include "trace_table.h"

namespace {

// Value of a net that was never written: the default OpticalSignal (no field, wavelength id 0). Id 0
// aliases the first registered wavelength (B-012), so isnan(getWavelength()) does not detect it, and
// getWavelength() exits when no wavelength is registered yet.
inline bool isDefaultSignal(const OpticalSignal &s)
{
    return s.m_field == OpticalSignal::field_type(0.0) && s.m_wavelength_id == 0;
}

}

void Probe::on_port_in_changed()
{
    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << ":" << endl;
        cout << "Signal: " << port_channel_name(p_in) << endl;
        cout << "trace power: " << m_trace_power << endl;
        cout << "trace modulus: " << m_trace_modulus << endl;
        cout << "trace phase: " << m_trace_phase << endl;
        cout << "trace wavelength: " << m_trace_wavelength << endl;
        cout << endl;
    }

    while (true) {
        // Wait for a new input signal, or for the probe to be enabled
        wait();

        // While disabled (e.g. during the OP of a .tran), ignore the input events. Once enabled,
        // sample the current value of the net, whether or not it changed in the meantime (B-048)
        if (!enable.read().to_bool())
        {
            wait(enable.posedge_event());
        }

        auto &s = p_in->read();

        // Sampled because the probe was enabled (the net did not change in this delta) and the
        // net was never written: nothing to trace (B-048)
        if (!p_in->event() && isDefaultSignal(s))
            continue;

        // cout << name() << ": " << s << endl;
        if (!isnan(s.getWavelength()))
        {
            if (m_trace_power)
                m_trace_sig_power.write(s.power());
            if (m_trace_modulus)
                m_trace_sig_modulus.write(s.modulus());
            if (m_trace_phase)
                m_trace_sig_phase.write(s.phase());
            if (m_trace_wavelength)
                m_trace_sig_wavelength.write(s.getWavelength());
            if (m_table)
                m_table->append(sc_time_stamp().to_seconds(), m_table_signal, s);
        }
    }
}

void Probe::setTraceTable(TraceTable *table)
{
    m_table = table;
    if (m_table)
        m_table_signal = m_table->signal_index(name());
}

void MLambdaProbe::setTraceTable(TraceTable *table)
{
    m_table = table;
    if (m_table)
        m_table_signal = m_table->signal_index(name());
}

void MLambdaProbe::setTraceFile(sc_trace_file *Tf)
{
    m_Tf = Tf;
    if (!m_Tf)
        return;

    unique_ptr<sc_signal<double>> temp_signal;
    std::vector<unique_ptr<sc_signal<double>>> temp_vector;

    // for each wavelength, create three signals:
    //      power, abs, phase, that will monitor these characteristics in
    // the input optical signals that arrive
    int i = 0;
    for(const auto &wl : m_lambdas)
    {
        stringstream ss;
        ss << setprecision(6) << std::fixed << wl*1e9;
        string wl_str = ss.str();
        // Necessary because '.' is interpreted by traces as the hierarchical separator
        std::replace(wl_str.begin(), wl_str.end(), '.', '_');

        temp_vector.clear();

        temp_signal = make_unique<sc_signal<double>>(spx_child_name(this, "_trace_pow", i*3).c_str());
        sc_trace(m_Tf, *temp_signal, (string(this->name()) + ".power@" + wl_str).c_str());
        temp_vector.push_back(std::move(temp_signal));

        temp_signal = make_unique<sc_signal<double>>(spx_child_name(this, "_trace_mod", i*3).c_str());
        sc_trace(m_Tf, *temp_signal, (string(this->name()) + ".abs@" + wl_str).c_str());
        temp_vector.push_back(std::move(temp_signal));

        temp_signal = make_unique<sc_signal<double>>(spx_child_name(this, "_trace_phase", i*3).c_str());
        sc_trace(m_Tf, *temp_signal, (string(this->name()) + ".phase@" + wl_str).c_str());
        temp_vector.push_back(std::move(temp_signal));

        m_lambda_signals.insert({wl, std::move(temp_vector)});

        ++i;
    }
}
void MLambdaProbe::on_port_in_changed()
{
    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << " ready (";
        for (const auto &p : m_lambda_signals)
        {
            cout << p.first << ", ";
        }
        cout << "\b\b)" << endl;
        cout << "Signal: " << port_channel_name(p_in) << endl;
        cout << endl;
    }
    //m_trace_sig.write(0);

    while (true) {
        // Wait for a new input signal, or for the probe to be enabled
        wait();

        auto &s = p_in->read();

        // While disabled (e.g. during the OP of a .tran), keep the last value of each listed
        // wavelength without tracing it (B-184). Unlisted wavelengths are ignored silently
        if (!enable.read().to_bool())
        {
            if (p_in->event() && !isDefaultSignal(s) && m_lambdas.count(s.getWavelength()))
                m_disabled_samples[s.getWavelength()] = s;
            continue;
        }

        // Sampled because the probe was enabled (the net did not change in this delta) and the
        // net was never written: nothing to trace or to report (B-048)
        const bool sampled = p_in->event() || !isDefaultSignal(s);

        // Just enabled: trace the value of every listed wavelength the net carried while the probe
        // was disabled (B-184), in wavelength order; the current value of the net (B-048) is the
        // last one of its wavelength
        if (enable.posedge())
        {
            if (sampled && m_lambdas.count(s.getWavelength()))
                m_disabled_samples[s.getWavelength()] = s;
            for (const auto &p : m_disabled_samples)
                traceSample(p.second);
            m_disabled_samples.clear();
            if (sampled && !m_lambdas.count(s.getWavelength()))
                cout << name() << " received unsupported signal (lambda = " << s.getWavelength() << "m)" << endl;
            continue;
        }

        if (!sampled)
            continue;

        // A listed wavelength (the VCD signals only exist with a trace file: without one, e.g. with
        // --no-trace or --format csv, a listed wavelength is not "unsupported", F-05)
        if (m_lambdas.count(s.getWavelength()))
        {
            //cout << name() << " received supported signal (lambda = " << s.getWavelength() << "m)" << endl;
            traceSample(s);
        }
        else {
            cout << name() << " received unsupported signal (lambda = " << s.getWavelength() << "m)" << endl;
        }
    }
}

void MLambdaProbe::traceSample(const OpticalSignal &s)
{
    auto search = m_lambda_signals.find(s.getWavelength());
    if (search != m_lambda_signals.end())
    {
        search->second[0]->write(s.power());
        search->second[1]->write(s.modulus());
        search->second[2]->write(s.phase());
    }
    if (m_table)
        m_table->append(sc_time_stamp().to_seconds(), m_table_signal, s);
}
