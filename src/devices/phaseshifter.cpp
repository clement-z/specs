#include "specs.h"
#include "devices/phaseshifter.h"

using namespace std;

void PhaseShifterUni::on_port_in_changed()
{
    auto transmission_field = pow(10.0, - m_attenuation_dB / 20);
    m_memory_in[0] = 0; // initializing for nan wavelength

    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << ":" << endl;
        cout << "transmission_power = " << pow(transmission_field,2) << " W/W" << endl;
        cout << "sensitivity = " << m_sensitivity << " rad/V" << endl;
        cout << "initial phase delay = " << m_phaseshift_rad << " rad @1.55)" << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p_in.get_interface()))->name();
        cout << " --> ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p_out.get_interface()))->name();
        cout << endl << "\t^" << endl;
        cout << "\t|" << endl << "\t";
        cout << (dynamic_cast<spx::ea_signal_type *>(p_vin.get_interface()))->name();
        cout << endl;
        cout << endl;
    }
    while (true) {
        // Wait for a new input signal
        wait();

        // Read the input signal and apply attenuation
        OpticalSignal s = p_in->read();


        // Store field value in memory
        m_memory_in[s.m_wavelength_id] = s.m_field;

        // Get new ID for output event
        s.getNewId();

        // Apply transmission and phase-shift
        s *= polar(transmission_field, m_sensitivity * p_vin->read());

        // Write to ouput port after zero delay
        m_out_writer.delayedWrite(s, SC_ZERO_TIME);
    }
}

void PhaseShifterUni::on_port_vin_changed()
{
    auto transmission_field = pow(10.0, - m_attenuation_dB / 20);

    uint32_t cur_wavelength_id = 0;
    OpticalSignal::field_type cur_field = (complex<double>)0;

    while (true)
    {
        wait();
        // Read the new phase-delay
        m_phaseshift_rad = m_sensitivity * p_vin->read();

        // writes the signals of all wavelengths with the new phase shift
        for(auto id_field : m_memory_in)
        {
            cur_wavelength_id = id_field.first;
            cur_field = id_field.second;
            auto s = OpticalSignal(cur_field, cur_wavelength_id);

            // Get a new ID for the signal
            s.getNewId();

            // Apply transmission and phase-shift
            s *= polar(transmission_field, m_phaseshift_rad);

            // Write to ouput port after zero delay
            m_out_writer.delayedWrite(s, SC_ZERO_TIME);
        }
    }
}


void PhaseShifterBi::on_p0_in_changed()
{
    auto transmission_field = pow(10.0, - m_attenuation_dB / 20);
    m_memory_p0[0] = 0; // initializing for nan wavelength

    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << ":" << endl;
        cout << "transmission_power = " << pow(transmission_field,2) << " W/W" << endl;
        cout << "sensitivity = " << m_sensitivity << " rad/V" << endl;
        cout << "initial phase delay = " << m_phaseshift_rad << " rad @1.55)" << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p0_in.get_interface()))->name();
        cout << " --> ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p1_out.get_interface()))->name();
        cout << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p0_out.get_interface()))->name();
        cout << " <-- ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p1_in.get_interface()))->name();
        cout << endl << "\t^" << endl;
        cout << "\t|" << endl << "\t";
        cout << (dynamic_cast<spx::ea_signal_type *>(p_vin.get_interface()))->name();
        cout << endl;
        cout << endl;
    }
    while (true) {
        // Wait for a new input signal
        wait();

        // Read the input signal and apply attenuation
        OpticalSignal s = p0_in->read();


        // Store field value in memory
        m_memory_p0[s.m_wavelength_id] = s.m_field;

        // Get new ID for output event
        s.getNewId();

        // Apply transmission and phase-shift
        s *= polar(transmission_field, m_sensitivity * p_vin->read());

        // Write to ouput port after zero delay
        m_p1_writer.delayedWrite(s, SC_ZERO_TIME);
    }
}

void PhaseShifterBi::on_p1_in_changed()
{
    auto transmission_field = pow(10.0, - m_attenuation_dB / 20);
    m_memory_p1[0] = 0; // initializing for nan wavelength

    while (true) {
        // Wait for a new input signal
        wait();

        // Read the input signal and apply attenuation
        OpticalSignal s = p1_in->read();

        // Store field value in memory
        m_memory_p1[s.m_wavelength_id] = s.m_field;

        // Get new ID for output event
        s.getNewId();

        // Apply transmission and phase-shift
        s *= polar(transmission_field, m_sensitivity * p_vin->read());

        // Write to ouput port after zero delay
        m_p0_writer.delayedWrite(s, SC_ZERO_TIME);
    }
}

void PhaseShifterBi::on_port_vin_changed()
{
    auto transmission_field = pow(10.0, - m_attenuation_dB / 20);

    uint32_t cur_wavelength_id = 0;
    OpticalSignal::field_type cur_field = (complex<double>)0;

    while (true)
    {
        wait();
        // Read the new phase-delay
        m_phaseshift_rad = m_sensitivity * p_vin->read();

        // writes the signals of all wavelengths with the new phase shift
        for(auto id_field : m_memory_p0)
        {
            cur_wavelength_id = id_field.first;
            cur_field = id_field.second;
            auto s = OpticalSignal(cur_field, cur_wavelength_id);

            // Get a new ID for the signal
            s.getNewId();

            // Apply transmission and phase-shift
            s *= polar(transmission_field, m_phaseshift_rad);

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
            s *= polar(transmission_field, m_phaseshift_rad);

            // Write to ouput port after zero delay
            m_p0_writer.delayedWrite(s, SC_ZERO_TIME);
        }
    }
}
