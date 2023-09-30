#include "specs.h"
#include <waveguide.h>

using namespace std;

void WaveguideUni::on_port_in_changed()
{
    const double c = 299792458.0;

    // attenuation in dB
    m_attenuation_dB = m_attenuation_dB_cm * m_length_cm;

    // transmission in field: 10^(-dB/20)
    const double transmission = pow(10.0, - m_attenuation_dB / 20.0);

    // vg = c/ng
    // => delay = L / vg = (L * ng) / c
    const double group_delay_ns = 1e9 * m_length_cm * 1e-2 / (c / m_ng);

    // precalculate 2 * pi * L
    double phase_delay_factor = 2.0 * M_PI * m_length_cm * 1e-2;

    // ng = neff - lambda * dneff/dlambda
    // => dneff/dlambda = (neff - ng)/lambda
    double dneff_dlambda = (m_neff - m_ng) / 1.55e-6;

    // Parameters relative to dispersion
    double d2neff_dlambda2_over_2 = -1 * c * m_D / (2 * 1.55e-6);
    double dng_dlambda = c*m_D;

    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << ":" << endl;
        cout << "length = " << m_length_cm/100 << " m" << endl;
        cout << "neff = " << m_neff << "" << endl;
        cout << "ng = " << m_ng << "" << endl;
        cout << "transmission_power = " << pow(transmission, 2) << " W/W" << endl;
        cout << "group delay = " << group_delay_ns << " ns" << endl;
        // cout << "dneff/dlambda = " << dneff_dlambda*1e-6 << " um^-1" << endl;
        cout << "phase delay = " << 1e9 * m_length_cm * 1e-2 / (c / m_neff) << " ns";
        cout << " ("
                << fmod((2 * M_PI * m_neff / 1.55e-6) * m_length_cm * 1e-2, 2 * M_PI)
                << "rad @1.55)" << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p_in.get_interface()))->name();
        cout << " --> ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p_out.get_interface()))->name();
        cout << endl;
        cout << endl;
    }

    if(m_D == 0)
    {
        while (true) {
            // Wait for a new input signal
            wait();
            // Read the input signal
            auto s = p_in->read();

            // calculate phase-delay
            const double neff = m_neff + dneff_dlambda * (s.getWavelength() - 1.55e-6);

            const double phase_delay = phase_delay_factor * neff / s.getWavelength();

            // Apply transmission function
            const OpticalSignal::field_type S12 = polar(transmission, phase_delay);
            s *= S12;

            // Get new ID for output event
            s.getNewId();

            // Write to ouput port after group delay
            m_out_writer.delayedWrite(s, sc_time(group_delay_ns, SC_NS));
        }
    }
    else // dispersion has a defined value
    {
        while (true) {
            // Wait for a new input signal
            wait();
            // Read the input signal
            auto s = p_in->read();

            // calculate phase-delay
            const double neff = m_neff + dneff_dlambda * (s.getWavelength() - 1.55e-6)
                                 + d2neff_dlambda2_over_2 * pow(s.getWavelength() - 1.55e-6, 2);
            const double ng = m_ng + dng_dlambda * (s.getWavelength() - 1.55e-6);

            const double phase_delay_disp = phase_delay_factor * neff / s.getWavelength();
            const double group_delay_ns_disp = 1e9 * m_length_cm * 1e-2 / (c / ng);
            // Apply transmission function
            const OpticalSignal::field_type S12 = polar(transmission, phase_delay_disp);
            s *= S12;

            // Get new ID for output event
            s.getNewId();

            // Write to ouput port after group delay
            m_out_writer.delayedWrite(s, sc_time(group_delay_ns_disp, SC_NS));
        }
    }
}


