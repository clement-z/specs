#include <optical_output_port.h>
#include <optical_signal.h>
#include <specs.h>

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

OpticalOutputPort::OpticalOutputPort(sc_module_name name, port_type &p)
    : sc_module(name)
    , m_port(p)
    , m_config(nullptr)
{
    SC_HAS_PROCESS(OpticalOutputPort);

    // SC_THREAD(on_data_ready);
    // sensitive << m_event_queue;

    SC_THREAD(on_data_ready);
    sensitive << m_event_queue;

    // SC_THREAD(on_data_ready_fd);
    // sensitive << m_event_queue_fd;

    // SC_THREAD(drop_all_events);
    // sensitive << specsGlobalConfig.drop_all_events;
}

// void OpticalOutputPort::drop_all_events() {
//     while(true)
//     {
//         wait();
//         cout << name() << ": dropping all events!" << endl;
//         while (!isempty())
//         {
//             cout << name() << ": dropping all events" << endl;
//             m_event_queue.cancel_all();
//             m_queue = queue_type();
//             m_emitted_val = OpticalSignal(0);
//             m_queue_td = queue_type();
//             m_emitted_val = OpticalSignal(0);
//             m_queue_fd = queue_type();
//             m_emitted_val = OpticalSignal(0);
//             m_cur_val = OpticalSignal(0);
//             wait(SC_ZERO_TIME);
//         }
//     }
// }

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
    return (vector_distance/last_size) > m_reltol;
}

void OpticalOutputPort::applyConfig() {
    if (!m_config)
    {
        // Shouldn't ever need to come here if prepareSimulation was called
        cerr << "Found OpticalOutputPort without configuration. ";
        cerr << "Did you forget to run prepareSimulation() ?" << endl;
        exit(1);
        //m_config = make_shared<OpticalOutputPortConfig>();
    }

    m_temporal_resolution = sc_time::from_value(m_config->m_timestep_value);
    m_mode = m_config->m_mode;
    m_reltol = m_config->m_reltol;
    m_abstol = m_config->m_abstol;
}

void OpticalOutputPort::start_of_simulation() {
    applyConfig();
}

void OpticalOutputPort::on_data_ready()
{
    // Initialize output queue
    m_queue = queue_type();

    spx::oa_value_type::field_type desired;

    while (true) {
        // Wait for data ready notification
        wait();

        // Check if output queue is empty (would be a bug)
        if (m_queue.size() == 0) {
            if (specsGlobalConfig.drop_all_events)
                continue;
            cerr << "error: write cancelled, because no values are present" << endl;
            if (true) sc_stop();
            else      continue;
        }

        // Get current time
        sc_time now = sc_time_stamp();

        // Get the next queue item and pop it from the queue
        auto tuple = m_queue.top();
        m_queue.pop();

        // If next event is also now, notify event queue
        // if (m_queue.top().first == now)
        //     m_event_queue.notify(SC_ZERO_TIME);

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
            if (true) sc_stop();
            else      continue;
        }

        // Check signal error to decice whether to emit signal
        bool emit_signal = false;
        bool pass_abstol = false;
        bool pass_reltol = false;

        uint32_t wlid = s.m_wavelength_id;

        //auto &desired = m_desired_fields[wlid];
        auto &emitted = m_emitted_fields[wlid];

        // Store the desired output value which we know is valid
        if (m_use_deltas)
        {
            desired = m_desired_fields[wlid] + s.m_field;
            m_desired_fields[wlid] = desired;
        }
        else
            desired = s.m_field;

        // Decide whether to emit signal or not
        pass_abstol = check_emit_by_abstol(desired, emitted);
        pass_reltol = check_emit_by_reltol(desired, emitted);

        // Only emits when passes both tests
        emit_signal = pass_abstol && pass_reltol;

        // Emit zeros instead of signals smaller than a 10 abstol
        // TODO: check this !!
        if (emit_signal && (abs(desired) < 10*m_abstol))
            desired = complex<double>(0,0);


        // cout << "emit: " << emit_signal << endl;
        // Emit the desired output value if its stars are aligned
        if (emit_signal || m_skip_next_convergence_check || m_skip_convergence_check)
        {
            // cout << dynamic_cast<spx::oa_signal_type *>(m_port.get_interface())->name();
            // cout << " emitting " << m_desired_fields[wlid] << endl;
            m_skip_next_convergence_check = false;

            // Replace the stored emitted output value at that wavelength
            emitted = desired;

            // Write the value to the port
            m_port->write(spx::oa_value_type(desired, wlid));
        }
    }
}

