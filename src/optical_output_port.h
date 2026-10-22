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
    // F-15 expert option (.options coalesce, specs --no-coalescing): a write for a (time, wavelength)
    // already pending replaces (or adds to) that entry. Off: every write is a new entry, and entries
    // of equal time are taken in write order (FIFO)
    bool m_coalesce = true;
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
    // Pending entries without coalescing (F-15): equal times in insertion order
    typedef std::multimap<sc_time, OpticalSignal> fifo_queue_type;


// private:
public:
    port_type &m_port;
    queue_type m_queue;
    fifo_queue_type m_fifo_queue; // used instead of m_queue when m_coalesce is off
    sc_event_queue m_event_queue;

    std::map<uint32_t,OpticalSignal::field_type> m_desired_fields;
    std::map<uint32_t,OpticalSignal::field_type> m_emitted_fields;

    sc_time m_temporal_resolution;
    OpticalOutputPortMode m_mode;
    double m_reltol;
    double m_abstol;
    double m_snap_factor;
    bool m_use_deltas = false;
    bool m_skip_next_convergence_check = false;
    bool m_coalesce = true;
    bool m_collect_stats = false;
    OpticalOutputPortEventCap *m_event_cap = nullptr; // owned by m_config

    // F-15 --stats (kept over the whole run)
    OpticalOutputPortStats m_stats;

    // Last emission, for the non-convergence report of the .op/.dc fixed point (F-28)
    bool m_has_emitted = false;
    sc_dt::uint64 m_last_emission_delta = 0; // sc_delta_count() when it was emitted
    uint32_t m_last_emission_wlid = 0;
    double m_last_emission_change = 0; // |E - E_last|
    double m_last_emission_threshold = 0; // max(abstol, reltol * |E_last|)

    std::shared_ptr<const OpticalOutputPortConfig> m_config;

    void on_data_ready();
    void count_scheduled(bool coalesced);
    bool check_emit_by_abstol(const OpticalSignal::field_type &desired, const OpticalSignal::field_type &last);
    bool check_emit_by_reltol(const OpticalSignal::field_type &desired, const OpticalSignal::field_type &last);

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
        m_event_queue.cancel_all();
        m_queue = queue_type();
        m_fifo_queue.clear();
    }

    void reset()
    {
        drop_queue();
        m_desired_fields.clear();
        m_emitted_fields.clear();
    }

    inline bool isempty() const
    {
        return m_queue.empty() && m_fifo_queue.empty();
    }

    // Pending entries (of the queue in use)
    inline size_t pending() const
    {
        return m_coalesce ? m_queue.size() : m_fifo_queue.size();
    }

    const OpticalOutputPortStats &getStats() const
    {
        return m_stats;
    }

    void delayedWrite(const OpticalSignal &value, const sc_time &delay, const unsigned int resolution_multiplier=1);
};

// Emission gate (B-021). A new value is emitted only if it differs from the last emitted
// value (same wavelength) by more than abstol AND by more than reltol relative to it:
//   |desired - last| > max(abstol, reltol * |last|)
// (not SPICE's abstol + reltol * max(|new|, |old|)). The tolerances are thresholds below
// which a change need not be propagated, not solver tolerances. Both may be 0 (B-153):
// with abstol = reltol = 0 every change is emitted. Defined here (inline) so
// that the unit tests can call them.
inline bool OpticalOutputPort::check_emit_by_abstol(const OpticalSignal::field_type &desired,
                                                    const OpticalSignal::field_type &last)
{
    double vector_distance = abs(desired - last);
    return vector_distance > m_abstol;
}

inline bool OpticalOutputPort::check_emit_by_reltol(const OpticalSignal::field_type &desired,
                                                    const OpticalSignal::field_type &last)
{
    double vector_distance = abs(desired - last);
    double last_size = abs(last);
    // Nothing (or 0) emitted yet: any change passes, no change doesn't. Same result as
    // the division below gave (x/0 = inf > reltol, 0/0 = NaN > reltol is false)
    if (last_size == 0)
        return vector_distance > 0;
    // reltol = 0 (B-153): any change passes, also one so small next to |last| that the
    // division below underflows to 0
    if (m_reltol == 0)
        return vector_distance > 0;
    return (vector_distance/last_size) > m_reltol;
}
