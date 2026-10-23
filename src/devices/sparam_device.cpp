#include "specs.h"
#include "devices/sparam_device.h"
#include "utils/sysc_utils.h"

#include <algorithm>
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
    m_copy.resize(p_out.size() * p_in.size());

    SC_METHOD(on_inputs_changed);
    for (auto &p : p_in)
        sensitive << *p;
    sensitive << m_arrival_event;
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
    m_negative_delay_reported.assign(p_out.size() * p_in.size(), 0);
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
    if (!m_per_path_delays)
        cout << "delay_per = output" << endl;
    for (size_t i = 0; i < p_in.size(); ++i)
        cout << port_channel_name(*p_in[i]) << " --> port " << m_data->portNames()[m_port_map[m_input_nets[i]]] << endl;
    for (size_t k = 0; k < p_out.size(); ++k)
        cout << port_channel_name(*p_out[k]) << " <-- port " << m_data->portNames()[m_port_map[m_output_nets[k]]] << endl;
    cout << endl;
}

sc_time SParamDeviceBase::autoDelay(double tau_s)
{
    // Delays are only honoured by event-driven ports in a time-domain analysis (as checkDelayRounding())
    const auto &config = m_out_writers.empty() ? nullptr : m_out_writers[0]->getConfig();
    const bool timed = config && config->m_mode == OpticalOutputPortMode::EVENT_DRIVEN
                       && specsGlobalConfig.analysis_type == SPECSConfig::TIME_DOMAIN;
    const double resolution_s = sc_get_time_resolution().to_seconds() * (config ? config->m_timestep_value : 1);
    // a causal delay is >= 0: 0 is used (reportNegativeDelay())
    tau_s = std::max(tau_s, 0.0);
    if (timed && !m_delay_rounding_reported
            && classifyDelayRounding(tau_s, resolution_s, OpticalOutputPort::DELAY_ROUNDING_WARN_THRESHOLD) != DELAY_ROUNDING_OK)
    {
        // B-014: one report per device, as the waveguides' (a warning: not written in the netlist)
        m_out_writers[0]->checkDelayRounding(name(), tau_s, false);
        m_delay_rounding_reported = true;
    }
    // the same rounding as a waveguide's (sc_time from ns)
    return sc_time(tau_s * 1e9, SC_NS);
}

void SParamDeviceBase::reportNegativeDelay(size_t k, size_t i, double wavelength_m, double tau_s)
{
    const size_t path = k * p_in.size() + i;
    const auto &config = m_out_writers.empty() ? nullptr : m_out_writers[0]->getConfig();
    const bool timed = config && config->m_mode == OpticalOutputPortMode::EVENT_DRIVEN
                       && specsGlobalConfig.analysis_type == SPECSConfig::TIME_DOMAIN;
    const double resolution_s = sc_get_time_resolution().to_seconds() * (config ? config->m_timestep_value : 1);
    // only when it matters, i.e. when the negative delay is at least half a time step (smaller ones, e.g.
    // the finite-difference noise of a constant phase, round to 0 anyway)
    if (!timed || m_negative_delay_reported[path] || -tau_s < resolution_s / 2)
        return;
    m_negative_delay_reported[path] = 1;
    const size_t out = m_port_map[m_output_nets[k]], in = m_port_map[m_input_nets[i]];
    cerr << "Warning (" << name() << "): negative group delay " << tau_s << " s from port '"
         << m_data->portNames()[in] << "' to port '" << m_data->portNames()[out] << "' at " << wavelength_m
         << " m in the S-parameters, 0 used";
    if (!m_per_path_delays)
        cerr << " for every path into port '" << m_data->portNames()[out] << "' (delay_per=\"output\": "
             << "the delay of its largest path)";
    cerr << " (reported once per path)";
    if (m_data->nearFlaggedGridPoint(out, in, wavelength_m))
        cerr << "; it comes from grid points where the grid is too coarse (see the grid warning): "
             << "probably an artefact of the grid";
    cerr << (m_per_path_delays ? ". The path acts" : ". The output acts")
         << " as a connection without delay, also inside a feedback loop (nothing else "
         << "reports a loop without delay)" << endl;
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

    // the delay of each path (only honoured by event-driven ports in a time-domain analysis)
    e.delay.assign(no * ni, SC_ZERO_TIME);
    e.negative_delay_s.assign(no * ni, 0);
    for (size_t k = 0; k < no; ++k)
    {
        if (m_delay_mode == DELAY_AUTO && !m_per_path_delays && !m_sources[k].empty())
        {
            // delay_per="output": the delay of the largest path into the output (the first one if several
            // are equal), for every path into it
            size_t best = m_sources[k][0];
            for (size_t i : m_sources[k])
                if (abs(e.S[k * ni + i]) > abs(e.S[k * ni + best]))
                    best = i;
            const double tau = m_data->groupDelay(m_port_map[m_output_nets[k]], m_port_map[m_input_nets[best]], wl);
            const sc_time delay = autoDelay(tau);
            for (size_t i : m_sources[k])
                e.delay[k * ni + i] = delay;
            if (tau < 0)
                e.negative_delay_s[k * ni + best] = tau;
            continue;
        }
        for (size_t i : m_sources[k])
        {
            if (m_delay_mode == DELAY_FIXED)
                e.delay[k * ni + i] = m_fixed_delay;
            else if (m_delay_mode == DELAY_AUTO)
            {
                const double tau = m_data->groupDelay(m_port_map[m_output_nets[k]], m_port_map[m_input_nets[i]], wl);
                e.delay[k * ni + i] = autoDelay(tau);
                if (tau < 0)
                    e.negative_delay_s[k * ni + i] = tau;
            }
        }
    }
    return e;
}