// Should be removed
void OpticalOutputPort::on_data_ready_fd()
{
    cerr << "Using deprecated function: " << __FUNCTION__ << endl;
    exit(1);
#if 0
    // Initialize output queue
    m_queue_fd = queue_type();

    m_cur_val_fd = OpticalSignal(0);
    m_emitted_val_fd = OpticalSignal(0);

    // Current error between cur_val and emitted_val
    double err_abs_power = 0;
    double err_abs_phase = 0;

    while (true) {
        // Wait for data ready notification
        wait();

        // Check if output queue is empty (would be a bug)
        if (m_queue_fd.size() == 0) {
            if (specsGlobalConfig.drop_all_events)
                continue;
            cerr << "error: write cancelled, because no values are present" << endl;
            if (true) sc_stop();
            else      continue;
        }

        // Get current time
        sc_time now = sc_time_stamp();

        // Get the next queue item and pop it from the queue
        auto tuple = m_queue_fd.top();
        m_queue_fd.pop();

        // Assign to more readable names
        const auto &t = tuple.first;
        const auto &s = tuple.second;

        // Check whether the signal should indeed be emitted now (if not, it's a bug)
        if (t != now)
        {
            cerr << "error: desync in optical output port" << endl;
            cerr << "\t - expected time:" << t.to_seconds() << endl;
            cerr << "\t - current time:" << now.to_seconds() << endl;
            if (true) sc_stop();
            else      continue;
        }

        // If the new desired signal has NaN wavelength, ignore it
        if (isnan(s.getWavelength()))
        {
            continue;
        }

        // Check if wavelength is the same as stored signal
        bool wavelength_nan = isnan(m_emitted_val_fd.getWavelength());
        bool wavelength_same = s.m_wavelength_id == m_emitted_val_fd.m_wavelength_id;
        bool wavelength_greater = s.getWavelength() > m_emitted_val_fd.getWavelength();
        bool wavelength_ok = wavelength_same || wavelength_nan || wavelength_greater;

        // Check signal error to decice whether to emit signal
        bool emit_signal = false;
        bool pass_abstol = false;
        bool pass_reltol = false;

        // If new wavelength should not be emitted, don't do anything and
        // wait for the next signal
        if (!wavelength_ok)
            continue;

        // Here we know s can be emitted. Update the desired output value
        if (m_use_deltas)
            m_cur_val_fd += s;
        else
            m_cur_val_fd = s;

        // If previous wavelength was NaN or smaller than current wavelength, emit new signal
        if (wavelength_nan || wavelength_greater)
        {
            emit_signal = true;
        }

        // If same wavelength, decide whether to emit signal or not based on tolerances
        if (wavelength_same)
        {
            pass_abstol = check_emit_by_abstol(m_cur_val_fd, m_emitted_val_fd);
            pass_reltol = check_emit_by_reltol(m_cur_val_fd, m_emitted_val_fd);

            // Only emits when passes both tests
            emit_signal = pass_abstol && pass_reltol;

            // Emit zeros instead of signals smaller than 10 abstol
            if (emit_signal && (abs(m_cur_val_fd.m_field) < 10*m_abstol))
                m_cur_val_fd.m_field = complex<double>(0,0);
        }

        // Emit the desired output value if its stars are aligned
        if (emit_signal)
        {
            // Replace the stored emitted output value
            m_emitted_val_fd = m_cur_val_fd;

            // Write the value to the port
            m_port->write(m_cur_val_fd);
        }
    }
#endif
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
    auto t = sc_time_stamp() + delay;
    if (t.value() - sc_time_stamp().value() != delay.value())
        cerr << "Event cannot be correctly described with current timestep" << endl;

    //
    if (true /*&& resolution_multiplier > 1*/) {
        // squash time to closest timestamp using device temporal resolution
        t = snap_to_next_valid_time(t, resolution_multiplier);
    }


    // find first scheduled signal with this timestamp and this lambda
    auto it = std::find_if(m_queue.begin(), m_queue.end(), [&value,&t](const auto &x) {
            return t == x.first && value.m_wavelength_id == x.second.m_wavelength_id;
            });

    // check if an event was already scheduled for the same timestamp
    if (it == m_queue.cend()) {
        // if not, just schedule the new event
        m_queue.push(std::make_pair(t, value));
        m_event_queue.notify(t - sc_time_stamp());
    }
    else {
        // if yes, just replace or sum with the old one
        if (m_use_deltas)
            it->second += value;
        else
            it->second = value;
    }
}

void OpticalOutputPort::delayedWriteSampledTime(const OpticalSignal &value, const sc_time &delay, const unsigned int resolution_multiplier)
{
    (void)value;
    (void)delay;
    (void)resolution_multiplier;
    // TODO
    // Goal here will be to manage, maybe, some averaging
    // i.e. when we output sampled-time, if we receive event-driven signals, we convert them
    // to sampled-time by averaging over the timestep and emitting the average
}

void OpticalOutputPort::immediateWriteFrequencyDomain(const OpticalSignal &value)
{
#if 1
    //if (specsGlobalConfig.drop_all_events)
    //    return;
    //if (value.power() <= m_abs_tol_power && m_emitted_val_fd.power() <= m_abs_tol_power)
    //    return;

    const sc_time &now = sc_time_stamp();
    if (m_queue.empty() || m_queue.cbegin()->first != now) {
        // if the queue is empty, or contains no event for current time,
        // push the signal directly
        m_queue.push(make_pair(now, value));
        m_event_queue.notify(SC_ZERO_TIME);
    }
    else {
        // if the queue is contains an event for current time
        if (m_queue.cbegin()->second.m_wavelength_id == value.m_wavelength_id) {
            // if wavelengths are equal, just replace or sum with the old one
            if (m_use_deltas)
                m_queue.begin()->second += value;
            else
                m_queue.begin()->second = value;
            //m_event_queue_fd.cancel_all();
            //m_event_queue_fd.notify(SC_ZERO_TIME);
        } else {
            // otherwise just schedule event separately
            m_queue.push(make_pair(now, value));
            m_event_queue.notify(SC_ZERO_TIME);
        }
    }
#else
    cerr << "Using implemented function: " << __FUNCTION__ << endl;
#endif
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
        default:
            break; // throw here
    }

}

