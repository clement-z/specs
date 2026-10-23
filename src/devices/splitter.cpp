#include "devices/splitter.h"
#include "specs.h"

void Splitter::start_of_simulation()
{
    m_transmission = pow(10.0, - m_attenuation_dB / 20);
    m_S12 = m_transmission * sqrt(m_split_ratio);
    m_S13 = m_transmission * sqrt(1 - m_split_ratio);
}

void Splitter::print_initialization()
{
    cout << name() << ":" << endl;
    cout << "transmission = " << pow(m_transmission, 2) << " W/W" << endl;
    cout << "splitting ratio_power = " << m_split_ratio << " W/W" << endl;
    cout << "\t    " << port_channel_name(p_out1) << endl;
    cout << port_channel_name(p_in)  << " -->" << endl;
    cout << "\t    " << port_channel_name(p_out2);
    cout << endl;
    cout << endl;
}

void Splitter::on_port_in_changed()
{
    // Read input signal
    const auto &s = p_in->read();

    // Apply device's transmission
    auto s1 = m_S12 * s;
    auto s2 = m_S13 * s;

    // Get new IDs
    s1.getNewId();
    s2.getNewId();

    // Write to output ports after delay
    m_out1_writer.delayedWrite(s1, SC_ZERO_TIME);
    m_out2_writer.delayedWrite(s2, SC_ZERO_TIME);
}
