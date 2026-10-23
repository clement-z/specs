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

// F-15 (decision Q2): signals can carry a debug id, printed as "[NNNN]" by operator<<, only when
// built with -Dsignal_ids=true (SPECS_SIGNAL_IDS). Off by default: every new value and every copy
// took the next id from a global counter
class OpticalSignal {
public:
    typedef complex<double> field_type;

#ifdef SPECS_SIGNAL_IDS
private:
    // class variable for holding next id to attribute
    static unsigned int nextId;
#endif

public:
    field_type m_field; // complex field of the signal (sqrt(W))
    uint32_t m_wavelength_id; // accesses the global vector of wavelengths
#ifdef SPECS_SIGNAL_IDS
    unsigned int m_id;
#endif

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

#ifdef SPECS_SIGNAL_IDS
    // Copy constructor (a copy gets a new id)
    OpticalSignal(const OpticalSignal &s)
        : m_field(s.m_field)
        , m_wavelength_id(s.m_wavelength_id)
        , m_id(s.m_id)
    { getNewId(); }
#else
    OpticalSignal(const OpticalSignal &s) = default;
#endif

    // Gives the signal a new debug id (nothing without SPECS_SIGNAL_IDS)
    inline void getNewId()
    {
#ifdef SPECS_SIGNAL_IDS
        m_id = nextId++;
#endif
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

    // Wavelength matching (B-010, B-062): two wavelengths equal within this relative tolerance,
    // |a - b| <= WAVELENGTH_MATCH_RTOL * max(|a|, |b|), are one wavelength, so that values written
    // differently (1550*1e-9 and 1.55e-6), computed (c/f, {expression}) or accumulated by a .dc sweep
    // (B-009) match. Used by the wavelength registry (getIDFromWavelength(), the first registered value
    // is kept) and by MLPROBE's wavelength list. A compile-time constant (doc/numerics.md); it can merge
    // two wavelengths that are really different but closer than that, or miss values that drift more
    static constexpr double WAVELENGTH_MATCH_RTOL = 1e-12;
    static bool sameWavelength(double a, double b)
    {
        return a == b || std::abs(a - b) <= WAVELENGTH_MATCH_RTOL * std::max(std::abs(a), std::abs(b));
    }

    // The id of a wavelength: the first registered one that matches it (sameWavelength()), else a new
    // one. Called when sources are set up and at each .dc point, not per event
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
        // Work with field value
        return rhs.m_field == m_field
            && rhs.m_wavelength_id == m_wavelength_id;
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

    operator field_type() const {
        return m_field;
    }

#ifdef SPECS_SIGNAL_IDS
    OpticalSignal operator=(const OpticalSignal &rhs)
    {
        m_field = rhs.m_field;
        m_wavelength_id = rhs.m_wavelength_id;
        getNewId();
        return *this;
    }
#else
    OpticalSignal &operator=(const OpticalSignal &rhs) = default;
#endif

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

#ifdef SPECS_SIGNAL_IDS
    oss << "[" << setw(4) << setfill('0') << s.m_id << setfill(' ') << "] ";
    oss << " ";
#endif
    oss << "@ " << setw(4) << setprecision(8) << s.getWavelength() * 1e9 << setw(2)
        << (isnan(s.getWavelength()) ? "" : " nm");
    oss << " (" << setprecision(6) << fixed << s.modulus() << " √W";
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
#ifdef SPECS_SIGNAL_IDS
    sc_trace(tf, s.m_id, NAME + ".id");
#endif
    sc_trace(tf, s.m_wavelength_id, NAME + ".wavelengthid");
}
