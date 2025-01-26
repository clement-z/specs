#include <ios>
#include "devices/probe.h"
#include <cstdlib> // system()

#define __modname(SUFFIX, IDX) \
    ((""s + this->name() + "_" + SUFFIX + to_string(IDX)).c_str())


void Probe::on_port_in_changed()
{
    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << ":" << endl;
        cout << "Signal: " << (dynamic_cast<spx::oa_signal_type *>(p_in.get_interface()))->name() << endl;
        cout << "trace power: " << m_trace_power << endl;
        cout << "trace modulus: " << m_trace_modulus << endl;
        cout << "trace phase: " << m_trace_phase << endl;
        cout << "trace wavelength: " << m_trace_wavelength << endl;
        cout << endl;
    }

    while (true) {
        // Wait for a new input signal
        wait();

        // Check if enabled first
        if (!enable.read().to_bool())
        {
            wait(enable.posedge_event());
            continue;
        }

        auto &s = p_in->read();
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
                // m_trace_sig_wavelength.write(299792458.0 / s.m_wavelength);
                m_trace_sig_wavelength.write(s.getWavelength());
        }
    }
}

void PowerProbe::on_port_in_changed()
{
    //m_trace_sig.write(0);

    while (true) {
        // Wait for a new input signal
        wait();
        auto &s = p_in->read();
        if (!isnan(s.getWavelength()))
            m_trace_sig_power.write(s.power());
    }
}

void PhaseProbe::on_port_in_changed()
{
    //m_trace_sig.write(0);

    while (true) {
        // Wait for a new input signal
        wait();
        auto &s = p_in->read();
        if (!isnan(s.getWavelength()))
            m_trace_sig_phase.write(s.phase());
    }
}

//__modname(SUFFIX, IDX)
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
        // int wl_nm = floor(wl*1e9);
        // int wl_sub_nm = round((wl*1e9 - wl_nm) * 1e6);
        // ss << wl_nm << "nm" << wl_sub_nm;
        ss << setprecision(6) << std::fixed << wl*1e9;
        string wl_str = ss.str();
        // Necessary because '.' is interpreted by traces as the hierarchical separator
        std::replace(wl_str.begin(), wl_str.end(), '.', '_');

        temp_vector.clear();

        temp_signal = make_unique<sc_signal<double>>(__modname("trace_pow", i*3));
        sc_trace(m_Tf, *temp_signal, (string(this->name()) + ".power@" + wl_str).c_str());
        temp_vector.push_back(std::move(temp_signal));

        temp_signal = make_unique<sc_signal<double>>(__modname("trace_mod", i*3));
        sc_trace(m_Tf, *temp_signal, (string(this->name()) + ".abs@" + wl_str).c_str());
        temp_vector.push_back(std::move(temp_signal));

        temp_signal = make_unique<sc_signal<double>>(__modname("trace_phase", i*3));
        sc_trace(m_Tf, *temp_signal, (string(this->name()) + ".phase@" + wl_str).c_str());
        temp_vector.push_back(std::move(temp_signal));

        //m_lambda_signals[m_lambdas[i]] = std::move(temp_vector);
        //or
        m_lambda_signals.insert({wl, std::move(temp_vector)});

        ++i;
    }
}
void MLambdaProbe::on_port_in_changed()
{
    // std::map< int, std::vector< unique_ptr<sc_signal<double> > > > wl_map;

    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << " ready (";
        for (const auto &p : m_lambda_signals)
        {
            cout << p.first << ", ";
        }
        cout << "\b\b)" << endl;
        cout << "Signal: " << (dynamic_cast<spx::oa_signal_type *>(p_in.get_interface()))->name() << endl;
        cout << endl;
    }
    //m_trace_sig.write(0);

    while (true) {
        // Wait for a new input signal
        wait();

        // Check if enabled first
        if (!enable.read().to_bool())
        {
            wait(enable.posedge_event());
            continue;
        }

        auto &s = p_in->read();
        auto search = m_lambda_signals.find(s.getWavelength());

        if (search != m_lambda_signals.end())
        {
            //cout << name() << " received supported signal (lambda = " << s.getWavelength() << "m)" << endl;
            m_lambda_signals[s.getWavelength()][0]->write(s.power());
            m_lambda_signals[s.getWavelength()][1]->write(s.modulus());
            m_lambda_signals[s.getWavelength()][2]->write(s.phase());
        }
        else {
            cout << name() << " received unsupported signal (lambda = " << s.getWavelength() << "m)" << endl;
        }
    }
}