#include "optical_output_port.h"
#include "optical_signal.h"
#include "specs.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

string oopPortMode2str(OpticalOutputPortMode mode)
{
    switch (mode) {
        case EVENT_DRIVEN:
            return "Event-driven time-domain";
        case SAMPLED_TIME:
            return "Emulated time-driven time-domain";
        case FREQUENCY_DOMAIN:
            return "Event-driven frequency-domain";
        default:
            return "UNDEFINED";
    }
}

double roundDelay(double exact_s, double resolution_s)
{
    // sc_time(double, unit) truncates value/resolution + 0.5 to an integer
    return std::floor(exact_s / resolution_s + 0.5) * resolution_s;
}

DelayRoundingVerdict classifyDelayRounding(double exact_s, double resolution_s, double max_rel_error)
{
    // Zero (or invalid) delays: nothing is lost
    if (!(exact_s > 0))
        return DELAY_ROUNDING_OK;
    const double rounded_s = roundDelay(exact_s, resolution_s);
    if (rounded_s == 0)
        return DELAY_ROUNDING_ERROR;
    if (std::abs(rounded_s - exact_s) / exact_s > max_rel_error)
        return DELAY_ROUNDING_WARN;
    return DELAY_ROUNDING_OK;
}

void OpticalOutputPort::checkDelayRounding(const char *device_name, double delay_s, bool zero_is_error) const
{
    // Called once per device before the simulation (start_of_simulation()), so it reads the
    // port configuration, not m_mode (set by applyConfig()). Delays are only honoured by
    // event-driven ports in a time-domain analysis: .op and .dc (NO_DELAY ports), and -m fd,
    // ignore them, so a rounding there is harmless
    if (!m_config || m_config->m_mode != OpticalOutputPortMode::EVENT_DRIVEN
            || specsGlobalConfig.analysis_type != SPECSConfig::TIME_DOMAIN)
        return;

    // Events are scheduled on multiples of the port time step: the engine time
    // resolution times the resolution multiplier (snap_to_next_valid_time())
    const double tick_s = sc_get_time_resolution().to_seconds();
    const sc_time::value_type multiplier = m_config->m_timestep_value;
    const double resolution_s = tick_s * multiplier;

    const DelayRoundingVerdict verdict = classifyDelayRounding(delay_s, resolution_s, DELAY_ROUNDING_WARN_THRESHOLD);
    if (verdict == DELAY_ROUNDING_OK)
        return;

    // How to fix it: the coarsest engine timescale that represents the delay well enough
    std::ostringstream fix;
    fix << "use a finer time resolution";
    const int timescale = static_cast<int>(std::lround(std::log10(tick_s)));
    bool found = false;
    for (int ts = timescale - 1; ts >= SPECSConfig::ONE_FS && !found; --ts)
    {
        if (classifyDelayRounding(delay_s, std::pow(10.0, ts) * multiplier, DELAY_ROUNDING_WARN_THRESHOLD) == DELAY_ROUNDING_OK)
        {
            fix << ", e.g. .options timescale=" << ts;
            found = true;
        }
    }
    if (!found)
        fix << " (.options timescale=" << SPECSConfig::ONE_FS << " is the finest)";
    if (multiplier > 1)
        fix << ", or a smaller .options resolution (" << multiplier << ")";

    if (verdict == DELAY_ROUNDING_ERROR && zero_is_error)
    {
        std::cerr << "Error (" << device_name << "): delay " << delay_s << " s rounds to 0 at the time resolution "
                  << resolution_s << " s (the delayed events would happen in the same time step); " << fix.str() << std::endl;
        exit(1);
    }
    if (verdict == DELAY_ROUNDING_ERROR)
    {
        std::cerr << "Warning (" << device_name << "): delay " << delay_s << " s rounds to 0 at the time resolution "
                  << resolution_s << " s (the delayed events happen in the same time step); " << fix.str() << std::endl;
        return;
    }

    const double rounded_s = roundDelay(delay_s, resolution_s);
    std::ostringstream rel_error;
    rel_error << std::showpos << std::setprecision(3) << 100 * (rounded_s - delay_s) / delay_s;
    std::cerr << "Warning (" << device_name << "): delay " << delay_s << " s rounded to " << rounded_s << " s ("
              << rel_error.str() << " %) at the time resolution " << resolution_s << " s; " << fix.str() << std::endl;
}

