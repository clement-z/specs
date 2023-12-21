#include "specs.h"
#include <power_meter.h>
#include <cstdlib> // system()
#include <random>
#include <complex>

using namespace std;

void PowerMeter::trace(sc_trace_file *Tf) const
{
    sc_trace(Tf, m_cur_power, (string(name()) + ".power").c_str());
}

void PowerMeter::on_port_in_changed()
{
    // always initialize memory
    m_memory_in[0] = 0;

    while (true) {
        // Wait for a new input signal
        wait();

        const auto &p_in_read = p_in->read();

        auto cur_wavelength_id = p_in_read.m_wavelength_id;
        // Updating the field memory
        m_memory_in[cur_wavelength_id] = p_in_read.m_field;

        double total_power = 0;
        for (auto field : m_memory_in)
        {
            total_power += norm(field.second);
        }
        m_cur_power = total_power;
    }
}
