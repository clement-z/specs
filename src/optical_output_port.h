#pragma once

#include <cstdlib>
#include <deque>
#include <systemc.h>
#include <utility>
#include <vector>
#include <queue>
#include <map>

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

class OpticalOutputPortConfig {
public:
    OpticalOutputPortMode m_mode = OpticalOutputPortMode::DEFAULT;
    double m_abstol = 1e-8;
    double m_reltol = 1e-4;
    // Emitted values with |E| < m_snap_factor * m_abstol are replaced by 0; 0 disables it (B-021)
    double m_snap_factor = 10;
    sc_time::value_type m_timestep_value = 1; // relative to systemc timestep

    OpticalOutputPortConfig() {}
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

    std::shared_ptr<const OpticalOutputPortConfig> m_config;

    void on_data_ready();
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
    void checkDelayRounding(const char *device_name, double delay_s) const;
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
    }

    void reset()
    {
        drop_queue();
        m_desired_fields.clear();
        m_emitted_fields.clear();
    }

    inline bool isempty() const
    {
        return m_queue.empty();
    }

    void delayedWrite(const OpticalSignal &value, const sc_time &delay, const unsigned int resolution_multiplier=1);
};

// Emission gate (B-021). A new value is emitted only if it differs from the last emitted
// value (same wavelength) by more than abstol AND by more than reltol relative to it:
//   |desired - last| > max(abstol, reltol * |last|)
// (not SPICE's abstol + reltol * max(|new|, |old|)). The tolerances are thresholds below
// which a change need not be propagated, not solver tolerances. Defined here (inline) so
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
    return (vector_distance/last_size) > m_reltol;
}
