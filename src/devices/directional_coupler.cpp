#include "specs.h"
#include "devices/directional_coupler.h"

using namespace std;

void DirectionalCouplerBase::start_of_simulation()
{
    m_through_power_dB = 10*log10(m_dc_through_coupling_power) - m_dc_loss;
    m_cross_power_dB = 10*log10(1.0 - m_dc_through_coupling_power) - m_dc_loss;

    const double transmission_through = pow(10.0, m_through_power_dB / 20.0);
    const double transmission_cross = pow(10.0, m_cross_power_dB / 20.0);

    // Pre-calculate S-parameters (the same for both inputs: S13 = S24, S14 = S23)
    m_S_through = polar(transmission_through, m_through_phase_rad);
    m_S_cross = polar(transmission_cross, m_cross_phase_rad);

    // F-15: built once, not per event
    m_delay = sc_time(m_delay_ns, SC_NS);
}

void DirectionalCouplerUni::print_initialization()
{
    const OpticalSignal::field_type &S13 = m_S_through;
    const OpticalSignal::field_type &S14 = m_S_cross;

    cout << name() << ":" << endl;
    cout << "through_power = " << norm(S13) << " W/W" << endl;
    cout << "cross_power = " << norm(S14)<< " W/W" << endl;
    cout << "through_field = " << abs(S13) << "" << endl;
    cout << "cross_field = " << abs(S14)<< "" << endl;
    cout << "insertion loss = " << m_dc_loss << "dB" << endl;
    cout << port_channel_name(p_in1);
    cout << " --,__,-> ";
    cout << port_channel_name(p_out1);
    cout << endl;

    cout << port_channel_name(p_in2);
    cout << " --'  '-> ";
    cout << port_channel_name(p_out2);
    cout << endl;

    cout << endl;
}

void DirectionalCouplerUni::on_port_in1_changed()
{
    const OpticalSignal::field_type &S13 = m_S_through;
    const OpticalSignal::field_type &S24 = m_S_through;
    const OpticalSignal::field_type &S14 = m_S_cross;
    const OpticalSignal::field_type &S23 = m_S_cross;

    // Read current inputs
    OpticalSignal p_in1_read = p_in1->read();

    auto cur_wavelength_id = p_in1_read.m_wavelength_id;
    // Updating the field memory
    m_memory_in1[cur_wavelength_id] = p_in1_read.m_field;

    auto s3 = OpticalSignal(m_memory_in1[cur_wavelength_id] * S13 +
                    m_memory_in2[cur_wavelength_id] * S23
                    , cur_wavelength_id);

    auto s4 = OpticalSignal(m_memory_in1[cur_wavelength_id] * S14 +
                            m_memory_in2[cur_wavelength_id] * S24
                            , cur_wavelength_id);

    m_out1_writer.delayedWrite(s3, m_delay);
    m_out2_writer.delayedWrite(s4, m_delay);
}

void DirectionalCouplerUni::on_port_in2_changed()
{
    const OpticalSignal::field_type &S13 = m_S_through;
    const OpticalSignal::field_type &S24 = m_S_through;
    const OpticalSignal::field_type &S14 = m_S_cross;
    const OpticalSignal::field_type &S23 = m_S_cross;

    // Read current inputs
    OpticalSignal p_in2_read = p_in2->read();

    auto cur_wavelength_id = p_in2_read.m_wavelength_id;
    // Updating the field memory
    m_memory_in2[cur_wavelength_id] = p_in2_read.m_field;

    auto s3 = OpticalSignal(m_memory_in1[cur_wavelength_id] * S13 +
                            m_memory_in2[cur_wavelength_id] * S23
                    , cur_wavelength_id);

    auto s4 = OpticalSignal(m_memory_in1[cur_wavelength_id] * S14 +
                            m_memory_in2[cur_wavelength_id] * S24
                            , cur_wavelength_id);

    m_out1_writer.delayedWrite(s3, m_delay);
    m_out2_writer.delayedWrite(s4, m_delay);
}


void DirectionalCouplerBi::print_initialization()
{
    const OpticalSignal::field_type &S02 = m_S_through;
    const OpticalSignal::field_type &S03 = m_S_cross;

    cout << name() << ":" << endl;
    cout << "through_power = " << norm(S02) << " W/W" << endl;
    cout << "cross_power = " << norm(S03)<< " W/W" << endl;
    cout << "through_field = " << abs(S02) << "" << endl;
    cout << "cross_field = " << abs(S03)<< "" << endl;
    cout << "insertion loss = " << m_dc_loss << "dB" << endl;
    cout << port_channel_name(p0_in);

    cout << " --,__,-> ";
    cout << port_channel_name(p2_out);
    cout << endl;
    cout << port_channel_name(p1_in);
    cout << " --'  '-> ";
    cout << port_channel_name(p3_out);
    cout << endl;

    cout << port_channel_name(p0_out);
    cout << " <-,__,-- ";
    cout << port_channel_name(p2_in);
    cout << endl;
    cout << port_channel_name(p1_out);
    cout << " <-'  '-- ";
    cout << port_channel_name(p3_in);
    cout << endl;

    cout << endl;
}

