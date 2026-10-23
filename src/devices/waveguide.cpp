#include "specs.h"
#include "devices/waveguide.h"
#include "utils/transmission_cache.h"

using namespace std;

void WaveguideBase::prepareTransmission()
{
    const double c = 299792458.0;

    // attenuation in dB
    m_attenuation_dB = m_attenuation_dB_cm * m_length_cm;

    // transmission in field: 10^(-dB/20)
    m_transmission = pow(10.0, - m_attenuation_dB / 20.0);

    // vg = c/ng
    // => delay = L / vg = (L * ng) / c
    const double group_delay_ns = groupDelay_ns();

    // precalculate 2 * pi * L
    m_phase_delay_factor = 2.0 * M_PI * m_length_cm * 1e-2;

    // ng = neff - lambda * dneff/dlambda
    // => dneff/dlambda = (neff - ng)/lambda
    m_dneff_dlambda = (m_neff - m_ng) / 1.55e-6;

    // Parameters relative to dispersion
    m_d2neff_dlambda2_over_2 = -1 * c * m_D / (2 * 1.55e-6);
    m_dng_dlambda = c*m_D;

    m_group_delay = sc_time(group_delay_ns, SC_NS);
    m_dispersive = m_D != 0;
}

void WaveguideBase::propagate(OpticalSignal s, OpticalOutputPort &out)
{
    const double c = 299792458.0;

    if(!m_dispersive)
    {
        // Transmission function at the wavelength (computed once per wavelength, F-15)
        const OpticalSignal::field_type &S12 = m_S12_cache.get(s.m_wavelength_id, [&]() {
            // calculate phase-delay
            const double neff = m_neff + m_dneff_dlambda * (s.getWavelength() - 1.55e-6);

            const double phase_delay = m_phase_delay_factor * neff / s.getWavelength();

            return polar(m_transmission, phase_delay);
        });

        // Apply transmission function
        s *= S12;

        // Get new ID for output event
        s.getNewId();

        // Write to ouput port after group delay
        out.delayedWrite(s, m_group_delay);
    }
    else // dispersion has a defined value
    {
        // Transmission function and group delay at the wavelength (computed once per
        // wavelength, F-15)
        const auto &S12_delay = m_S12_delay_cache.get(s.m_wavelength_id, [&]() {
            // calculate phase-delay
            const double neff = m_neff + m_dneff_dlambda * (s.getWavelength() - 1.55e-6)
                                 + m_d2neff_dlambda2_over_2 * pow(s.getWavelength() - 1.55e-6, 2);
            const double ng = m_ng + m_dng_dlambda * (s.getWavelength() - 1.55e-6);

            const double phase_delay_disp = m_phase_delay_factor * neff / s.getWavelength();
            const double group_delay_ns_disp = 1e9 * m_length_cm * 1e-2 / (c / ng);
            return std::make_pair(OpticalSignal::field_type(polar(m_transmission, phase_delay_disp)),
                                  sc_time(group_delay_ns_disp, SC_NS));
        });
        // Apply transmission function
        s *= S12_delay.first;

        // Get new ID for output event
        s.getNewId();

        // Write to ouput port after group delay
        out.delayedWrite(s, S12_delay.second);
    }
}

void WaveguideUni::start_of_simulation()
{
    // B-014: check once how the group delay is rounded to the time step. With dispersion
    // (m_D != 0) the delay depends on the wavelength: the nominal one (1.55 um) is checked.
    // Rounding to 0 is an error only for a waveguide written in the netlist
    m_out_writer.checkDelayRounding(name(), groupDelay_ns() * 1e-9, m_from_netlist);
    prepareTransmission();
}

void WaveguideUni::print_initialization()
{
    const double c = 299792458.0;

    cout << name() << ":" << endl;
    cout << "length = " << m_length_cm/100 << " m" << endl;
    cout << "neff = " << m_neff << "" << endl;
    cout << "ng = " << m_ng << "" << endl;
    cout << "transmission_power = " << pow(m_transmission, 2) << " W/W" << endl;
    cout << "group delay = " << groupDelay_ns() << " ns" << endl;
    cout << "phase delay = " << 1e9 * m_length_cm * 1e-2 / (c / m_neff) << " ns";
    cout << " ("
            << fmod((2 * M_PI * m_neff / 1.55e-6) * m_length_cm * 1e-2, 2 * M_PI)
            << "rad @1.55)" << endl;
    cout << port_channel_name(p_in);
    cout << " --> ";
    cout << port_channel_name(p_out);
    cout << endl;
    cout << endl;
}

void WaveguideUni::on_port_in_changed()
{
    // Read the input signal, write it transmitted after the group delay
    propagate(p_in->read(), m_out_writer);
}


void WaveguideBi::start_of_simulation()
{
    // B-014: as WaveguideUni. Same delay in both directions: checked (and reported) once
    m_p1_out_writer.checkDelayRounding(name(), groupDelay_ns() * 1e-9, m_from_netlist);
    prepareTransmission();
}

void WaveguideBi::print_initialization()
{
    const double c = 299792458.0;

    cout << name() << ":" << endl;
    cout << "length = " << m_length_cm/100 << " m" << endl;
    cout << "neff = " << m_neff << "" << endl;
    cout << "ng = " << m_ng << "" << endl;
    cout << "transmission_power = " << pow(m_transmission, 2) << " W/W" << endl;
    cout << "group delay = " << groupDelay_ns() << " ns" << endl;
    cout << "phase delay = " << 1e9 * m_length_cm * 1e-2 / (c / m_neff) << " ns";
    cout << " ("
            << fmod((2 * M_PI * m_neff / 1.55e-6) * m_length_cm * 1e-2, 2 * M_PI)
            << "rad @1.55)" << endl;
    cout << port_channel_name(p0_in);
    cout << " --> ";
    cout << port_channel_name(p1_out);
    cout << endl;
    cout << port_channel_name(p0_out);
    cout << " <-- ";
    cout << port_channel_name(p1_in);
    cout << endl;
    cout << endl;
}

void WaveguideBi::on_p0_in_changed()
{
    // Read the input signal, write it transmitted after the group delay
    propagate(p0_in->read(), m_p1_out_writer);
}

void WaveguideBi::on_p1_in_changed()
{
    // Read the input signal, write it transmitted after the group delay
    propagate(p1_in->read(), m_p0_out_writer);
}
