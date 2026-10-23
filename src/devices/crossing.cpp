#include "specs.h"
#include "devices/crossing.h"

using namespace std;

void CrossingBase::s_parameters(OpticalSignal::field_type &through, OpticalSignal::field_type &cross) const
{
    // Crosstalk field ratio x. If it's NAN, it's because it was not specified and thus the
    // linear should be zero (-inf dB). Its sign is ignored (B-032): a positive value is read
    // as an isolation, so xtalk = 20 and xtalk = -20 both put 1 % of the power on the cross port
    const double crosstalk_field_lin = (isnan(m_crosstalk_power_dB)) ? 0 : pow(10.0, -fabs(m_crosstalk_power_dB) / 20);
    // Through field ratio t: excess loss (att) on top of the power lost to crosstalk,
    // i.e. |t|^2 = 10^(-att/10) (1 - |x|^2) and |t|^2 + |x|^2 <= 1 for att >= 0
    const double transmission_field_lin = pow(10.0, -m_attenuation_power_dB / 20)
                                          * sqrt(1 - crosstalk_field_lin * crosstalk_field_lin);

    through = polar(transmission_field_lin, 0.0);
    cross = polar(crosstalk_field_lin, M_PI / 2); // in quadrature, as in the directional coupler
}

void CrossingBase::start_of_simulation()
{
    // Pre-calculate S-parameters
    s_parameters(m_through, m_cross);
}

void CrossingUni::write_outputs(uint32_t wavelength_id,
                                const OpticalSignal::field_type &through,
                                const OpticalSignal::field_type &cross)
{
    const auto &in1 = m_memory_in1[wavelength_id];
    const auto &in2 = m_memory_in2[wavelength_id];

    // Apply S-parameters (at this wavelength only)
    auto s3 = OpticalSignal(in1 * through + in2 * cross, wavelength_id);
    auto s4 = OpticalSignal(in1 * cross + in2 * through, wavelength_id);

    // Write to ouput port after delay
    m_out1_writer.delayedWrite(s3, sc_time(0, SC_NS));
    m_out2_writer.delayedWrite(s4, sc_time(0, SC_NS));
}

void CrossingUni::print_initialization()
{
    const OpticalSignal::field_type &through = m_through;
    const OpticalSignal::field_type &cross = m_cross;

    cout << name() << ":" << endl;
    cout << "transmission_power = " << norm(through) << " W/W" << endl;
    cout << "crosstalk_power = " << norm(cross)<< " W/W" << endl;
    cout << port_channel_name(p_in1);
    cout << " ---   ---> ";
    cout << port_channel_name(p_out1);
    cout << endl;
    cout << port_channel_name(p_in2);
    cout << " ---   ---> ";
    cout << port_channel_name(p_out2);
    cout << endl;

    cout << endl;
}

void CrossingUni::on_port_in1_changed()
{
    // Read current input and update the field memory at its wavelength
    OpticalSignal p_in1_read = p_in1->read();
    auto cur_wavelength_id = p_in1_read.m_wavelength_id;
    m_memory_in1[cur_wavelength_id] = p_in1_read.m_field;

    write_outputs(cur_wavelength_id, m_through, m_cross);
}

void CrossingUni::on_port_in2_changed()
{
    // Read current input and update the field memory at its wavelength
    OpticalSignal p_in2_read = p_in2->read();
    auto cur_wavelength_id = p_in2_read.m_wavelength_id;
    m_memory_in2[cur_wavelength_id] = p_in2_read.m_field;

    write_outputs(cur_wavelength_id, m_through, m_cross);
}

