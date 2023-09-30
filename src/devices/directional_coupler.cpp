#include "specs.h"
#include <directional_coupler.h>

using namespace std;

void DirectionalCouplerUni::on_port_in1_changed()
{
    m_through_power_dB = 10*log10(m_dc_through_coupling_power) - m_dc_loss;
    m_cross_power_dB = 10*log10(1.0 - m_dc_through_coupling_power) - m_dc_loss;

    m_memory_in1[0] = 0; // initializing for nan wavelength

    const double transmission_through = pow(10.0, m_through_power_dB / 20.0);
    const double transmission_cross = pow(10.0, m_cross_power_dB / 20.0);

    // Pre-calculate S-parameters
    OpticalSignal::field_type S13, S14, S23, S24;
    S13 = polar(transmission_through, m_through_phase_rad);
    S24 = polar(transmission_through, m_through_phase_rad);
    S14 = polar(transmission_cross, m_cross_phase_rad);
    S23 = polar(transmission_cross, m_cross_phase_rad);

    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << ":" << endl;
        cout << "through_power = " << norm(S13) << " W/W" << endl;
        cout << "cross_power = " << norm(S14)<< " W/W" << endl;
        cout << "through_field = " << abs(S13) << "" << endl;
        cout << "cross_field = " << abs(S14)<< "" << endl;
        cout << "insertion loss = " << m_dc_loss << "dB" << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p_in1.get_interface()))->name();
        cout << " --,__,-> ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p_out1.get_interface()))->name();
        cout << endl;

        cout << (dynamic_cast<spx::oa_signal_type *>(p_in2.get_interface()))->name();
        cout << " --'  '-> ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p_out2.get_interface()))->name();
        cout << endl;

        cout << endl;
    }

    OpticalSignal p_in1_read;

    while (true) {
        // Wait for a new input signal
        wait();

        // Read current inputs
        p_in1_read = p_in1->read();

        auto cur_wavelength_id = p_in1_read.m_wavelength_id;
        // Updating the field memory
        m_memory_in1[cur_wavelength_id] = p_in1_read.m_field;

        auto s3 = OpticalSignal(m_memory_in1[cur_wavelength_id] * S13 +
                        m_memory_in2[cur_wavelength_id] * S23
                        , cur_wavelength_id);

        auto s4 = OpticalSignal(m_memory_in1[cur_wavelength_id] * S14 +
                                m_memory_in2[cur_wavelength_id] * S24
                                , cur_wavelength_id);

        m_out1_writer.delayedWrite(s3, sc_time(m_delay_ns, SC_NS));
        m_out2_writer.delayedWrite(s4, sc_time(m_delay_ns, SC_NS));
    }
}

void DirectionalCouplerUni::on_port_in2_changed()
{
    m_through_power_dB = 10*log10(m_dc_through_coupling_power) - m_dc_loss;
    m_cross_power_dB = 10*log10(1.0 - m_dc_through_coupling_power) - m_dc_loss;

    m_memory_in2[0] = 0; // initializing for nan wavelength

    const double transmission_through = pow(10.0, m_through_power_dB / 20.0);
    const double transmission_cross = pow(10.0, m_cross_power_dB / 20.0);

    // Pre-calculate S-parameters
    OpticalSignal::field_type S13, S14, S23, S24;
    S13 = polar(transmission_through, m_through_phase_rad);
    S24 = polar(transmission_through, m_through_phase_rad);
    S14 = polar(transmission_cross, m_cross_phase_rad);
    S23 = polar(transmission_cross, m_cross_phase_rad);

    OpticalSignal p_in2_read;

    while (true) {
        // Wait for a new input signal
        wait();

        // Read current inputs
        p_in2_read = p_in2->read();

        auto cur_wavelength_id = p_in2_read.m_wavelength_id;
        // Updating the field memory
        m_memory_in2[cur_wavelength_id] = p_in2_read.m_field;

        auto s3 = OpticalSignal(m_memory_in1[cur_wavelength_id] * S13 +
                                m_memory_in2[cur_wavelength_id] * S23
                        , cur_wavelength_id);

        auto s4 = OpticalSignal(m_memory_in1[cur_wavelength_id] * S14 +
                                m_memory_in2[cur_wavelength_id] * S24
                                , cur_wavelength_id);

        m_out1_writer.delayedWrite(s3, sc_time(m_delay_ns, SC_NS));
        m_out2_writer.delayedWrite(s4, sc_time(m_delay_ns, SC_NS));
    }
}


