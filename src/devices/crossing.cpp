#include "specs.h"
#include "crossing.h"

using namespace std;

void CrossingUni::on_input_changed()
{
    // If it's NAN, it's because it was not specified and thus the linear should be zero (-inf dB)
    const double crosstalk_field_lin = (isnan(m_crosstalk_power_dB)) ? 0 : pow(10.0, m_crosstalk_power_dB / 20);
    const double transmission_field_lin = pow(10.0, -m_attenuation_power_dB / 20); // due to attenuations

    // Pre-calculate S-parameters
    OpticalSignal::field_type S13, S14, S23, S24;
    // the second part relates to power that is crossed over (not transmitted)
    S13 = polar(transmission_field_lin, 0.0); // From in1 to out1
    S24 = polar(transmission_field_lin, 0.0); // From in2 to out2
    S14 = polar(crosstalk_field_lin, 0.0);   // From in1 to out2 (crosstalk)
    S23 = polar(crosstalk_field_lin, 0.0);   // From in2 to out1 (crosstalk)

    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << ":" << endl;
        cout << "transmission_power = " << norm(S13) << " W/W" << endl;
        cout << "crosstalk_power = " << norm(S14)<< " W/W" << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p_in1.get_interface()))->name();
        cout << " ---   ---> ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p_out1.get_interface()))->name();
        cout << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p_in2.get_interface()))->name();
        cout << " ---   ---> ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p_out2.get_interface()))->name();
        cout << endl;

        cout << endl;
    }

    while (true) {
        // Wait for a new input signal
        wait();

        // Read current inputs
        auto s1 = p_in1->read();
        auto s2 = p_in2->read();

        // Apply S-parameters
        auto s3 = s1 * S13 + s2 * S23;
        auto s4 = s1 * S14 + s2 * S24;

        // Get new IDs for signal
        s3.getNewId();
        s4.getNewId();

        // Write to ouput port after delay
        m_out1_writer.delayedWrite(s3, sc_time(0, SC_NS));
        m_out2_writer.delayedWrite(s4, sc_time(0, SC_NS));
    }
}

void CrossingBi::on_input_changed()
{
    // If it's NAN, it's because it was not specified and thus the linear should be zero (-inf dB)
    const double crosstalk_field_lin = (isnan(m_crosstalk_power_dB)) ? 0 : pow(10.0, m_crosstalk_power_dB / 20);
    const double transmission_field_lin = pow(10.0, -m_attenuation_power_dB / 20); // due to attenuations

    // Pre-calculate S-parameters
    OpticalSignal::field_type through = polar(transmission_field_lin, 0.0);
    OpticalSignal::field_type cross = polar(crosstalk_field_lin, 0.0);

    OpticalSignal::field_type S02, S03, S12, S13;
    // the second part relates to power that is crossed over (not transmitted)
    S02 = through; // From in1 to out1
    S13 = through; // From in2 to out2
    S03 = cross;   // From in1 to out2 (crosstalk)
    S12 = cross;   // From in2 to out1 (crosstalk)

    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << ":" << endl;
        cout << "transmission_power = " << norm(through) << " W/W" << endl;
        cout << "crosstalk_power = " << norm(cross)<< " W/W" << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p0_in.get_interface()))->name();
        cout << " ---   ---> ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p2_out.get_interface()))->name();
        cout << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p1_in.get_interface()))->name();
        cout << " ---   ---> ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p3_out.get_interface()))->name();
        cout << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p2_in.get_interface()))->name();
        cout << " ---   ---> ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p0_out.get_interface()))->name();
        cout << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p3_in.get_interface()))->name();
        cout << " ---   ---> ";
        cout << (dynamic_cast<spx::oa_signal_type *>(p1_out.get_interface()))->name();
        cout << endl;

        cout << endl;
    }

    while (true) {
        // Wait for a new input signal
        wait();

        // Read current inputs
        auto s0_in = p0_in->read();
        auto s1_in = p1_in->read();
        auto s2_in = p2_in->read();
        auto s3_in = p3_in->read();

        // Apply S-parameters
        auto s0_out = s2_in * S02 + s3_in * S03;
        auto s1_out = s2_in * S12 + s3_in * S13;
        auto s2_out = s0_in * S02 + s1_in * S12;
        auto s3_out = s0_in * S03 + s1_in * S13;

        // Get new IDs for signal
        s0_out.getNewId();
        s1_out.getNewId();
        s2_out.getNewId();
        s3_out.getNewId();

        // Write to ouput port after delay
        m_p0_out_writer.delayedWrite(s0_out, sc_time(0, SC_NS));
        m_p1_out_writer.delayedWrite(s1_out, sc_time(0, SC_NS));
        m_p2_out_writer.delayedWrite(s2_out, sc_time(0, SC_NS));
        m_p3_out_writer.delayedWrite(s3_out, sc_time(0, SC_NS));
    }
}