void CrossingBi::write_forward_outputs(uint32_t wavelength_id,
                                       const OpticalSignal::field_type &through,
                                       const OpticalSignal::field_type &cross)
{
    const auto &in0 = m_memory_in0[wavelength_id];
    const auto &in1 = m_memory_in1[wavelength_id];

    // Apply S-parameters (at this wavelength only)
    // p0 -> p2 through, p1 -> p2 cross; p0 -> p3 cross, p1 -> p3 through
    auto s2_out = OpticalSignal(in0 * through + in1 * cross, wavelength_id);
    auto s3_out = OpticalSignal(in0 * cross + in1 * through, wavelength_id);

    // Write to ouput port after delay
    m_p2_out_writer.delayedWrite(s2_out, sc_time(0, SC_NS));
    m_p3_out_writer.delayedWrite(s3_out, sc_time(0, SC_NS));
}

void CrossingBi::write_backward_outputs(uint32_t wavelength_id,
                                        const OpticalSignal::field_type &through,
                                        const OpticalSignal::field_type &cross)
{
    const auto &in2 = m_memory_in2[wavelength_id];
    const auto &in3 = m_memory_in3[wavelength_id];

    // Apply S-parameters (reciprocal: the same as forward, at this wavelength only)
    // p2 -> p0 through, p3 -> p0 cross; p2 -> p1 cross, p3 -> p1 through
    auto s0_out = OpticalSignal(in2 * through + in3 * cross, wavelength_id);
    auto s1_out = OpticalSignal(in2 * cross + in3 * through, wavelength_id);

    // Write to ouput port after delay
    m_p0_out_writer.delayedWrite(s0_out, sc_time(0, SC_NS));
    m_p1_out_writer.delayedWrite(s1_out, sc_time(0, SC_NS));
}

void CrossingBi::print_initialization()
{
    const OpticalSignal::field_type &through = m_through;
    const OpticalSignal::field_type &cross = m_cross;

    cout << name() << ":" << endl;
    cout << "transmission_power = " << norm(through) << " W/W" << endl;
    cout << "crosstalk_power = " << norm(cross)<< " W/W" << endl;
    cout << port_channel_name(p0_in);
    cout << " ---   ---> ";
    cout << port_channel_name(p2_out);
    cout << endl;
    cout << port_channel_name(p1_in);
    cout << " ---   ---> ";
    cout << port_channel_name(p3_out);
    cout << endl;
    cout << port_channel_name(p2_in);
    cout << " ---   ---> ";
    cout << port_channel_name(p0_out);
    cout << endl;
    cout << port_channel_name(p3_in);
    cout << " ---   ---> ";
    cout << port_channel_name(p1_out);
    cout << endl;

    cout << endl;
}

void CrossingBi::on_p0_in_changed()
{
    // Read current input and update the field memory at its wavelength
    OpticalSignal p0_in_read = p0_in->read();
    auto cur_wavelength_id = p0_in_read.m_wavelength_id;
    m_memory_in0[cur_wavelength_id] = p0_in_read.m_field;

    write_forward_outputs(cur_wavelength_id, m_through, m_cross);
}

void CrossingBi::on_p1_in_changed()
{
    // Read current input and update the field memory at its wavelength
    OpticalSignal p1_in_read = p1_in->read();
    auto cur_wavelength_id = p1_in_read.m_wavelength_id;
    m_memory_in1[cur_wavelength_id] = p1_in_read.m_field;

    write_forward_outputs(cur_wavelength_id, m_through, m_cross);
}

void CrossingBi::on_p2_in_changed()
{
    // Read current input and update the field memory at its wavelength
    OpticalSignal p2_in_read = p2_in->read();
    auto cur_wavelength_id = p2_in_read.m_wavelength_id;
    m_memory_in2[cur_wavelength_id] = p2_in_read.m_field;

    write_backward_outputs(cur_wavelength_id, m_through, m_cross);
}

void CrossingBi::on_p3_in_changed()
{
    // Read current input and update the field memory at its wavelength
    OpticalSignal p3_in_read = p3_in->read();
    auto cur_wavelength_id = p3_in_read.m_wavelength_id;
    m_memory_in3[cur_wavelength_id] = p3_in_read.m_field;

    write_backward_outputs(cur_wavelength_id, m_through, m_cross);
}
