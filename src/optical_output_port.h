#pragma once

#include <cstdlib>
#include <deque>
#include <systemc.h>
#include <utility>
#include <vector>
#include <queue>
#include <map>
#include <memory>
#include <set>

#include "optical_net.h"
#include "optical_signal.h"
#include "utils/pqueue.h"

using std::cout;
using std::endl;

using std::pair;
using std::vector;
using std::priority_queue;

enum OpticalOutputPortMode {
    PORT_MODE_MINVAL,
    EVENT_DRIVEN,
    SAMPLED_TIME,
    NO_DELAY,
    PORT_MODE_MAXVAL,

    // aliases
    TIME_DOMAIN      = EVENT_DRIVEN,
    FREQUENCY_DOMAIN = NO_DELAY,
    DEFAULT          = EVENT_DRIVEN,
    UNDEFINED        = PORT_MODE_MAXVAL,
};

string oopPortMode2str(OpticalOutputPortMode mode);

// B-014: a delay built from a double (sc_time(double, unit)) is rounded to the
// nearest time step, half up, like SystemC does
enum DelayRoundingVerdict {
    DELAY_ROUNDING_OK,    // zero, or rounded by at most the threshold
    DELAY_ROUNDING_WARN,  // rounded by more than the threshold (relative error)
    DELAY_ROUNDING_ERROR, // a non-zero delay rounded to 0 time steps
};
double roundDelay(double exact_s, double resolution_s);
DelayRoundingVerdict classifyDelayRounding(double exact_s, double resolution_s, double max_rel_error);

// F-15 event cap (expert option, .options maxevents, specs --max-events): the simulation stops
// (sc_stop(), at the end of the delta cycle) once the output ports sharing this object have
// processed max_events pending entries in total. One per run, shared through the port
// configuration (no global); SPECSConfig reports it
struct OpticalOutputPortEventCap {
    uint64_t max_events = 0;
    uint64_t processed = 0;
    bool reached = false;

    OpticalOutputPortEventCap(uint64_t max) : max_events(max) {}
};

class OpticalOutputPortConfig {
public:
    OpticalOutputPortMode m_mode = OpticalOutputPortMode::DEFAULT;
    double m_abstol = 1e-8;
    double m_reltol = 1e-4;
    // Emitted values with |E| < m_snap_factor * m_abstol are replaced by 0; 0 disables it (B-021)
    double m_snap_factor = 10;
    sc_time::value_type m_timestep_value = 1; // relative to systemc timestep
    // F-15 --stats: count the activity of the port (OpticalOutputPortStats)
    bool m_collect_stats = false;
    // F-15 event cap; nullptr: no cap
    std::shared_ptr<OpticalOutputPortEventCap> m_event_cap;

    OpticalOutputPortConfig() {}
};

// F-15 --stats: activity counters of one output port, collected when its configuration says so
// (m_collect_stats) and kept over the whole run (reset() keeps them); RunStats sums them.
// Every processed entry is counted once:
//   processed = emitted + suppressed_abstol + suppressed_reltol + suppressed_snap
struct OpticalOutputPortStats {
    uint64_t scheduled = 0; // values written to the port (delayedWrite())
    uint64_t coalesced = 0; // of which merged into the entry pending for the same (time, wavelength)
    uint64_t processed = 0; // pending entries taken at their time
    uint64_t emitted = 0; // of which written to the net with a new value (or forced)
    uint64_t suppressed_abstol = 0; // not written: |E - E_last| <= abstol
    uint64_t suppressed_reltol = 0; // not written: above abstol, |E - E_last| <= reltol * |E_last|
    // passed the gate, but snapped to 0 (|E| < snapfactor * abstol) while 0 was the last value
    // emitted at that wavelength: written again, no change
    uint64_t suppressed_snap = 0;
    uint64_t pending_max = 0; // pending entries after each scheduled value: largest
    uint64_t pending_sum = 0; // and sum (mean: pending_sum / scheduled)
    bool has_emitted = false;
    sc_time last_emission; // time of the last emitted value
    std::set<uint32_t> wavelength_ids; // wavelengths of the processed entries
};

class OpticalOutputPort : public sc_module {
public:
    typedef OpticalOutputPort this_type;
	typedef sc_port<sc_signal_out_if<OpticalSignal>> port_type;
	typedef pair<sc_time, OpticalSignal> pair_type;
    typedef PQueue<pair_type> queue_type;


// private:
public:
    port_type &m_port;
    queue_type m_queue;

