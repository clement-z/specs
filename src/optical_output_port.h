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

class OpticalOutputPortConfig {
public:
    OpticalOutputPortMode m_mode = OpticalOutputPortMode::DEFAULT;
    double m_abstol = 1e-8;
    double m_reltol = 1e-4;
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
    // queue_type m_queue_fd;
    // queue_type m_queue_td;
    sc_event_queue m_event_queue;
    // sc_event_queue m_event_queue_td;
    // sc_event_queue m_event_queue_fd;
    // OpticalSignal m_cur_val;
    // OpticalSignal m_cur_val_td;
    // OpticalSignal m_cur_val_fd;
    // OpticalSignal m_emitted_val;
    // OpticalSignal m_emitted_val_fd;

    std::map<uint32_t,OpticalSignal::field_type> m_desired_fields;
    std::map<uint32_t,OpticalSignal::field_type> m_emitted_fields;

    sc_time m_temporal_resolution;
    OpticalOutputPortMode m_mode;
    double m_reltol;
    double m_abstol;
    bool m_use_deltas = false;
    bool m_converger = false;
    bool m_skip_next_convergence_check = false;
    bool m_skip_convergence_check = false;

    std::shared_ptr<const OpticalOutputPortConfig> m_config;

    // void drop_all_events();
    void on_data_ready();
    void on_data_ready_fd();
    bool check_emit_by_abstol(const OpticalSignal::field_type &desired, const OpticalSignal::field_type &last);
    bool check_emit_by_reltol(const OpticalSignal::field_type &desired, const OpticalSignal::field_type &last);

    void applyConfig();
    virtual void start_of_simulation();
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

    void swap_wavelengths(uint32_t wl1, uint32_t wl2)
    {
        drop_queue();

        // Create entries with 0 if they don't exist
        m_desired_fields.emplace(wl1, 0);
        m_desired_fields.emplace(wl2, 0);
        m_emitted_fields.emplace(wl1, 0);
        m_emitted_fields.emplace(wl2, 0);

        // Swap wl1 and wl2 entries
        swap(m_desired_fields.at(wl1), m_desired_fields.at(wl2));
        swap(m_emitted_fields.at(wl1), m_emitted_fields.at(wl2));
    }

    void delete_wavelength(uint32_t wl)
    {
        drop_queue();
        // Create entries with 0 if they don't exist
        m_desired_fields.erase(wl);
        m_emitted_fields.erase(wl);
    }

    // inline double current_err() const {
    //     double err_abs_power = abs(m_cur_val.power() - m_emitted_val.power());
    //     double err_abs_phase = abs(m_cur_val.phase() - m_emitted_val.phase());
    //     if (m_cur_val.power() > m_abstol)
    //         return max(err_abs_power, err_abs_phase);
    //     return err_abs_power;
    // }

    inline double current_err_fd() const {
        cerr << "Using deprecated function: " << __FUNCTION__ << endl;
        exit(1);
        // double err_abs_power = abs(m_cur_val_fd.power() - m_emitted_val_fd.power());
        // double err_abs_phase = abs(m_cur_val_fd.phase() - m_emitted_val_fd.phase());
        // if (m_cur_val_fd.power() >= m_abstol)
        //     return max(err_abs_power, err_abs_phase);
        // return err_abs_power;
        return 0;
    }

    inline bool isempty() const
    {
        return m_queue.empty();
    }

    void delayedWrite(const OpticalSignal &value, const sc_time &delay, const unsigned int resolution_multiplier=1);
};
