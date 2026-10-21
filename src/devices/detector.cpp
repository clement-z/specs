#include "specs.h"
#include "devices/detector.h"
#include <random>
#include <complex>

using namespace std;

void Detector::on_port_in_changed()
{
    // always initialize memory
    m_memory_in[0] = 0;

    // rng seed
    init();

    while (true) {
        // Wait for a new input signal
        wait();

        const auto &p_in_read = p_in->read();

        auto cur_wavelength_id = p_in_read.m_wavelength_id;
        // Updating the field memory (also while disabled: an enable writes the stored readout)
        m_memory_in[cur_wavelength_id] = p_in_read.m_field;
        m_input_changed = true;
        m_event_manual_trigger.notify();
    }
}

void Detector::on_time_tick()
{
    sc_time sampling_time = sc_time(m_sampling_time, SC_SEC);
    if (sampling_time.value() == 0)
        sampling_time = sc_time::from_value(1);

    double readout;
    double readout_no_interf;

    while (true)
    {
        // Wait for enable signal (outer loop: also after a disable during the simulation)
        if (! enable.read().to_bool())
        {
            // cout << name() << " waiting for enable" << endl;
            wait(enable.posedge_event());
            cout << name() << " was enabled" << endl;

            // Write the readout of the fields stored so far (e.g. CW inputs set during the OP)
            m_input_changed = false;
            compute_readout(readout, readout_no_interf);
            write_readout(readout, readout_no_interf);
        }

        while (enable.read().to_bool())
        {
            // Only a beat makes the readout time-dependent: sample it every Ts while it lasts.
            // Otherwise wait for the next input event only, so that a quiet circuit ends the
            // simulation (no sc_pending_activity(): detectors kept each other alive, B-050).
            if (is_beating())
                wait(sampling_time, m_event_manual_trigger | enable.value_changed_event());
            else
                wait(m_event_manual_trigger | enable.value_changed_event());

            if (! enable.read().to_bool())
                break; // disabled: never write, wait for the next posedge

            compute_readout(readout, readout_no_interf);
            if (m_input_changed)
            {
                // Input event: always write (sc_signal only notifies readers on a change)
                m_input_changed = false;
                write_readout(readout, readout_no_interf);
            }
            else
            {
                // Periodic sample of a beat: write only a change beyond the tolerance
                // max(abstol, reltol * |last written|), with the simulator-wide port tolerances
                // (specsGlobalConfig.default_abstol, applied here to the current in A, and
                // default_reltol)
                const double tol = max(specsGlobalConfig.default_abstol,
                                       specsGlobalConfig.default_reltol * abs(m_cur_readout));
                if (abs(readout - m_cur_readout) > tol)
                    write_readout(readout, readout_no_interf);
            }
        }
    }
}

bool Detector::is_beating() const
{
    unsigned int n_nonzero = 0;
    for (const auto &field : m_memory_in)
    {
        if (field.second != 0.0)
            n_nonzero++;
    }
    return n_nonzero >= 2;
}

void Detector::compute_readout(double &readout, double &readout_no_interf)
{
    OpticalSignal::field_type total_field = 0;
    OpticalSignal::field_type::value_type total_power = 0;
    double photocurrent;

    for (const auto &field : m_memory_in)
        total_power += norm(field.second);

    if (is_beating())
    {
        /* Get current time tk*/
        double tk = sc_time_stamp().to_seconds();

        /* Calculate sum of fields at current time (interference between wavelengths) */
        for (const auto &field : m_memory_in)
        {
            double wl = specsGlobalConfig.wavelengths_vector[field.first];
            double freq = 299792458 / wl;
            total_field += field.second * exp(complex<double>(0, 2 * M_PI * freq * tk));
        }

        if (norm(total_field) == 0)
            total_field = 1e-20*m_rngDist(m_rngGen);

        photocurrent = norm(total_field) * m_responsivity_A_W;
    }
    else
    {
        // At most one non-zero wavelength: no beat, use sum_k |E_k|^2 (exact, no ulp jitter
        // from e^{j2pi f t})
        if (total_power == 0)
        {
            total_field = 1e-20*m_rngDist(m_rngGen);
            photocurrent = norm(total_field) * m_responsivity_A_W;
        }
        else
            photocurrent = total_power * m_responsivity_A_W;
    }

    readout = photocurrent + (!m_noiseBypass)*noise_gen(photocurrent) + m_darkCurrent_A;
    readout_no_interf = total_power * m_responsivity_A_W + (!m_noiseBypass)*noise_gen(photocurrent) + m_darkCurrent_A;
}

void Detector::write_readout(const double &readout, const double &readout_no_interf)
{
    // The traced members hold the last written values: one VCD record per write
    m_cur_readout = readout;
    m_cur_readout_no_interf = readout_no_interf;

    // Write to output port
    p_readout->write(m_cur_readout);
}

/*
Generates a current noise to be applied to the noiseless_readout,
calculated from the responsivity.

Considers: TIA input referred, shot, and thermal
*/
double Detector::noise_gen(const double &noiseless_readout)
{
    // elementary charge
    const double q = 1.60217e-19;

    // Boltzmann constant
    const double K = 1.38064e-23;

    double inoise_tia_2 = m_opFreq_Hz*pow(m_iTIA,2);
    double inoise_shot_2 = m_opFreq_Hz*2*q*(noiseless_readout+m_darkCurrent_A);
    double inoise_therm_2 = m_opFreq_Hz*4*K*m_temp_K/m_equivR_Ohm;

    // the RMS value is also the variance of the random variable
    double inoise_rms = inoise_tia_2 + inoise_shot_2 + inoise_therm_2;

    // mean = noiseless, std = sqrt(variance)
    // the second element of the product is the gaussian(0,1)
    return sqrt(inoise_rms) * m_rngDist(m_rngGen);
}

void Detector::init()
{
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    m_rngGen.seed(rd());
}
