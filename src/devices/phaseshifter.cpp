#include "specs.h"
#include "devices/phaseshifter.h"
#include "utils/transmission_cache.h"

using namespace std;

void PhaseShifterUni::start_of_simulation()
{
    m_transmission_field = pow(10.0, - m_attenuation_dB / 20);
    m_memory_in[0] = 0; // initializing for nan wavelength
    // F-15: polar() only when the phase changes
    m_in_transmission = PolarCache(m_transmission_field);
}

void PhaseShifterUni::print_initialization()
{
    cout << name() << ":" << endl;
    cout << "transmission_power = " << pow(m_transmission_field,2) << " W/W" << endl;
    cout << "sensitivity = " << m_sensitivity << " rad/V" << endl;
    cout << "initial phase delay = " << m_phaseshift_rad << " rad" << endl;
    cout << port_channel_name(p_in);
    cout << " --> ";
    cout << port_channel_name(p_out);
    cout << endl << "\t^" << endl;
    cout << "\t|" << endl << "\t";
    cout << port_channel_name(p_vin);
    cout << endl;
    cout << endl;
}

void PhaseShifterUni::on_port_in_changed()
{
    // Read the input signal and apply attenuation
    OpticalSignal s = p_in->read();


    // Store field value in memory
    m_memory_in[s.m_wavelength_id] = s.m_field;

    // Get new ID for output event
    s.getNewId();

    // Apply transmission and phase-shift
    s *= m_in_transmission.get(m_sensitivity * p_vin->read());

    // Write to ouput port after zero delay
    m_out_writer.delayedWrite(s, SC_ZERO_TIME);
}

void PhaseShifterUni::on_port_vin_changed()
{
    uint32_t cur_wavelength_id = 0;
    OpticalSignal::field_type cur_field = (complex<double>)0;

    // Read the new phase-delay
    m_phaseshift_rad = m_sensitivity * p_vin->read();
    // F-15: the same for every wavelength, computed once
    const OpticalSignal::field_type transmission = polar(m_transmission_field, m_phaseshift_rad);

    // writes the signals of all wavelengths with the new phase shift
    for(auto id_field : m_memory_in)
    {
        cur_wavelength_id = id_field.first;
        cur_field = id_field.second;
        auto s = OpticalSignal(cur_field, cur_wavelength_id);

        // Get a new ID for the signal
        s.getNewId();

        // Apply transmission and phase-shift
        s *= transmission;

        // Write to ouput port after zero delay
        m_out_writer.delayedWrite(s, SC_ZERO_TIME);
    }
}


void PhaseShifterBi::start_of_simulation()
{
    m_transmission_field = pow(10.0, - m_attenuation_dB / 20);
    m_memory_p0[0] = 0; // initializing for nan wavelength
    m_memory_p1[0] = 0; // initializing for nan wavelength
    // F-15: polar() only when the phase changes
    m_p0_transmission = PolarCache(m_transmission_field);
    m_p1_transmission = PolarCache(m_transmission_field);
}

void PhaseShifterBi::print_initialization()
{
    cout << name() << ":" << endl;
    cout << "transmission_power = " << pow(m_transmission_field,2) << " W/W" << endl;
    cout << "sensitivity = " << m_sensitivity << " rad/V" << endl;
    cout << "initial phase delay = " << m_phaseshift_rad << " rad" << endl;
    cout << port_channel_name(p0_in);
    cout << " --> ";
    cout << port_channel_name(p1_out);
    cout << endl;
    cout << port_channel_name(p0_out);
    cout << " <-- ";
    cout << port_channel_name(p1_in);
    cout << endl << "\t^" << endl;
    cout << "\t|" << endl << "\t";
    cout << port_channel_name(p_vin);
    cout << endl;
    cout << endl;
}

void PhaseShifterBi::on_p0_in_changed()
{
    // Read the input signal and apply attenuation
    OpticalSignal s = p0_in->read();


    // Store field value in memory
    m_memory_p0[s.m_wavelength_id] = s.m_field;

    // Get new ID for output event
    s.getNewId();

    // Apply transmission and phase-shift
    s *= m_p0_transmission.get(m_sensitivity * p_vin->read());

    // Write to ouput port after zero delay
    m_p1_writer.delayedWrite(s, SC_ZERO_TIME);
}

void PhaseShifterBi::on_p1_in_changed()
{
    // Read the input signal and apply attenuation
    OpticalSignal s = p1_in->read();

    // Store field value in memory
    m_memory_p1[s.m_wavelength_id] = s.m_field;

    // Get new ID for output event
    s.getNewId();

    // Apply transmission and phase-shift
    s *= m_p1_transmission.get(m_sensitivity * p_vin->read());

    // Write to ouput port after zero delay
    m_p0_writer.delayedWrite(s, SC_ZERO_TIME);
}

void PhaseShifterBi::on_port_vin_changed()
{
    uint32_t cur_wavelength_id = 0;
    OpticalSignal::field_type cur_field = (complex<double>)0;

    // Read the new phase-delay
    m_phaseshift_rad = m_sensitivity * p_vin->read();
    // F-15: the same for every wavelength, computed once
    const OpticalSignal::field_type transmission = polar(m_transmission_field, m_phaseshift_rad);

    // writes the signals of all wavelengths with the new phase shift
    for(auto id_field : m_memory_p0)
    {
        cur_wavelength_id = id_field.first;
        cur_field = id_field.second;
        auto s = OpticalSignal(cur_field, cur_wavelength_id);

        // Get a new ID for the signal
        s.getNewId();

        // Apply transmission and phase-shift
        s *= transmission;

        // Write to ouput port after zero delay
        m_p1_writer.delayedWrite(s, SC_ZERO_TIME);
    }
    for(auto id_field : m_memory_p1)
    {
        cur_wavelength_id = id_field.first;
        cur_field = id_field.second;
        auto s = OpticalSignal(cur_field, cur_wavelength_id);

        // Get a new ID for the signal
        s.getNewId();

        // Apply transmission and phase-shift
        s *= transmission;

        // Write to ouput port after zero delay
        m_p0_writer.delayedWrite(s, SC_ZERO_TIME);
    }
}
