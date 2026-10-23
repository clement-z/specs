#pragma once

#include <bit>
#include <complex>
#include <cstdint>
#include <vector>

// F-15: caches of device transmissions (S-parameters). Each cached value is computed by the
// same expression as before, only less often: same arithmetic, same bits.

// Values that depend only on the wavelength, computed the first time a wavelength is seen.
// Indexed by wavelength id (the index in the global wavelength vector, which only grows)
template <class T>
class WavelengthCache {
public:
    // Value for wavelength_id, computed by compute() the first time
    template <class F>
    const T &get(uint32_t wavelength_id, F &&compute)
    {
        if (wavelength_id >= m_known.size())
        {
            m_values.resize(wavelength_id + 1);
            m_known.resize(wavelength_id + 1, 0);
        }
        if (!m_known[wavelength_id])
        {
            m_values[wavelength_id] = compute();
            m_known[wavelength_id] = 1;
        }
        return m_values[wavelength_id];
    }

private:
    std::vector<T> m_values;
    std::vector<char> m_known;
};

// polar(amplitude, phase) for the last phase, recomputed only when the phase changes. Phases
// are compared bit for bit: -0 and +0 give different imaginary parts, NaN != NaN
class PolarCache {
public:
    // amplitude 0 until assigned (devices assign it before the simulation)
    PolarCache() {}
    explicit PolarCache(double amplitude)
        : m_amplitude(amplitude)
    {}

    const std::complex<double> &get(double phase)
    {
        const uint64_t bits = std::bit_cast<uint64_t>(phase);
        if (!m_valid || bits != m_phase_bits)
        {
            m_value = std::polar(m_amplitude, phase);
            m_phase_bits = bits;
            m_valid = true;
        }
        return m_value;
    }

private:
    double m_amplitude = 0;
    bool m_valid = false;
    uint64_t m_phase_bits = 0;
    std::complex<double> m_value;
};
