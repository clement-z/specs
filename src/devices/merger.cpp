#include "devices/merger.h"
#include "specs.h"

using namespace std;

void Merger::on_port_in1_changed()
{
    const double transmission = pow(10.0, - m_attenuation_dB / 20) / sqrt(2);
    m_memory_in1[0] = 0; // initializing for nan wavelength

    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << ":" << endl;
        cout << "transmission = " << pow(transmission, 2) << " W/W" << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p_in1.get_interface()))->name() << endl;
        cout << "\t --> \t" << (dynamic_cast<spx::oa_signal_type *>(p_out.get_interface()))->name() << endl;
        cout << (dynamic_cast<spx::oa_signal_type *>(p_in2.get_interface()))->name();
        cout << endl;
        cout << endl;
    }

    OpticalSignal p_in1_read;

    while (true) {
        // Wait for a new input signal
        wait();
        // Read sum of input signals
        p_in1_read = p_in1->read();

        auto cur_wavelength_id = p_in1_read.m_wavelength_id;
        // Updating the field memory
        m_memory_in1[cur_wavelength_id] = p_in1_read.m_field;

        auto s = OpticalSignal(m_memory_in1[cur_wavelength_id] +
                                m_memory_in2[cur_wavelength_id]
                                , cur_wavelength_id);

        s *= transmission;

        m_out_writer.delayedWrite(s,SC_ZERO_TIME);
    }
}

void Merger::on_port_in2_changed()
{
    const double transmission = pow(10.0, - m_attenuation_dB / 20) / sqrt(2);
    m_memory_in2[0] = 0; // initializing for nan wavelength

    OpticalSignal p_in2_read;

    while (true) {
        // Wait for a new input signal
        wait();
        // Read sum of input signals
        p_in2_read = p_in2->read();

        auto cur_wavelength_id = p_in2_read.m_wavelength_id;
        // Updating the field memory
        m_memory_in2[cur_wavelength_id] = p_in2_read.m_field;

        auto s = OpticalSignal(m_memory_in1[cur_wavelength_id] +
                                m_memory_in2[cur_wavelength_id]
                                , cur_wavelength_id);

        s *= transmission;

        m_out_writer.delayedWrite(s,SC_ZERO_TIME);
    }
}