OpticalOutputPort::OpticalOutputPort(sc_module_name name, port_type &p)
    : sc_module(name)
    , m_port(p)
    , m_config(nullptr)
{
    SC_HAS_PROCESS(OpticalOutputPort);

    // F-15: a method, not a thread (no context switch per entry). It runs only when an entry
    // is due (the thread it replaces ran at initialization only to wait). It also does the work
    // of the method of the former sc_event_queue member (fire_pending())
    SC_METHOD(on_data_ready);
    sensitive << m_pending_event;
    dont_initialize();
}

void OpticalOutputPort::applyConfig() {
    if (!m_config)
    {
        // Shouldn't ever need to come here if prepareSimulation was called
        cerr << "Found OpticalOutputPort without configuration. ";
        cerr << "Did you forget to run prepareSimulation() ?" << endl;
        exit(1);
    }
    if (m_config->m_mode == OpticalOutputPortMode::SAMPLED_TIME)
    {
        // B-002: delayedWriteSampledTime() is a TODO that drops every write. Rejected
        // here, at configuration time, rather than in delayedWrite() (per event)
        cerr << "Error: " << name() << ": sampled-time mode is not implemented" << endl;
        exit(1);
    }

    m_temporal_resolution = sc_time::from_value(m_config->m_timestep_value);
    m_mode = m_config->m_mode;
    m_reltol = m_config->m_reltol;
    m_abstol = m_config->m_abstol;
    m_snap_factor = m_config->m_snap_factor;
    m_collect_stats = m_config->m_collect_stats;
    m_event_cap = m_config->m_event_cap.get();
}

void OpticalOutputPort::start_of_simulation() {
    applyConfig();
}

// F-15: m_pending_event fired: emit the entry that is due, then notify the next one. The kernel
// ran the two (as separate methods, both sensitive to m_pending_event) in this order
void OpticalOutputPort::on_data_ready()
{
    emit_due_entry();
    fire_pending();
}