    // F-15: the notifications of the pending entries, as the sc_event_queue used here before
    // made them, without its allocation per notification: m_pending_event fires once per entry,
    // at its time (equal times in successive delta cycles). m_pending_times holds the times not
    // yet fired; fire_pending() (called by the method on_data_ready(), at the point where the
    // kernel ran the method of sc_event_queue) takes the time that fired and notifies the next one
    sc_event m_pending_event;
    std::priority_queue<sc_time, std::vector<sc_time>, std::greater<sc_time>> m_pending_times;

    std::map<uint32_t,OpticalSignal::field_type> m_emitted_fields;

    sc_time m_temporal_resolution;
    OpticalOutputPortMode m_mode;
    double m_reltol;
    double m_abstol;
    double m_snap_factor;
    bool m_skip_next_convergence_check = false;
    // .dc (B-003, B-185): each point is a fresh steady state. After a reset(), the first entry of
    // each wavelength is written whatever the emission gate says (the gate compares it with 0) and
    // with a value-change event, also when the net already holds that value (OpticalNet), so that
    // every wavelength of the port reaches the devices and probes downstream at every point. Set by
    // SPECSConfig::runDCAnalysis() only
    bool m_emit_each_wavelength = false;
    // The net written, when it is an OpticalNet (start_of_simulation()); else the forced emissions
    // above are plain writes
    OpticalNet *m_net = nullptr;
    bool m_collect_stats = false;
    OpticalOutputPortEventCap *m_event_cap = nullptr; // owned by m_config

    // F-15 --stats (kept over the whole run)
    OpticalOutputPortStats m_stats;

    // Last emission, for the non-convergence report of the .op/.dc fixed point (F-28)
    bool m_has_emitted = false;
    sc_dt::uint64 m_last_emission_delta = 0; // sc_delta_count() when it was emitted
    uint32_t m_last_emission_wlid = 0;
    OpticalSignal::field_type m_last_emission_desired = 0; // E
    OpticalSignal::field_type m_last_emission_last = 0; // E_last

    // |E - E_last| of the last emission
    double lastEmissionChange() const
    {
        return abs(m_last_emission_desired - m_last_emission_last);
    }
    // max(abstol, reltol * |E_last|) of the last emission
    double lastEmissionThreshold() const
    {
        return std::max(m_abstol, m_reltol * abs(m_last_emission_last));
    }

    std::shared_ptr<const OpticalOutputPortConfig> m_config;

    void on_data_ready();
    void emit_due_entry();
    void fire_pending();
    void notify_pending(const sc_time &delay);
    void count_scheduled(bool coalesced);
    bool check_emit_by_abstol(const OpticalSignal::field_type &desired, const OpticalSignal::field_type &last);
    bool check_emit_by_reltol(const OpticalSignal::field_type &desired, const OpticalSignal::field_type &last);
    bool check_snap_to_zero(const OpticalSignal::field_type &desired) const;

    void applyConfig();
    virtual void start_of_simulation();

    // B-014: a delay rounded by more than this (relative error) to the time step gives a
    // warning. Chosen from the delays of every golden case, testbench and backlog repro
    // (2026-10-21): realistic parameters at the default 100 fs are rounded by at most
    // 5.95 % (the default 100 um waveguide, 755 fs -> 800 fs), delays shorter than a few
    // time steps by 11.7 % (2.27 ps at 1 ps, circuits/add_drop_pyspecs.cir) or 13 %
    // (88 fs at 100 fs), the B-014 failure mode by 87 % (53 fs -> 100 fs). 10 % warns
    // only for delays shorter than 5 time steps
    static constexpr double DELAY_ROUNDING_WARN_THRESHOLD = 0.1;
    // A non-zero delay that rounds to 0 is an error (exit 1) when zero_is_error is set (delays
    // written in the netlist), a warning otherwise (delays a device creates internally)
    void checkDelayRounding(const char *device_name, double delay_s, bool zero_is_error) const;
    sc_time snap_to_next_valid_time(const sc_time &t, const unsigned int resolution_multiplier=1);
    void delayedWriteEventDriven(const OpticalSignal &value, const sc_time &delay, const unsigned int resolution_multiplier=1);
    void delayedWriteSampledTime(const OpticalSignal &value, const sc_time &delay, const unsigned int resolution_multiplier=1);
    void immediateWriteFrequencyDomain(const OpticalSignal &value);

public:
    OpticalOutputPort(sc_module_name name, port_type &p);

    ~OpticalOutputPort() {}

    // Set config
    void setConfig(shared_ptr<OpticalOutputPortConfig> &config)
    {
        m_config = config;
    }
    const shared_ptr<const OpticalOutputPortConfig>& getConfig() const
    {
        return m_config;
    }

