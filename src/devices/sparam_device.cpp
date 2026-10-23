#include "specs.h"
#include "devices/sparam_device.h"
#include "utils/sysc_utils.h"

#include <cmath>

using namespace std;

SParamDeviceBase::SParamDeviceBase(sc_module_name name, size_t nports, bool bidirectional)
    : spx_module(name)
    , m_nports(nports)
    , m_bidirectional(bidirectional)
{
    if (bidirectional)
    {
        for (size_t k = 0; k < nports; ++k)
        {
            m_input_nets.push_back(k);
            m_output_nets.push_back(k);
        }
    }
    else
    {
        // first half inputs, second half outputs (as COUPLER: in1 in2 out1 out2)
        for (size_t k = 0; k < nports / 2; ++k)
            m_input_nets.push_back(k);
        for (size_t k = nports / 2; k < nports; ++k)
            m_output_nets.push_back(k);
    }
    for (size_t net : m_input_nets)
        p_in.push_back(make_unique<spx::oa_port_in_type>(("p" + to_string(net) + "_in").c_str()));
    for (size_t net : m_output_nets)
    {
        p_out.push_back(make_unique<spx::oa_port_out_type>(("p" + to_string(net) + "_out").c_str()));
        m_out_writers.push_back(make_unique<OpticalOutputPort>(("p" + to_string(net) + "_out_delayed_writer").c_str(), *p_out.back()));
    }
    m_memory.resize(p_in.size());

    SC_METHOD(on_inputs_changed);
    for (auto &p : p_in)
        sensitive << *p;
    dont_initialize();
    if (specsGlobalConfig.verbose_component_initialization)
        SC_THREAD(print_initialization);
}

OpticalSignal::field_type SParamDeviceBase::transmission(size_t out_net, size_t in_net, double wavelength_m) const
{
    return m_data->S(m_port_map[out_net], m_port_map[in_net], wavelength_m);
}

void SParamDeviceBase::start_of_simulation()
{
    // Paths of the data between the device's inputs and outputs
    m_reach.assign(p_in.size(), {});
    m_sources.assign(p_out.size(), {});
    for (size_t i = 0; i < p_in.size(); ++i)
        for (size_t k = 0; k < p_out.size(); ++k)
            if (m_data->active(m_port_map[m_output_nets[k]], m_port_map[m_input_nets[i]]))
            {
                m_reach[i].push_back(k);
                m_sources[k].push_back(i);
            }

    // B-014: a fixed delay is checked once, like a waveguide's (an error if it rounds to 0 and comes
    // from the netlist); the automatic delays when they are computed (one report per device)
    if (m_delay_mode == DELAY_FIXED && !m_out_writers.empty())
        m_out_writers[0]->checkDelayRounding(name(), m_delay_s, m_from_netlist);
    m_fixed_delay = sc_time(m_delay_mode == DELAY_FIXED ? m_delay_s * 1e9 : 0, SC_NS);
}

void SParamDeviceBase::print_initialization()
{
    cout << name() << ":" << endl;
    cout << "file = " << m_data->source() << " (" << SParamData::formatName(m_data->format())
         << (m_data->conjugated() ? ", conjugated" : "") << ")" << endl;
    cout << "ports = " << m_nports << (m_bidirectional ? " (bidirectional)" : " (unidirectional)") << endl;
    cout << "wavelength range = [" << m_data->minWavelength() << ", " << m_data->maxWavelength() << "] m" << endl;
    cout << "delay = ";
    if (m_delay_mode == DELAY_AUTO)
        cout << "auto";
    else if (m_delay_mode == DELAY_NONE)
        cout << "none";
    else
        cout << m_delay_s << " s";
    cout << endl;
    for (size_t i = 0; i < p_in.size(); ++i)
        cout << port_channel_name(*p_in[i]) << " --> port " << m_data->portNames()[m_port_map[m_input_nets[i]]] << endl;
    for (size_t k = 0; k < p_out.size(); ++k)
        cout << port_channel_name(*p_out[k]) << " <-- port " << m_data->portNames()[m_port_map[m_output_nets[k]]] << endl;
    cout << endl;
}