void OpticalOutputPort::emit_due_entry()
{
    spx::oa_value_type::field_type desired;

    // Check if output queue is empty (would be a bug)
    if (m_queue.size() == 0) {
        cerr << "error: write cancelled, because no values are present" << endl;
        sc_stop();
        return;
    }

    // Get current time
    sc_time now = sc_time_stamp();

    // Get the next queue item and pop it from the queue
    auto tuple = m_queue.top();
    m_queue.pop();

    // Assign to more readable names
    const auto &t = tuple.first; //time
    const auto &s = tuple.second; //signal

    // Check whether the signal should indeed be emitted now (if not, it's a bug)
    if (t != now)
    {
        cerr << "error: desync in optical output port " << this->name() << endl;
        cerr << "\t - expected time:" << t.to_seconds() << endl;
        cerr << "\t - current time:" << now.to_seconds() << endl;
        cerr << "\t - signal:" << s << endl;
        sc_stop();
        return;
    }

    // Check signal error to decice whether to emit signal
    bool emit_signal = false;
    bool pass_abstol = false;
    bool pass_reltol = false;

    uint32_t wlid = s.m_wavelength_id;

    auto &emitted = m_emitted_fields[wlid];

    // Store the desired output value which we know is valid
    desired = s.m_field;

    // Decide whether to emit signal or not (rule: see check_emit_by_reltol(), B-021)
    pass_abstol = check_emit_by_abstol(desired, emitted);
    pass_reltol = check_emit_by_reltol(desired, emitted);

    // Only emits when passes both tests
    emit_signal = pass_abstol && pass_reltol;

    // Emit zeros instead of values smaller than snap_factor * abstol (default 10, as in
    // the paper benchmarks; .options snapfactor, 0 disables it, B-021)
    bool snapped = false;
    if (emit_signal && check_snap_to_zero(desired))
    {
        desired = complex<double>(0,0);
        snapped = true;
    }

    // F-15 --stats: one count per processed entry (OpticalOutputPortStats)
    if (m_collect_stats)
    {
        ++m_stats.processed;
        m_stats.wavelength_ids.insert(wlid);
        if (!emit_signal && !m_skip_next_convergence_check)
        {
            if (!pass_abstol)
                ++m_stats.suppressed_abstol;
            else
                ++m_stats.suppressed_reltol;
        }
        else if (snapped && desired == emitted)
            ++m_stats.suppressed_snap;
        else
        {
            ++m_stats.emitted;
            m_stats.has_emitted = true;
            m_stats.last_emission = now;
        }
    }

    // F-15 event cap: stop at the end of this delta cycle once the run has processed
    // max_events entries (SPECSConfig reports it)
    if (m_event_cap && ++m_event_cap->processed >= m_event_cap->max_events && !m_event_cap->reached)
    {
        m_event_cap->reached = true;
        sc_stop();
    }


    // cout << "emit: " << emit_signal << endl;
    // Emit the desired output value if its stars are aligned
    if (emit_signal || m_skip_next_convergence_check)
    {
        // cout << dynamic_cast<spx::oa_signal_type *>(m_port.get_interface())->name();
        // cout << " emitting " << desired << endl;
        m_skip_next_convergence_check = false;

        // Remember the change, for the .op/.dc non-convergence report (F-28)
        // (the magnitudes are computed when the report asks for them)
        m_has_emitted = true;
        m_last_emission_delta = sc_delta_count();
        m_last_emission_wlid = wlid;
        m_last_emission_desired = desired;
        m_last_emission_last = emitted;

        // Replace the stored emitted output value at that wavelength
        emitted = desired;

        // Write the value to the port
        m_port->write(spx::oa_value_type(desired, wlid));
    }
}

// F-15: an entry pending in delay (sc_event_queue::notify(): the event is notified only when
// the entry is earlier than all the pending ones)
void OpticalOutputPort::notify_pending(const sc_time &delay)
{
    const sc_time t = sc_time_stamp() + delay;
    if (m_pending_times.empty() || t < m_pending_times.top())
        m_pending_event.notify(delay);
    m_pending_times.push(t);
}

// F-15: m_pending_event fired (sc_event_queue::fire_event()): one entry is due now, notify the
// next one
void OpticalOutputPort::fire_pending()
{
    // cancelled (drop_queue())
    if (m_pending_times.empty())
        return;
    m_pending_times.pop();
    if (!m_pending_times.empty())
        m_pending_event.notify(m_pending_times.top() - sc_time_stamp());
}

sc_time OpticalOutputPort::snap_to_next_valid_time(const sc_time &t, const unsigned int resolution_multiplier)
{
    // Squash time to closest timestamp using device temporal resolution and multiplier

    // Find the effective number of ticks/timestep of the output port
    sc_time::value_type dt_val = m_temporal_resolution.value() * resolution_multiplier;

    // Should be at least one tick
    if (dt_val <= 1)
       return t;

    // Snap the time to the closest multiple of dt
    //auto t_snap_value = dt_val * round((double)t.value() / dt_val);
    auto t_snap_value = dt_val * ((t.value() + dt_val/2)/ dt_val);

    // If we end up before current time, snap to the next multiple of dt
    // We do this instead of using ceil() because in the case where the event
    // is far into the future, it is more accurate to round().
    if (t_snap_value < sc_time_stamp().value())
        t_snap_value += dt_val;

    // Return the snaped time value (in number of simulation ticks)
    return sc_time::from_value(t_snap_value);
}

