#include <algorithm>
#include <iostream>

#include <pcm_device.h>

#define ZERO_PULSE_THRESHOLD (1e-10)

using namespace std;

void PCMElement::on_input_changed()
{
    // setting the transmission for the initial state
    update_transmission_local();


    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << ":" << endl;
        cout << "Emelt = " << m_meltEnergy*1e9 << " nJ" << endl;
        cout << "Nstates = " << m_nStates << "" << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p_in.get_interface()))->name();
        cout << " --> ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p_out.get_interface()))->name();
        cout << endl;
        cout << endl;
    }

    m_last_pulse_power = 0;
    bool in_window = false;
    vector<pulse_sample_t> vector_of_samples;
    uint32_t cur_wavelength_id;
    m_memory_in[0] = 0;
    double total_in_power = 0;

    while (true) {
        // Wait next input change
        wait();

        // Read signal and store field in memory for that wavelength
        auto s = p_in->read();
        cur_wavelength_id = s.m_wavelength_id;
        m_memory_in[cur_wavelength_id] = s.m_field;

        // Summing powers of all wavelengths
        total_in_power = 0;
        for (auto lambdaID_field : m_memory_in)
        {
            total_in_power += norm(lambdaID_field.second);
        }

        // Storage of optical state on the input
        const sc_time &now = sc_time_stamp();

        bool last_was_zero = m_last_pulse_power < ZERO_PULSE_THRESHOLD;
        bool current_is_zero = total_in_power < ZERO_PULSE_THRESHOLD;
        bool rising_edge = last_was_zero && !current_is_zero;
        if (!rising_edge)
        {
            // Enter here when:
            //   - a signal is received on top of another one (pulse intersect)
            //   - a signal ends (power falls to 0)

            // Record the event
            vector_of_samples.emplace_back(now.to_seconds(), total_in_power);
        }
        else
        {
            // Enter here when:
            //   - a signal starts (power rises from 0 to non-0 value)
            // This case is necessarily the first rise

            // Will unroll events and check if there was a phase change before according to
            // the vector and advance the state accordingly to get the transmission
            // cout << "\t\t first rise detected" << endl;
            in_window = phase_change(vector_of_samples, true);
            if (!in_window)
                vector_of_samples.clear();

            // Record the current event
            vector_of_samples.emplace_back(now.to_seconds(), total_in_power);
        }

        m_last_pulse_power = total_in_power;

        // Write attenuated signal to output
        s *= m_transmission;
        s.getNewId();

        m_out_writer.delayedWrite(s, SC_ZERO_TIME);
    }
}

bool PCMElement::phase_change(const vector<pulse_sample_t> &samples, const bool &local)
{
    // Here should decide between sending the
    // sim off to external or resolving inside

    // Should set new values for m_state and m_transmission

    double energy_absorbed = 0;
    double dur_pulse = 0;
    // will return true if we shouldn't erase the vector
    bool in_influence_window = false;

    if (samples.empty())
        return true;

    if (sc_time_stamp().to_seconds() -  samples.back().first < influence_time_ns*1e-9)
        return true;

    if (local)
    {
        // Integrating power along each duration
        // pulse 0 stays at pow[0] for tim[1]-tim[0] seconds
        // the last sample is zero in power, just here to get the final time
        for (unsigned int i = 0; i < samples.size()-1; i++)
        {
            dur_pulse = samples[i+1].first - samples[i].first;
            energy_absorbed += samples[i].second * dur_pulse;
        }
        // if enough energy and not saturated
        if ((energy_absorbed > m_meltEnergy) && (m_state < m_nStates))
        {
            // cout << sc_time_stamp() << ": \n\t" << name() << " phase changed with "
            //           << energy_absorbed*1e6 << " uJ" << endl;
            m_state++;
            // cout << "\tnew state: " << m_state << endl;
            update_transmission_local();
        }
    }
    else // external phase change simulation
    {
        // send the simulation to external software
        cout << "TODO: implement handles to external sim"
                << endl;
    }
    return in_influence_window;
}

void PCMElement::update_transmission_local()
{
    double sp = 0.85;
    double ep = 0.95;
    double speed = 3;
    m_transmission = sp+(ep-sp)*tanh(speed*m_state/m_nStates);
    // cout << "\tnew trans_power: " << m_transmission << "- W/W" << endl;

    // For field need to do a square root
    m_transmission = sqrt(m_transmission);
}
