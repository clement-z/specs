#pragma once

#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <cmath>
#include <complex>
#include <stdlib.h>

#include <systemc.h>

using namespace std;

class OpticalSignal {
public:
    typedef complex<double> field_type;

private:
    // class variable for holding next id to attribute
    static unsigned int nextId;

public:
    field_type m_field; // complex field of the signal (V/m)
    uint32_t m_wavelength_id; // accesses the global vector of wavelengths
    unsigned int m_id;

    // Constructor from values
    OpticalSignal(const field_type &field = 0.0,
                  const double &wavelength = numeric_limits<double>::quiet_NaN())
        : m_field(field)
    {
        if (!isnan(wavelength))
            m_wavelength_id = getIDFromWavelength(wavelength);
        else
            m_wavelength_id = 0;
        getNewId();
    }

    // Constructor sending an id
    OpticalSignal(const field_type &field,
                  const uint32_t &wavelength_id)
        : m_field(field)
        , m_wavelength_id(wavelength_id)
    {
        getNewId();
    }

    // Copy constructor
    OpticalSignal(const OpticalSignal &s)
        : m_field(s.m_field)
        , m_wavelength_id(s.m_wavelength_id)
        , m_id(s.m_id)
    { getNewId(); }

    virtual ~OpticalSignal() {}

    inline void getNewId()
    {
        m_id = nextId++;
    }

    void setWavelength(const double &wavelength)
    {
        if (!isnan(wavelength))
            m_wavelength_id = getIDFromWavelength(wavelength);
        else
            m_wavelength_id = 0;
    }

    double getWavelength() const;

    static double getWavelength(const uint32_t &wavelength_id);

    uint32_t getIDFromWavelength(const double &wavelength);

    inline static complex<double> amplitudePhaseToField(double amplitude, double phase)
    {
        return polar(amplitude, phase);
    }

    inline double modulus() const {
        return abs(m_field);
    }

    inline double power() const {
        return norm(m_field);
    }

    inline double phase() const {
        return arg(m_field);
    }

    // Equality is used to determine if a signal changes
    // which is why true `==` is used here in field
    // TODO: check if we need to use is_close() instead
    // For wavelength, there should be no need not to use `==`
    // for now, as no calculation is done on it. But in the future,
    // we might need to review this as well
    bool operator==(const OpticalSignal &rhs) const
    {
#if 0
        // Work with ID
        return rhs.m_id == m_id
            || (isnan(rhs.m_wavelength) && isnan(m_wavelength));
#else
        // Work with field value
        return rhs.m_field == m_field
            && rhs.m_wavelength_id == m_wavelength_id;
#endif
    }

    bool operator!=(const OpticalSignal &rhs) const
    {
        return !(*this == rhs);
    }

    bool operator<(const OpticalSignal &rhs) const
    {
        return rhs.modulus() < modulus();
    }

    inline void advancePhaseByPhase(const double &phase_rad)
    {
        m_field *= polar(1.0, phase_rad);
    }

    inline void advancePhaseByTime(const double &time_s)
    {
        // phase in rad = w * t = (2pi * (c/lambda)) * t
        constexpr const auto c_2pi = 2 * M_PI * 299792458.0;
        const auto phase_rad = (c_2pi / this->getWavelength()) * time_s;

        advancePhaseByPhase(phase_rad);
    }

    static OpticalSignal sumSignals(
        OpticalSignal s0,
        OpticalSignal s1);

    operator field_type() const {
        return m_field;
    }

    OpticalSignal operator=(const OpticalSignal &rhs)
    {
        m_field = rhs.m_field;
        m_wavelength_id = rhs.m_wavelength_id;
        getNewId();
        return *this;
    }

    OpticalSignal &operator+=(const OpticalSignal &rhs);
    OpticalSignal &operator-=(const OpticalSignal &rhs);

    OpticalSignal &operator*=(const field_type &rhs)
    {
        m_field *= rhs;
        return *this;
    }

    OpticalSignal &operator*=(const double &rhs)
    {
        m_field *= rhs;
        return *this;
    }

    OpticalSignal &operator/=(const double &rhs)
    {
        (*this) *= (1 / rhs);
        return *this;
    }

    inline friend OpticalSignal operator+(OpticalSignal lhs, const OpticalSignal &rhs);
    inline friend OpticalSignal operator-(OpticalSignal lhs, const OpticalSignal &rhs);

    template <typename T>
    inline friend OpticalSignal operator*(OpticalSignal lhs, const T &rhs);

    template <typename T>
    inline friend OpticalSignal operator*(const T &lhs, OpticalSignal rhs);

    template <typename T>
    inline friend OpticalSignal operator/(OpticalSignal lhs, const T &rhs);

    inline friend std::ostream &operator<<(std::ostream &os, const OpticalSignal &s);
    inline friend void
    sc_trace(sc_trace_file *tf, const OpticalSignal &s, const std::string &NAME);
};

OpticalSignal operator+(OpticalSignal lhs, const OpticalSignal &rhs)
{
    return lhs += rhs;
}

OpticalSignal operator-(OpticalSignal lhs, const OpticalSignal &rhs)
{
    return lhs -= rhs;
}

template <typename T>
OpticalSignal operator*(OpticalSignal lhs, const T &rhs)
{
    return lhs *= rhs;
}

template <typename T>
OpticalSignal operator*(const T &lhs, OpticalSignal rhs)
{
    return rhs *= lhs;
}

template <typename T>
inline OpticalSignal operator/(OpticalSignal lhs, const T &rhs)
{
    return lhs /= rhs;
}

inline std::ostream &operator<<(std::ostream &os, const OpticalSignal &s)
{
    using std::defaultfloat;
    using std::fixed;
    using std::setfill;
    using std::setprecision;
    using std::setw;

    std::ostringstream oss;

    oss << "[" << setw(4) << setfill('0') << s.m_id << setfill(' ') << "] ";
    oss << " @ " << setw(4) << setprecision(8) << s.getWavelength() * 1e9 << setw(2)
        << (isnan(s.getWavelength()) ? "" : " nm");
    oss << " (" << setprecision(6) << fixed << s.modulus() << " V.m⁻¹";
    oss << ", " << setprecision(6) << fixed << s.power() << " W";
    oss << ", " << setprecision(4) << fixed << s.phase() << " rad";
    oss << ")";

    os << oss.str();
    return os;
}

inline void sc_trace(sc_trace_file *tf, const OpticalSignal &s, const std::string &NAME)
{
    cerr << "Use of deprecated" << __FUNCTION__<< endl;

    // Note: C++ complex doesn't provide access to real and imaginary part
    // Therefore we cannot have references or pointers to it...
    sc_trace(tf, s.m_id, NAME + ".id");
    sc_trace(tf, s.m_wavelength_id, NAME + ".wavelengthid");
}