void WaveguideBi::on_p0_in_changed()
{
    const double c = 299792458.0;

    // attenuation in dB
    m_attenuation_dB = m_attenuation_dB_cm * m_length_cm;

    // transmission in field: 10^(-dB/20)
    const double transmission = pow(10.0, - m_attenuation_dB / 20.0);

    // vg = c/ng
    // => delay = L / vg = (L * ng) / c
    const double group_delay_ns = 1e9 * m_length_cm * 1e-2 / (c / m_ng);

    // precalculate 2 * pi * L
    double phase_delay_factor = 2.0 * M_PI * m_length_cm * 1e-2;

    // ng = neff - lambda * dneff/dlambda
    // => dneff/dlambda = (neff - ng)/lambda
    double dneff_dlambda = (m_neff - m_ng) / 1.55e-6;

    // Parameters relative to dispersion
    double d2neff_dlambda2_over_2 = -1 * c * m_D / (2 * 1.55e-6);
    double dng_dlambda = c*m_D;

    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << ":" << endl;
        cout << "length = " << m_length_cm/100 << " m" << endl;
        cout << "neff = " << m_neff << "" << endl;
        cout << "ng = " << m_ng << "" << endl;
        cout << "transmission_power = " << pow(transmission, 2) << " W/W" << endl;
        cout << "group delay = " << group_delay_ns << " ns" << endl;
        // cout << "dneff/dlambda = " << dneff_dlambda*1e-6 << " um^-1" << endl;
        cout << "phase delay = " << 1e9 * m_length_cm * 1e-2 / (c / m_neff) << " ns";
        cout << " ("
                << fmod((2 * M_PI * m_neff / 1.55e-6) * m_length_cm * 1e-2, 2 * M_PI)
                << "rad @1.55)" << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p0_in.get_interface()))->name();
        cout << " --> ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p1_out.get_interface()))->name();
        cout << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p0_out.get_interface()))->name();
        cout << " <-- ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p1_in.get_interface()))->name();
        cout << endl;
        cout << endl;
    }

    if(m_D == 0)
    {
        while (true) {
            // Wait for a new input signal
            wait();
            // Read the input signal
            auto s = p0_in->read();

            // calculate phase-delay
            const double neff = m_neff + dneff_dlambda * (s.getWavelength() - 1.55e-6);

            const double phase_delay = phase_delay_factor * neff / s.getWavelength();

            // Apply transmission function
            const OpticalSignal::field_type S12 = polar(transmission, phase_delay);
            s *= S12;

            // Get new ID for output event
            s.getNewId();

            // Write to ouput port after group delay
            m_p1_out_writer.delayedWrite(s, sc_time(group_delay_ns, SC_NS));
        }
    }
    else // dispersion has a defined value
    {
        while (true) {
            // Wait for a new input signal
            wait();
            // Read the input signal
            auto s = p0_in->read();

            // calculate phase-delay
            const double neff = m_neff + dneff_dlambda * (s.getWavelength() - 1.55e-6)
                                 + d2neff_dlambda2_over_2 * pow(s.getWavelength() - 1.55e-6, 2);
            const double ng = m_ng + dng_dlambda * (s.getWavelength() - 1.55e-6);

            const double phase_delay_disp = phase_delay_factor * neff / s.getWavelength();
            const double group_delay_ns_disp = 1e9 * m_length_cm * 1e-2 / (c / ng);
            // Apply transmission function
            const OpticalSignal::field_type S12 = polar(transmission, phase_delay_disp);
            s *= S12;

            // Get new ID for output event
            s.getNewId();

            // Write to ouput port after group delay
            m_p1_out_writer.delayedWrite(s, sc_time(group_delay_ns_disp, SC_NS));
        }
    }
}

void WaveguideBi::on_p1_in_changed()
{
    const double c = 299792458.0;

    // attenuation in dB
    m_attenuation_dB = m_attenuation_dB_cm * m_length_cm;

    // transmission in field: 10^(-dB/20)
    const double transmission = pow(10.0, - m_attenuation_dB / 20.0);

    // vg = c/ng
    // => delay = L / vg = (L * ng) / c
    const double group_delay_ns = 1e9 * m_length_cm * 1e-2 / (c / m_ng);

    // precalculate 2 * pi * L
    double phase_delay_factor = 2.0 * M_PI * m_length_cm * 1e-2;

    // ng = neff - lambda * dneff/dlambda
    // => dneff/dlambda = (neff - ng)/lambda
    double dneff_dlambda = (m_neff - m_ng) / 1.55e-6;

    // Parameters relative to dispersion
    double d2neff_dlambda2_over_2 = -1 * c * m_D / (2 * 1.55e-6);
    double dng_dlambda = c*m_D;

    if(m_D == 0)
    {
        while (true) {
            // Wait for a new input signal
            wait();
            // Read the input signal
            auto s = p1_in->read();

            // calculate phase-delay
            const double neff = m_neff + dneff_dlambda * (s.getWavelength() - 1.55e-6);

            const double phase_delay = phase_delay_factor * neff / s.getWavelength();

            // Apply transmission function
            const OpticalSignal::field_type S12 = polar(transmission, phase_delay);
            s *= S12;

            // Get new ID for output event
            s.getNewId();

            // Write to ouput port after group delay
            m_p0_out_writer.delayedWrite(s, sc_time(group_delay_ns, SC_NS));
        }
    }
    else // dispersion has a defined value
    {
        while (true) {
            // Wait for a new input signal
            wait();
            // Read the input signal
            auto s = p1_in->read();

            // calculate phase-delay
            const double neff = m_neff + dneff_dlambda * (s.getWavelength() - 1.55e-6)
                                 + d2neff_dlambda2_over_2 * pow(s.getWavelength() - 1.55e-6, 2);
            const double ng = m_ng + dng_dlambda * (s.getWavelength() - 1.55e-6);

            const double phase_delay_disp = phase_delay_factor * neff / s.getWavelength();
            const double group_delay_ns_disp = 1e9 * m_length_cm * 1e-2 / (c / ng);
            // Apply transmission function
            const OpticalSignal::field_type S12 = polar(transmission, phase_delay_disp);
            s *= S12;

            // Get new ID for output event
            s.getNewId();

            // Write to ouput port after group delay
            m_p0_out_writer.delayedWrite(s, sc_time(group_delay_ns_disp, SC_NS));
        }
    }
}