sc_time SParamDeviceBase::autoDelay(double tau_s)
{
    if (tau_s < 0)
    {
        if (!m_negative_delay_reported)
        {
            cerr << "Warning (" << name() << "): negative group delay " << tau_s
                 << " s in the S-parameters, 0 used (reported once)" << endl;
            m_negative_delay_reported = true;
        }
        tau_s = 0;
    }
    if (!m_delay_rounding_reported && !m_out_writers.empty())
    {
        // B-014: one report per device, as the waveguides' (a warning: not written in the netlist)
        const auto &config = m_out_writers[0]->getConfig();
        const double resolution_s = sc_get_time_resolution().to_seconds() * (config ? config->m_timestep_value : 1);
        if (classifyDelayRounding(tau_s, resolution_s, OpticalOutputPort::DELAY_ROUNDING_WARN_THRESHOLD) != DELAY_ROUNDING_OK)
        {
            m_out_writers[0]->checkDelayRounding(name(), tau_s, false);
            m_delay_rounding_reported = true;
        }
    }
    // the same rounding as a waveguide's (sc_time from ns)
    return sc_time(tau_s * 1e9, SC_NS);
}

SParamDeviceBase::WavelengthEntry SParamDeviceBase::computeEntry(const OpticalSignal &s)
{
    const double wl = s.getWavelength();
    if (!m_data->inRange(wl))
    {
        cerr.precision(17);
        cerr << "Error (" << name() << "): wavelength " << wl << " m is outside the range ["
             << m_data->minWavelength() << ", " << m_data->maxWavelength() << "] m of the S-parameters of "
             << m_data->source() << endl;
        exit(1);
    }
    WavelengthEntry e;
    const size_t ni = p_in.size(), no = p_out.size();
    e.S.assign(no * ni, 0);
    for (size_t k = 0; k < no; ++k)
        for (size_t i : m_sources[k])
            e.S[k * ni + i] = transmission(m_output_nets[k], m_input_nets[i], wl);

    if (m_enforce_passivity)
    {
        // the whole matrix of the data at this wavelength
        const size_t n = m_data->ports();
        vector<SParamData::complex_type> full(n * n);
        for (size_t o = 0; o < n; ++o)
            for (size_t i = 0; i < n; ++i)
                full[o * n + i] = m_data->S(o, i, wl);
        const double worst = SParamData::maxPowerSum(full, n);
        if (worst > 1)
        {
            const double scale = 1 / sqrt(worst);
            for (auto &x : e.S)
                x *= scale;
        }
    }

    e.delay.assign(no, SC_ZERO_TIME);
    for (size_t k = 0; k < no; ++k)
    {
        if (m_delay_mode == DELAY_FIXED)
            e.delay[k] = m_fixed_delay;
        else if (m_delay_mode == DELAY_AUTO && !m_sources[k].empty())
        {
            // the largest path into the output (the first one if several are equal)
            size_t best = m_sources[k][0];
            for (size_t i : m_sources[k])
                if (abs(e.S[k * ni + i]) > abs(e.S[k * ni + best]))
                    best = i;
            const double tau = m_data->groupDelay(m_port_map[m_output_nets[k]], m_port_map[m_input_nets[best]], wl);
            e.delay[k] = autoDelay(tau);
        }
    }
    return e;
}

void SParamDeviceBase::onInput(size_t input, const OpticalSignal &s)
{
    const uint32_t wl_id = s.m_wavelength_id;
    for (auto &memory : m_memory)
        if (memory.size() <= wl_id)
            memory.resize(wl_id + 1, 0);
    m_memory[input][wl_id] = s.m_field;

    const WavelengthEntry &e = m_cache.get(wl_id, [&]() { return computeEntry(s); });
    const size_t ni = p_in.size();
    // full recomputation of the outputs this input reaches, from the memories (absolute writes)
    for (size_t k : m_reach[input])
    {
        OpticalSignal::field_type out = 0;
        for (size_t i : m_sources[k])
            out += e.S[k * ni + i] * m_memory[i][wl_id];
        m_out_writers[k]->delayedWrite(OpticalSignal(out, wl_id), e.delay[k]);
    }
}

void SParamDeviceBase::on_inputs_changed()
{
    // every input with a new value in this delta cycle, in net order
    for (size_t i = 0; i < p_in.size(); ++i)
        if ((*p_in[i])->event())
            onInput(i, (*p_in[i])->read());
}
