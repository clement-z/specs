#include "optical_signal.h"
#include "specs.h"

using namespace std::complex_literals;

unsigned int OpticalSignal::nextId = 0;

OpticalSignal &OpticalSignal::operator+=(const OpticalSignal &rhs)
{
    // cout << "Summing two signals: " << endl;
    // cout << "\t" << *this << endl;
    // cout << "\t" << rhs << endl;

    auto this_wavelength = this->getWavelength();
    auto rhs_wavelength = rhs.getWavelength();

    if (this_wavelength == rhs_wavelength)
        m_field += rhs.m_field;
    else if (isnan(rhs_wavelength))
        (void)rhs; // nothing to do
    else if (isnan(this_wavelength))
        *this = rhs;
    else if (specsGlobalConfig.simulation_mode == FREQUENCY_DOMAIN)
    {
        // TODO: move that somewhere else ???
        // Replace with rhs
        if (rhs_wavelength > this_wavelength)
            *this = rhs;
    }
    else
        cerr << "Attempted to sum signals of different wavelengths, but current"
             << "simulation mode doesn't allow it (" << __FUNCTION__ << ")" << endl;
    // cout << "\t--> " << *this << endl;
    return *this;
}

OpticalSignal &OpticalSignal::operator-=(const OpticalSignal &rhs)
{
    // cout << "Substracting two signals: " << endl;
    // cout << "\t" << *this << endl;
    // cout << "\t" << rhs << endl;

    auto this_wavelength = this->getWavelength();
    auto rhs_wavelength = rhs.getWavelength();

    if (this_wavelength == rhs_wavelength)
        m_field -= rhs.m_field;
    else if (isnan(rhs_wavelength))
        (void)rhs; // nothing to do
    else if (isnan(this_wavelength))
        *this = rhs;
    else if (specsGlobalConfig.simulation_mode == FREQUENCY_DOMAIN)
    {
        // TODO: move that somewhere else ???
        // Replace with rhs
        if (rhs_wavelength > this_wavelength)
            *this = rhs;
    }
    else
        cerr << "Attempted to substract signals of different wavelengths, but current"
             << "simulation mode doesn't allow it (" << __FUNCTION__ << ")" << endl;
    // cout << "\t--> " << *this << endl;
    return *this;
}

OpticalSignal OpticalSignal::sumSignals(OpticalSignal s0, OpticalSignal s1)
{
    cerr << "Use of deprecated" << __FUNCTION__<< endl;

    // Maybe doing just the sum is faster than this
    if (s0.m_field == 0.0) {
        OpticalSignal os_sum = s1;
        os_sum.getNewId();
        return os_sum;
    }
    if (s1.m_field == (complex<double>) 0) {
        OpticalSignal os_sum = s0;
        os_sum.getNewId();
        return os_sum;
    }

    if (s0.getWavelength() != s1.getWavelength())
    {
        bool ignore_wavelength_error = false;
        if (ignore_wavelength_error)
        {
            cerr << "Signals have different wavelengths:" << std::endl;
            cerr << "\t" << s0.getWavelength() << "(" << norm(s0.m_field) << ")"
                 << "\t" << s1.getWavelength() << "(" << norm(s1.m_field) << ")"
                 << std::endl;

            sc_stop();
        } else {
            OpticalSignal os_sum = s0;
            if (norm(s1.m_field) > norm(s0.m_field))
                os_sum = s1;
            os_sum.getNewId();
            return os_sum;
        }
    }

    complex<double> Asum = s0.m_field + s1.m_field;

    OpticalSignal os_sum = s0;
    os_sum.m_field = Asum;
    os_sum.getNewId();

    return os_sum;
}

uint32_t OpticalSignal::getIDFromWavelength(const double &wavelength)
{
    auto it = find (specsGlobalConfig.wavelengths_vector.cbegin(),
                    specsGlobalConfig.wavelengths_vector.cend(),
                    wavelength);

    uint32_t index;

    if (it != specsGlobalConfig.wavelengths_vector.cend())
    { // Found
        index = it - specsGlobalConfig.wavelengths_vector.cbegin();
    }
    else
    { // Didn't find, has to add it to the vector
        specsGlobalConfig.wavelengths_vector.push_back(wavelength);
        index = specsGlobalConfig.wavelengths_vector.size() - 1;
        // -1 because of zero indexing
    }

    if (index == (uint32_t)(-1))
    {
        cerr << "Error: too many wavelengths being used ." << endl;
        exit(1);
    }

    return index;
}

double OpticalSignal::getWavelength() const
{
    if (m_wavelength_id >= specsGlobalConfig.wavelengths_vector.size())
    {
        cerr << "Wavelength not found in global vector." << endl;
        exit(1);
    }
    return specsGlobalConfig.wavelengths_vector[m_wavelength_id];
}

double OpticalSignal::getWavelength(const uint32_t &wavelength_id)
{
    if (wavelength_id >= specsGlobalConfig.wavelengths_vector.size())
    {
        cerr << "Wavelength not found in global vector." << endl;
        exit(1);
    }
    return specsGlobalConfig.wavelengths_vector[wavelength_id];
}