void DirectionalCouplerBi::on_p0_in_changed()
{
    m_through_power_dB = 10*log10(m_dc_through_coupling_power) - m_dc_loss;
    m_cross_power_dB = 10*log10(1.0 - m_dc_through_coupling_power) - m_dc_loss;

    m_memory_in0[0] = 0; // initializing for nan wavelength

    const double transmission_through = pow(10.0, m_through_power_dB / 20.0);
    const double transmission_cross = pow(10.0, m_cross_power_dB / 20.0);

    // Pre-calculate S-parameters
    OpticalSignal::field_type S02, S13, S03, S12;
    S02 = polar(transmission_through, m_through_phase_rad);
    S13 = polar(transmission_through, m_through_phase_rad);
    S03 = polar(transmission_cross, m_cross_phase_rad);
    S12 = polar(transmission_cross, m_cross_phase_rad);

    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << ":" << endl;
        cout << "through_power = " << norm(S02) << " W/W" << endl;
        cout << "cross_power = " << norm(S03)<< " W/W" << endl;
        cout << "through_field = " << abs(S02) << "" << endl;
        cout << "cross_field = " << abs(S03)<< "" << endl;
        cout << "insertion loss = " << m_dc_loss << "dB" << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p0_in.get_interface()))->name();

        cout << " --,__,-> ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p2_out.get_interface()))->name();
        cout << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p1_in.get_interface()))->name();
        cout << " --'  '-> ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p3_out.get_interface()))->name();
        cout << endl;

        cout << (dynamic_cast<spx::oa_signal_type *>(p0_out.get_interface()))->name();
        cout << " <-,__,-- ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p2_in.get_interface()))->name();
        cout << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p1_out.get_interface()))->name();
        cout << " <-'  '-- ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p3_in.get_interface()))->name();
        cout << endl;

        cout << endl;
    }

    OpticalSignal p0_in_read;

    while (true) {
        // Wait for a new input signal
        wait();

        // Read current inputs
        p0_in_read = p0_in->read();

        auto cur_wavelength_id = p0_in_read.m_wavelength_id;

        // Updating the field memory
        m_memory_in0[cur_wavelength_id] = p0_in_read.m_field;

        auto s2 = OpticalSignal(m_memory_in0[cur_wavelength_id] * S02 +
                                m_memory_in1[cur_wavelength_id] * S12
                                , cur_wavelength_id);

        auto s3 = OpticalSignal(m_memory_in0[cur_wavelength_id] * S03 +
                                m_memory_in1[cur_wavelength_id] * S13
                                , cur_wavelength_id);

        m_p2_out_writer.delayedWrite(s2, sc_time(m_delay_ns, SC_NS));
        m_p3_out_writer.delayedWrite(s3, sc_time(m_delay_ns, SC_NS));
    }
}

void DirectionalCouplerBi::on_p1_in_changed()
{
    m_through_power_dB = 10*log10(m_dc_through_coupling_power) - m_dc_loss;
    m_cross_power_dB = 10*log10(1.0 - m_dc_through_coupling_power) - m_dc_loss;

    m_memory_in1[0] = 0; // initializing for nan wavelength

    const double transmission_through = pow(10.0, m_through_power_dB / 20.0);
    const double transmission_cross = pow(10.0, m_cross_power_dB / 20.0);

    // Pre-calculate S-parameters
    OpticalSignal::field_type S02, S13, S03, S12;
    S02 = polar(transmission_through, m_through_phase_rad);
    S13 = polar(transmission_through, m_through_phase_rad);
    S03 = polar(transmission_cross, m_cross_phase_rad);
    S12 = polar(transmission_cross, m_cross_phase_rad);

    OpticalSignal p1_in_read;

    while (true) {
        // Wait for a new input signal
        wait();

        // Read current inputs
        p1_in_read = p1_in->read();

        auto cur_wavelength_id = p1_in_read.m_wavelength_id;

        // Updating the field memory
        m_memory_in1[cur_wavelength_id] = p1_in_read.m_field;

        auto s2 = OpticalSignal(m_memory_in0[cur_wavelength_id] * S02 +
                                m_memory_in1[cur_wavelength_id] * S12
                                , cur_wavelength_id);

        auto s3 = OpticalSignal(m_memory_in0[cur_wavelength_id] * S03 +
                                m_memory_in1[cur_wavelength_id] * S13
                                , cur_wavelength_id);

        m_p2_out_writer.delayedWrite(s2, sc_time(m_delay_ns, SC_NS));
        m_p3_out_writer.delayedWrite(s3, sc_time(m_delay_ns, SC_NS));
    }
}