void SParamDeviceBase::writeOutput(size_t k, uint32_t wavelength_id, const WavelengthEntry &e, const sc_time &delay)
{
    const size_t ni = p_in.size();
    OpticalSignal::field_type out = 0;
    for (size_t i : m_sources[k])
        out += e.S[k * ni + i] * m_copy[k * ni + i][wavelength_id];
    m_out_writers[k]->delayedWrite(OpticalSignal(out, wavelength_id), delay);
}

void SParamDeviceBase::onInput(size_t input, const OpticalSignal &s)
{
    const uint32_t wl_id = s.m_wavelength_id;
    for (auto &copy : m_copy)
        if (copy.size() <= wl_id)
            copy.resize(wl_id + 1, 0);

    const WavelengthEntry &e = m_cache.get(wl_id, [&]() { return computeEntry(s); });
    // the delays are only honoured by event-driven ports (not during an operating point, .op, .dc and
    // the one before a transient, where the ports are in NO_DELAY mode): else every path is copied at
    // once, as the ports would write at once anyway
    const bool timed = !m_out_writers.empty() && m_out_writers[0]->m_mode == OpticalOutputPortMode::EVENT_DRIVEN;
    const size_t ni = p_in.size();
    if (!m_per_path_delays)
    {
        // delay_per="output": the copies are the inputs, each output this input reaches is recomputed and
        // written after its delay (absolute writes; the port takes care of the delay and of NO_DELAY mode)
        for (size_t k : m_reach[input])
        {
            for (size_t i : m_sources[k])
                if (e.negative_delay_s[k * ni + i] < 0)
                    reportNegativeDelay(k, i, s.getWavelength(), e.negative_delay_s[k * ni + i]);
            m_copy[k * ni + input][wl_id] = s.m_field;
            writeOutput(k, wl_id, e, e.delay[k * ni + input]);
        }
        return;
    }
    for (size_t k : m_reach[input])
    {
        const size_t path = k * ni + input;
        if (e.negative_delay_s[path] < 0)
            reportNegativeDelay(k, input, s.getWavelength(), e.negative_delay_s[path]);
        if (!timed || e.delay[path] == SC_ZERO_TIME)
        {
            // no delay: the copy is the input, the output is recomputed now (absolute write)
            m_copy[path][wl_id] = s.m_field;
            writeOutput(k, wl_id, e);
        }
        else
        {
            m_arrivals.push({sc_time_stamp() + e.delay[path], m_arrival_seq++, static_cast<uint32_t>(path), wl_id, s.m_field});
            m_arrival_event.notify(e.delay[path]); // the earliest pending notification is kept
        }
    }
}

void SParamDeviceBase::processArrivals()
{
    const sc_time now = sc_time_stamp();
    // (output, wavelength id) changed by the arrivals, in the order of their first arrival
    std::vector<std::pair<size_t, uint32_t>> changed;
    const size_t ni = p_in.size();
    while (!m_arrivals.empty() && m_arrivals.top().t <= now)
    {
        const Arrival a = m_arrivals.top();
        m_arrivals.pop();
        m_copy[a.path][a.wavelength_id] = a.field;
        const std::pair<size_t, uint32_t> key(a.path / ni, a.wavelength_id);
        if (std::find(changed.begin(), changed.end(), key) == changed.end())
            changed.push_back(key);
    }
    for (const auto &key : changed)
    {
        // computed when the input event was handled
        const WavelengthEntry &e = m_cache.get(key.second, [&]() { return computeEntry(OpticalSignal(0, key.second)); });
        writeOutput(key.first, key.second, e);
    }
    if (!m_arrivals.empty())
        m_arrival_event.notify(m_arrivals.top().t - now);
}

void SParamDeviceBase::on_inputs_changed()
{
    // the delayed copies due now, then every input with a new value in this delta cycle, in net order
    processArrivals();
    for (size_t i = 0; i < p_in.size(); ++i)
        if ((*p_in[i])->event())
            onInput(i, (*p_in[i])->read());
}
