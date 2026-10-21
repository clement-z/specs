// Inclusive sweep of .dc (F-12): sweep_count()/sweep_values() in utils/general_utils.h, compared
// with range(), the accumulation used before (stop excluded, or included through rounding, B-009).
#include <gtest/gtest.h>

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include "utils/general_utils.h"

namespace {
// the first n values of a and b have the same bits
void expect_same_bits(const std::vector<double> &a, const std::vector<double> &b, size_t n)
{
    ASSERT_GE(a.size(), n);
    ASSERT_GE(b.size(), n);
    for (size_t i = 0; i < n; ++i)
        EXPECT_EQ(std::bit_cast<uint64_t>(a[i]), std::bit_cast<uint64_t>(b[i])) << "point " << i;
}
}

// circuits/add_drop.cir: the old loop stopped before 1.565e-6, the sweep now ends on it
TEST(GeneralUtils, SweepAddDropIncludesStop)
{
    const double start = 1.535e-6, stop = 1.565e-6, step = 1e-9;
    const auto old_values = range(start, stop, step);
    const auto values = sweep_values(start, stop, step);
    EXPECT_EQ(old_values.size(), 30u);
    EXPECT_EQ(sweep_count(start, stop, step), 31u);
    ASSERT_EQ(values.size(), 31u);
    expect_same_bits(values, old_values, 30);
    EXPECT_NEAR(values.back(), stop, 1e-20);
}

// 1/0.1 = 10: the old loop already reached 0.9999999999999999 < 1 (11 points), unchanged
TEST(GeneralUtils, SweepIntegerRatio)
{
    const auto old_values = range(0.0, 1.0, 0.1);
    const auto values = sweep_values(0.0, 1.0, 0.1);
    EXPECT_EQ(sweep_count(0.0, 1.0, 0.1), 11u);
    ASSERT_EQ(values.size(), 11u);
    ASSERT_EQ(old_values.size(), 11u);
    expect_same_bits(values, old_values, 11);

    // ratio 9.99999999999999 (.dc in tests/golden/netlists/b165-dc-stray-args.cir)
    EXPECT_EQ(sweep_count(1.5e-6, 1.6e-6, 1e-8), 11u);
    expect_same_bits(sweep_values(1.5e-6, 1.6e-6, 1e-8), range(1.5e-6, 1.6e-6, 1e-8), 11);
}

// stop not on the grid: the points of the old loop
TEST(GeneralUtils, SweepNonIntegerRatio)
{
    const auto old_values = range(0.0, 1.0, 0.3);
    const auto values = sweep_values(0.0, 1.0, 0.3);
    ASSERT_EQ(values.size(), 4u);
    ASSERT_EQ(old_values.size(), 4u);
    expect_same_bits(values, old_values, 4);
}

// one interval: 2 points (accepted by .dc); a step larger than the span: 1 point (rejected by .dc)
TEST(GeneralUtils, SweepFewPoints)
{
    const auto values = sweep_values(0.0, 1.0, 1.0);
    ASSERT_EQ(values.size(), 2u);
    EXPECT_EQ(values[0], 0.0);
    EXPECT_EQ(values[1], 1.0);
    EXPECT_EQ(sweep_count(0.0, 1.0, 2.0), 1u);
    EXPECT_EQ(sweep_values(0.0, 1.0, 2.0), std::vector<double>{0.0});
}

TEST(GeneralUtilsDeathTest, SweepInvalidParameters)
{
    const auto failed = testing::ExitedWithCode(1);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    EXPECT_EXIT(sweep_count(1.0, 1.0, 0.1), failed, "^Invalid range parameters\n$");
    EXPECT_EXIT(sweep_count(1.0, 0.0, 0.1), failed, "^Invalid range parameters\n$");
    EXPECT_EXIT(sweep_count(0.0, 1.0, 0.0), failed, "^Invalid range parameters\n$");
    EXPECT_EXIT(sweep_count(0.0, 1.0, -0.1), failed, "^Invalid range parameters\n$");
    EXPECT_EXIT(sweep_count(0.0, nan, 0.1), failed, "^Invalid range parameters\n$");
    EXPECT_EXIT(sweep_count(0.0, inf, 0.1), failed, "^Invalid range parameters\n$");
    EXPECT_EXIT(sweep_values(0.0, 1.0, 0.0), failed, "^Invalid range parameters\n$");
}
