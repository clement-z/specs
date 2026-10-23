#include <gtest/gtest.h>

#include <bit>
#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>

#include "utils/transmission_cache.h"
#include "test/utils.h"

// F-15: device transmission caches give the bits of the expression they replace

TEST(TransmissionCache, WavelengthCacheComputesOncePerWavelength)
{
    SPECS_TEST_HEADER()
    {
        WavelengthCache<std::complex<double>> cache;
        int calls = 0;
        auto value = [&](uint32_t id) {
            return cache.get(id, [&]() {
                ++calls;
                return std::polar(0.9, 2.0 * id + 0.1);
            });
        };
        EXPECT_EQ(value(3), std::polar(0.9, 6.1));
        EXPECT_EQ(calls, 1);
        EXPECT_EQ(value(0), std::polar(0.9, 0.1));
        EXPECT_EQ(value(3), std::polar(0.9, 6.1));
        EXPECT_EQ(value(0), std::polar(0.9, 0.1));
        EXPECT_EQ(calls, 2);
        EXPECT_EQ(value(1000), std::polar(0.9, 2000.1));
        EXPECT_EQ(value(3), std::polar(0.9, 6.1)) << "kept when the cache grows";
        EXPECT_EQ(calls, 3);
    }
    SPECS_TEST_FOOTER();
}

TEST(TransmissionCache, PolarCacheMatchesPolarBitForBit)
{
    SPECS_TEST_HEADER()
    {
        auto bits = [](const std::complex<double> &z) {
            return std::make_pair(std::bit_cast<uint64_t>(z.real()), std::bit_cast<uint64_t>(z.imag()));
        };
        const double nan = std::numeric_limits<double>::quiet_NaN();
        const double inf = std::numeric_limits<double>::infinity();
        PolarCache cache(0.5);
        // repeated, alternating and signed-zero phases: -0 and +0 compare equal but give
        // imaginary parts of different signs
        for (double phase : {0.0, 0.0, -0.0, 0.0, -0.0, -0.0, 1.0, 1.0, 1e-300, -1.0, M_PI, M_PI,
                             std::nextafter(M_PI, 4.0), inf, nan, nan, 0.0})
        {
            const std::complex<double> expected = std::polar(0.5, phase);
            const std::complex<double> got = cache.get(phase);
            if (std::isnan(phase) || std::isinf(phase))
            {
                EXPECT_TRUE(std::isnan(got.real()) && std::isnan(got.imag())) << phase;
                EXPECT_TRUE(std::isnan(expected.real()) && std::isnan(expected.imag())) << phase;
            }
            else
                EXPECT_EQ(bits(got), bits(expected)) << phase;
        }
        EXPECT_TRUE(std::signbit(cache.get(-0.0).imag()));
        EXPECT_FALSE(std::signbit(cache.get(0.0).imag()));
    }
    SPECS_TEST_FOOTER();
}
