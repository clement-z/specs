#include "devices/merger.h"
#include "specs.h"

using namespace std;

void Merger::start_of_simulation()
{
    m_transmission = pow(10.0, - m_attenuation_dB / 20) / sqrt(2);
}

void Merger::print_initialization()
{
    cout << name() << ":" << endl;
    cout << "transmission = " << pow(m_transmission, 2) << " W/W" << endl;
    cout << port_channel_name(p_in1) << endl;
    cout << "\t --> \t" << port_channel_name(p_out) << endl;
    cout << port_channel_name(p_in2);
    cout << endl;
    cout << endl;
}

void Merger::on_port_in1_changed()
{
    // Read sum of input signals
    OpticalSignal p_in1_read = p_in1->read();

    auto cur_wavelength_id = p_in1_read.m_wavelength_id;
    // Updating the field memory
    m_memory_in1[cur_wavelength_id] = p_in1_read.m_field;

    auto s = OpticalSignal(m_memory_in1[cur_wavelength_id] +
                            m_memory_in2[cur_wavelength_id]
                            , cur_wavelength_id);

    s *= m_transmission;

    m_out_writer.delayedWrite(s,SC_ZERO_TIME);
}

void Merger::on_port_in2_changed()
{
    // Read sum of input signals
    OpticalSignal p_in2_read = p_in2->read();

    auto cur_wavelength_id = p_in2_read.m_wavelength_id;
    // Updating the field memory
    m_memory_in2[cur_wavelength_id] = p_in2_read.m_field;

    auto s = OpticalSignal(m_memory_in1[cur_wavelength_id] +
                            m_memory_in2[cur_wavelength_id]
                            , cur_wavelength_id);

    s *= m_transmission;

    m_out_writer.delayedWrite(s,SC_ZERO_TIME);
}
