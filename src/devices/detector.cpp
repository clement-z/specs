#include "specs.h"
#include <detector.h>
#include <cstdlib> // system()
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
        // Updating the field memory
        m_memory_in[cur_wavelength_id] = p_in_read.m_field;
        m_event_manual_trigger.notify();
    }
}

void Detector::on_time_tick()
{
    sc_time sampling_time = sc_time(m_sampling_time, SC_SEC);
    if (sampling_time.value() == 0)
        sampling_time = sc_time::from_value(1);

    OpticalSignal::field_type total_field;
    OpticalSignal::field_type::value_type total_power;
    double photocurrent;

    // Wait for enable signal
    if (! enable.read().to_bool())
    {
        // cout << name() << " waiting for enable" << endl;
        wait(enable.posedge_event());
        cout << name() << " was enabled" << endl;
    }

    while(true)
    {
        if (sc_pending_activity()) {
            wait(sampling_time, m_event_manual_trigger);
            //wait(m_event_manual_trigger); // if only time-averaged signal is of interest
        } else {
            /* wait for reset */
            //TODO (see CWSource code)
            wait(); // effectively wait until the end of the simulation
        }

        /* Get current time tk*/
        double tk = sc_time_stamp().to_seconds();

        /* Calculate current output value based on present fields */
        /* Calculate sum of fields at current time*/
        total_field = 0;
        total_power = 0;
        for (auto field : m_memory_in)
        {
            double wl = specsGlobalConfig.wavelengths_vector[field.first];
            double freq = 299792458 / wl;
            total_field += field.second * exp(complex<double>(0, 2 * M_PI * freq * tk));
            total_power += norm(field.second);
        }

        if (norm(total_field) == 0)
            total_field = 1e-20*m_rngDist(m_rngGen);

        photocurrent = norm(total_field) * m_responsivity_A_W;
        m_cur_readout = photocurrent + (!m_noiseBypass)*noise_gen(photocurrent) + m_darkCurrent_A;
        m_cur_readout_no_interf = total_power * m_responsivity_A_W + (!m_noiseBypass)*noise_gen(photocurrent) + m_darkCurrent_A;


        //m_cur_readout = total_field.real();
        // Write to output port
        //p_readout->write(m_cur_readout);
    }
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

/*
To be implemented in the future, user can specify the wavelength response
of the photodiode
*/
double Detector::wavelength_dependent_responsivity(const double &wavelength)
{
    (void)wavelength;
    if(true)
    {
        return m_responsivity_A_W;
    }
    else
    {
        // custom responsivity curve
        return 0; // should return the new responsivity
    }
}

void Detector::init()
{
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    m_rngGen.seed(rd());
}
