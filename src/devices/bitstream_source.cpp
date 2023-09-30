#include <bitstream_source.h>

#include <random>
#include <cstdlib>

using std::cout;
using std::endl;

unsigned int BitstreamSource::m_bits_per_value = 8;
//double BitstreamSource::m_output_jitter = 0.0;

void BitstreamSource::runner()
{
    // Initialize random number generator
    //mt19937 gen(m_seed);
    //uniform_int_distribution<> dis(1, m_max_value); // uniform in [1, max]
    //int operation_mode = 0; // 0: modulated source , 1: independent pulses

    auto max_val = pow(2.0, m_bits_per_value) - 1;

    cout << name() << ":" << endl;
    cout << "signal on: " << m_signal_on << endl;
    cout << "signal off: " << m_signal_off << endl;
    cout << "bits: ";
    cout << endl;

    for(const auto &val : m_values) {
        auto val_saturated = val;
        if (m_current_value > max_val)
            val_saturated = max_val;

        for (unsigned int iBit = 0; iBit < m_bits_per_value; ++iBit) {
            bool bit = val_saturated & (1 << iBit);
            cout << bit;
        }
    }
    cout << " --> " << (dynamic_cast<sc_signal<OpticalSignal> *>(p_out.get_interface()))->name();
    cout << endl;
    cout << endl;

    // Loop over all values
    for (unsigned int iVal = 0; iVal < m_values.size(); ++iVal) {
        m_current_value = m_values[iVal];

        if (m_current_value > max_val)
            m_current_value = max_val;

        for (unsigned int iBit = 0; iBit < m_bits_per_value; ++iBit) {
            // Wait at least one clk cycle, and for enable to be high
            do {
                // Wait next clk edge
                wait();
                cout << "CLK tick (" << this->name() << ")" << endl;
            } while (!p_enable->read());
            
            // Deduce bit "value" from binary representation
            // iBit = 0 represents LSB
            bool bit = m_current_value & (1 << iBit);

            // Initialize corresponding signal
            auto s = (bit ? OpticalSignal(m_signal_on) :
                            OpticalSignal(m_signal_off));

            s.getNewId(); // New Id is necessary to notify signal change

            // Send signal to output with no delay
            m_out_writer.delayedWrite(s, SC_ZERO_TIME);
        }
    }

    // Wait for end of simulation
    while (true) {
        // Do nothing
        wait();
    }
}
