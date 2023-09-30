#include <splitter.h>
#include <specs.h>

void Splitter::on_port_in_changed()
{
    const double transmission = pow(10.0, - m_attenuation_dB / 20);
    const double S12 = transmission * sqrt(m_split_ratio);
    const double S13 = transmission * sqrt(1 - m_split_ratio);

    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << ":" << endl;
        cout << "transmission = " << pow(transmission, 2) << " W/W" << endl;
        cout << "splitting ratio_power = 0.5" << " W/W" << endl;
        cout << "\t    " << (dynamic_cast<spx::oa_signal_type *>(p_out1.get_interface()))->name() << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p_in.get_interface()))->name()  << " -->" << endl;
        cout << "\t    " << (dynamic_cast<spx::oa_signal_type *>(p_out2.get_interface()))->name();
        cout << endl;
        cout << endl;
    }

    while (true) {
        // Wait for a new input signal
        wait();

        // Read input signal
        const auto &s = p_in->read();

        // Apply device's transmission
        auto s1 = S12 * s;
        auto s2 = S13 * s;

        // Get new IDs
        s1.getNewId();
        s2.getNewId();

        // Write to output ports after delay
        m_out1_writer.delayedWrite(s1, SC_ZERO_TIME);
        m_out2_writer.delayedWrite(s2, SC_ZERO_TIME);
    }
}