void OpticalOutputPort::delayedWriteEventDriven(const OpticalSignal &value, const sc_time &delay, const unsigned int resolution_multiplier)
{
    // Calculate the simulation time at which event was requested to be emitted.
    // (The delay was rounded to the time resolution when it was built from a double:
    // devices check that once, before the simulation, with checkDelayRounding(), B-014)
    auto t = sc_time_stamp() + delay;

    //
    if (true /*&& resolution_multiplier > 1*/) {
        // squash time to closest timestamp using device temporal resolution
        t = snap_to_next_valid_time(t, resolution_multiplier);
    }

    // find first scheduled signal with this timestamp and this lambda
    auto it = std::find_if(m_queue.begin(), m_queue.end(), [&value,&t](const auto &x) {
            return t == x.first && value.m_wavelength_id == x.second.m_wavelength_id;
            });
    const bool coalesced = it != m_queue.cend();

    // check if an event was already scheduled for the same timestamp
    if (it == m_queue.cend()) {
        // if not, just schedule the new event
        m_queue.push(std::make_pair(t, value));
        notify_pending(t - sc_time_stamp());
    }
    else {
        // if yes, just replace the old one
        it->second = value;
    }
    if (m_collect_stats)
        count_scheduled(coalesced);
}

// F-15 --stats: a value written to the port, merged into a pending entry or not
void OpticalOutputPort::count_scheduled(bool coalesced)
{
    ++m_stats.scheduled;
    if (coalesced)
        ++m_stats.coalesced;
    const uint64_t n = pending();
    m_stats.pending_sum += n;
    m_stats.pending_max = std::max(m_stats.pending_max, n);
}

void OpticalOutputPort::delayedWriteSampledTime(const OpticalSignal &value, const sc_time &delay, const unsigned int resolution_multiplier)
{
    (void)value;
    (void)delay;
    (void)resolution_multiplier;
    // TODO (until then, SAMPLED_TIME is rejected by SPECSConfig::verifyConfig() and applyConfig(), B-002)
    // Goal here will be to manage, maybe, some averaging
    // i.e. when we output sampled-time, if we receive event-driven signals, we convert them
    // to sampled-time by averaging over the timestep and emitting the average
}

void OpticalOutputPort::immediateWriteFrequencyDomain(const OpticalSignal &value)
{
    //if (value.power() <= m_abs_tol_power && m_emitted_val_fd.power() <= m_abs_tol_power)
    //    return;

    const sc_time &now = sc_time_stamp();

    // find a scheduled signal with this timestamp and this lambda anywhere in
    // the queue, not only at its top: another wavelength can be on top (B-001)
    auto it = std::find_if(m_queue.begin(), m_queue.end(), [&value,&now](const auto &x) {
            return now == x.first && value.m_wavelength_id == x.second.m_wavelength_id;
            });
    const bool coalesced = it != m_queue.end();

    if (it == m_queue.end()) {
        // if not, schedule the signal for the current time
        m_queue.push(make_pair(now, value));
        notify_pending(SC_ZERO_TIME);
    }
    else {
        // if yes, just replace the old one
        it->second = value;
    }
    if (m_collect_stats)
        count_scheduled(coalesced);
}

void OpticalOutputPort::delayedWrite(const OpticalSignal &value, const sc_time &delay, const unsigned int resolution_multiplier)
{
    switch(m_mode) {
        case OpticalOutputPortMode::EVENT_DRIVEN:
            delayedWriteEventDriven(value, delay, resolution_multiplier);
            return;
        case OpticalOutputPortMode::SAMPLED_TIME:
            delayedWriteSampledTime(value, delay, resolution_multiplier);
            return;
        case OpticalOutputPortMode::FREQUENCY_DOMAIN:
            //delayedWriteEventDriven(value, SC_ZERO_TIME, 1);
            immediateWriteFrequencyDomain(value);
            break;
        default:
            break; // throw here
    }

}

