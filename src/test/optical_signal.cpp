#include <gtest/gtest.h>
#include <systemc.h>
#include <cmath>

#include "optical_signal.h"
#include "specs.h"

#include "test/utils.h"

// B-010: wavelengths equal within a relative WAVELENGTH_MATCH_RTOL are one wavelength
TEST(WavelengthMatching, SameWavelengthIsRelativeWithinTheTolerance)
{
    const double rtol = OpticalSignal::WAVELENGTH_MATCH_RTOL;
    EXPECT_EQ(rtol, 1e-12);
    EXPECT_TRUE(OpticalSignal::sameWavelength(1.55e-6, 1.55e-6));
    EXPECT_TRUE(OpticalSignal::sameWavelength(1.55e-6, 1550 * 1e-9));
    EXPECT_TRUE(OpticalSignal::sameWavelength(1.55e-6, 1.55e-6 * (1 + 0.9e-12)));
    EXPECT_TRUE(OpticalSignal::sameWavelength(1.55e-6 * (1 - 0.9e-12), 1.55e-6));
    EXPECT_FALSE(OpticalSignal::sameWavelength(1.55e-6, 1.55e-6 * (1 + 1.1e-12)));
    EXPECT_FALSE(OpticalSignal::sameWavelength(1.55e-6, 1.551e-6));
    // relative: also at other scales
    EXPECT_TRUE(OpticalSignal::sameWavelength(1.0, 1.0 + 0.9e-12));
    EXPECT_FALSE(OpticalSignal::sameWavelength(1.0, 1.0 + 1.1e-12));
    EXPECT_FALSE(OpticalSignal::sameWavelength(0.0, 1e-300));
    EXPECT_TRUE(OpticalSignal::sameWavelength(0.0, 0.0));
    EXPECT_FALSE(OpticalSignal::sameWavelength(NAN, NAN));
    EXPECT_FALSE(OpticalSignal::sameWavelength(1.55e-6, NAN));
}

// The registry keeps the first registered value: a value that matches it gets its id
TEST(WavelengthMatching, RegistryGivesTheIdOfTheFirstMatchingWavelength)
{
    SPECS_TEST_HEADER()
    {
        auto &registry = specsGlobalConfig.wavelengths_vector;
        registry.clear();

        const OpticalSignal a(1.0, 1.55e-6);
        const double written_differently = 1550 * 1e-9;
        ASSERT_NE(written_differently, 1.55e-6);
        const OpticalSignal b(1.0, written_differently);
        EXPECT_EQ(b.m_wavelength_id, a.m_wavelength_id);
        EXPECT_EQ(b.getWavelength(), 1.55e-6);

        // c/f: 193.414489032258 THz is 1550 nm up to the last bits
        const double from_frequency = 299792458.0 / 193.414489032258e12;
        ASSERT_NE(from_frequency, 1.55e-6);
        const OpticalSignal f(1.0, from_frequency);
        EXPECT_EQ(f.m_wavelength_id, a.m_wavelength_id);

        // beyond the tolerance: another wavelength
        const OpticalSignal d(1.0, 1.55e-6 * (1 + 2e-12));
        EXPECT_NE(d.m_wavelength_id, a.m_wavelength_id);
        EXPECT_EQ(registry.size(), 2u);

        // a value within the tolerance of the second one only gets the second id
        const OpticalSignal e(1.0, 1.55e-6 * (1 + 2.9e-12));
        EXPECT_EQ(e.m_wavelength_id, d.m_wavelength_id);
        EXPECT_EQ(registry.size(), 2u);
    }
    SPECS_TEST_FOOTER();
}