    void drop_queue()
    {
        if (sc_get_simulator_status() == SC_RUNNING)
        {
            cerr << "Cannot drop if simulation is running" << endl;
            exit(1);
        }
        m_pending_times = decltype(m_pending_times)();
        m_pending_event.cancel();
        m_queue = queue_type();
    }

    void reset()
    {
        drop_queue();
        m_emitted_fields.clear();
    }

    inline bool isempty() const
    {
        return m_queue.empty();
    }

    // Pending entries
    inline size_t pending() const
    {
        return m_queue.size();
    }

    const OpticalOutputPortStats &getStats() const
    {
        return m_stats;
    }

    void delayedWrite(const OpticalSignal &value, const sc_time &delay, const unsigned int resolution_multiplier=1);
};

// F-15: compares two squared magnitudes (norms) computed with a few roundings each, when those
// roundings cannot change the result of comparing the magnitudes themselves (as std::abs()
// and a division compute them): +1 if x2 > y2, -1 if x2 < y2, 0 if they are within a relative
// 1e-13 (the roundings are below 1e-14) or either is outside [1e-280, 1e280] (zero, subnormal
// components, overflow, inf, NaN), where the caller decides on the magnitudes instead
inline int compareSquaredMagnitudes(double x2, double y2)
{
    constexpr double lo = 1e-280;
    constexpr double hi = 1e280;
    constexpr double margin = 1e-13;
    if (!(x2 >= lo && x2 <= hi && y2 >= lo && y2 <= hi))
        return 0;
    if (x2 > y2 * (1 + margin))
        return 1;
    if (x2 < y2 * (1 - margin))
        return -1;
    return 0;
}

// Emission gate (B-021). A new value is emitted only if it differs from the last emitted
// value (same wavelength) by more than abstol AND by more than reltol relative to it:
//   |desired - last| > max(abstol, reltol * |last|)
// (not SPICE's abstol + reltol * max(|new|, |old|)). The tolerances are thresholds below
// which a change need not be propagated, not solver tolerances. Both may be 0 (B-153):
// with abstol = reltol = 0 every change is emitted. Defined here (inline) so
// that the unit tests can call them.
// F-15: decided on squared norms (no hypot()) when compareSquaredMagnitudes() is sure, else
// on the magnitudes as before: the same decision for every input, bit for bit
inline bool OpticalOutputPort::check_emit_by_abstol(const OpticalSignal::field_type &desired,
                                                    const OpticalSignal::field_type &last)
{
    const OpticalSignal::field_type diff = desired - last;
    // No change: |diff| = 0 is never above abstol (>= 0)
    if (diff.real() == 0 && diff.imag() == 0)
        return false;
    const int order = compareSquaredMagnitudes(norm(diff), m_abstol * m_abstol);
    if (order != 0)
        return order > 0;
    double vector_distance = abs(diff);
    return vector_distance > m_abstol;
}

inline bool OpticalOutputPort::check_emit_by_reltol(const OpticalSignal::field_type &desired,
                                                    const OpticalSignal::field_type &last)
{
    const OpticalSignal::field_type diff = desired - last;
    // No change: 0 > reltol * |last| never holds (also for |last| = 0, below)
    if (diff.real() == 0 && diff.imag() == 0)
        return false;
    // Nothing (or 0) emitted yet: any change passes, no change doesn't. Same result as
    // the division below gave (x/0 = inf > reltol, 0/0 = NaN > reltol is false)
    // (|last| = 0 exactly when both parts are 0)
    if (last.real() == 0 && last.imag() == 0)
        return abs(diff) > 0;
    // reltol = 0 (B-153): any change passes, also one so small next to |last| that the
    // division below underflows to 0
    if (m_reltol == 0)
        return abs(diff) > 0;
    const int order = compareSquaredMagnitudes(norm(diff), m_reltol * m_reltol * norm(last));
    if (order != 0)
        return order > 0;
    double vector_distance = abs(diff);
    double last_size = abs(last);
    return (vector_distance/last_size) > m_reltol;
}

// Snap (B-021): an emitted value with |E| < snap_factor * abstol is replaced by 0 (snap_factor
// = 0 or abstol = 0: never). F-15: on squared norms like the gate above
inline bool OpticalOutputPort::check_snap_to_zero(const OpticalSignal::field_type &desired) const
{
    const double threshold = m_snap_factor * m_abstol;
    // |E| < 0 never holds (nor for NaN)
    if (threshold == 0)
        return false;
    const int order = compareSquaredMagnitudes(norm(desired), threshold * threshold);
    if (order != 0)
        return order < 0;
    return abs(desired) < threshold;
}