void DirectionalCouplerBi::on_p0_in_changed()
{
    const OpticalSignal::field_type &S02 = m_S_through;
    const OpticalSignal::field_type &S13 = m_S_through;
    const OpticalSignal::field_type &S03 = m_S_cross;
    const OpticalSignal::field_type &S12 = m_S_cross;

    // Read current inputs
    OpticalSignal p0_in_read = p0_in->read();

    auto cur_wavelength_id = p0_in_read.m_wavelength_id;

    // Updating the field memory
    m_memory_in0[cur_wavelength_id] = p0_in_read.m_field;

    auto s2 = OpticalSignal(m_memory_in0[cur_wavelength_id] * S02 +
                            m_memory_in1[cur_wavelength_id] * S12
                            , cur_wavelength_id);

    auto s3 = OpticalSignal(m_memory_in0[cur_wavelength_id] * S03 +
                            m_memory_in1[cur_wavelength_id] * S13
                            , cur_wavelength_id);

    m_p2_out_writer.delayedWrite(s2, m_delay);
    m_p3_out_writer.delayedWrite(s3, m_delay);
}

void DirectionalCouplerBi::on_p1_in_changed()
{
    const OpticalSignal::field_type &S02 = m_S_through;
    const OpticalSignal::field_type &S13 = m_S_through;
    const OpticalSignal::field_type &S03 = m_S_cross;
    const OpticalSignal::field_type &S12 = m_S_cross;

    // Read current inputs
    OpticalSignal p1_in_read = p1_in->read();

    auto cur_wavelength_id = p1_in_read.m_wavelength_id;

    // Updating the field memory
    m_memory_in1[cur_wavelength_id] = p1_in_read.m_field;

    auto s2 = OpticalSignal(m_memory_in0[cur_wavelength_id] * S02 +
                            m_memory_in1[cur_wavelength_id] * S12
                            , cur_wavelength_id);

    auto s3 = OpticalSignal(m_memory_in0[cur_wavelength_id] * S03 +
                            m_memory_in1[cur_wavelength_id] * S13
                            , cur_wavelength_id);

    m_p2_out_writer.delayedWrite(s2, m_delay);
    m_p3_out_writer.delayedWrite(s3, m_delay);
}

void DirectionalCouplerBi::on_p2_in_changed()
{
    const OpticalSignal::field_type &S02 = m_S_through;
    const OpticalSignal::field_type &S13 = m_S_through;
    const OpticalSignal::field_type &S03 = m_S_cross;
    const OpticalSignal::field_type &S12 = m_S_cross;

    // Read current inputs
    OpticalSignal p2_in_read = p2_in->read();

    auto cur_wavelength_id = p2_in_read.m_wavelength_id;

    // Updating the field memory
    m_memory_in2[cur_wavelength_id] = p2_in_read.m_field;

    auto s0 = OpticalSignal(m_memory_in2[cur_wavelength_id] * S02 +
                            m_memory_in3[cur_wavelength_id] * S03
                            , cur_wavelength_id);

    auto s1 = OpticalSignal(m_memory_in2[cur_wavelength_id] * S12 +
                            m_memory_in3[cur_wavelength_id] * S13
                            , cur_wavelength_id);

    m_p0_out_writer.delayedWrite(s0, m_delay);
    m_p1_out_writer.delayedWrite(s1, m_delay);
}

void DirectionalCouplerBi::on_p3_in_changed()
{
    const OpticalSignal::field_type &S02 = m_S_through;
    const OpticalSignal::field_type &S13 = m_S_through;
    const OpticalSignal::field_type &S03 = m_S_cross;
    const OpticalSignal::field_type &S12 = m_S_cross;

    // Read current inputs
    OpticalSignal p3_in_read = p3_in->read();

    auto cur_wavelength_id = p3_in_read.m_wavelength_id;

    // Updating the field memory
    m_memory_in3[cur_wavelength_id] = p3_in_read.m_field;

    auto s0 = OpticalSignal(m_memory_in2[cur_wavelength_id] * S02 +
                            m_memory_in3[cur_wavelength_id] * S03
                            , cur_wavelength_id);

    auto s1 = OpticalSignal(m_memory_in2[cur_wavelength_id] * S12 +
                            m_memory_in3[cur_wavelength_id] * S13
                            , cur_wavelength_id);

    m_p0_out_writer.delayedWrite(s0, m_delay);
    m_p1_out_writer.delayedWrite(s1, m_delay);
}