void DirectionalCouplerBi::on_p2_in_changed()
{
    m_through_power_dB = 10*log10(m_dc_through_coupling_power) - m_dc_loss;
    m_cross_power_dB = 10*log10(1.0 - m_dc_through_coupling_power) - m_dc_loss;

    m_memory_in2[0] = 0; // initializing for nan wavelength

    const double transmission_through = pow(10.0, m_through_power_dB / 20.0);
    const double transmission_cross = pow(10.0, m_cross_power_dB / 20.0);

    // Pre-calculate S-parameters
    OpticalSignal::field_type S02, S13, S03, S12;
    S02 = polar(transmission_through, m_through_phase_rad);
    S13 = polar(transmission_through, m_through_phase_rad);
    S03 = polar(transmission_cross, m_cross_phase_rad);
    S12 = polar(transmission_cross, m_cross_phase_rad);

    OpticalSignal p2_in_read;

    while (true) {
        // Wait for a new input signal
        wait();

        // Read current inputs
        p2_in_read = p2_in->read();

        auto cur_wavelength_id = p2_in_read.m_wavelength_id;

        // Updating the field memory
        m_memory_in2[cur_wavelength_id] = p2_in_read.m_field;

        auto s0 = OpticalSignal(m_memory_in2[cur_wavelength_id] * S02 +
                                m_memory_in3[cur_wavelength_id] * S03
                                , cur_wavelength_id);

        auto s1 = OpticalSignal(m_memory_in2[cur_wavelength_id] * S12 +
                                m_memory_in3[cur_wavelength_id] * S13
                                , cur_wavelength_id);

        m_p0_out_writer.delayedWrite(s0, sc_time(m_delay_ns, SC_NS));
        m_p1_out_writer.delayedWrite(s1, sc_time(m_delay_ns, SC_NS));
    }
}

void DirectionalCouplerBi::on_p3_in_changed()
{
    m_through_power_dB = 10*log10(m_dc_through_coupling_power) - m_dc_loss;
    m_cross_power_dB = 10*log10(1.0 - m_dc_through_coupling_power) - m_dc_loss;

    m_memory_in3[0] = 0; // initializing for nan wavelength

    const double transmission_through = pow(10.0, m_through_power_dB / 20.0);
    const double transmission_cross = pow(10.0, m_cross_power_dB / 20.0);

    // Pre-calculate S-parameters
    OpticalSignal::field_type S02, S13, S03, S12;
    S02 = polar(transmission_through, m_through_phase_rad);
    S13 = polar(transmission_through, m_through_phase_rad);
    S03 = polar(transmission_cross, m_cross_phase_rad);
    S12 = polar(transmission_cross, m_cross_phase_rad);

    OpticalSignal p3_in_read;

    while (true) {
        // Wait for a new input signal
        wait();

        // Read current inputs
        p3_in_read = p3_in->read();

        auto cur_wavelength_id = p3_in_read.m_wavelength_id;

        // Updating the field memory
        m_memory_in3[cur_wavelength_id] = p3_in_read.m_field;

        auto s0 = OpticalSignal(m_memory_in2[cur_wavelength_id] * S02 +
                                m_memory_in3[cur_wavelength_id] * S03
                                , cur_wavelength_id);

        auto s1 = OpticalSignal(m_memory_in2[cur_wavelength_id] * S12 +
                                m_memory_in3[cur_wavelength_id] * S13
                                , cur_wavelength_id);

        m_p0_out_writer.delayedWrite(s0, sc_time(m_delay_ns, SC_NS));
        m_p1_out_writer.delayedWrite(s1, sc_time(m_delay_ns, SC_NS));
    }
}