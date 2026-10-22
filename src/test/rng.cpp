// spx::Rng / spx::rng_for (F-01): per-stream random numbers derived from (master seed, stream name),
// fully specified by the C++ standard (std::seed_seq + std::mt19937_64), with uniform() and normal()
// implemented in utils/rng.cpp. The known-answer values below were cross-checked against an
// independent Python implementation of [rand.util.seedseq] and [rand.eng.mers] (2026-10-21): a change
// of these values changes every noisy simulation, so it needs the same sign-off as a physics change.
#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "specs.h"
#include "utils/rng.h"

#include "test/utils.h"

namespace {
std::vector<uint64_t> raw_outputs(uint64_t master_seed, const std::string &name, size_t n = 4)
{
    spx::Rng rng = spx::rng_for(master_seed, name);
    std::vector<uint64_t> out;
    for (size_t i = 0; i < n; ++i)
        out.push_back(rng.next());
    return out;
}

std::vector<double> normals(spx::Rng rng, size_t n)
{
    std::vector<double> out;
    for (size_t i = 0; i < n; ++i)
        out.push_back(rng.normal());
    return out;
}
}

TEST(Rng, KnownAnswers)
{
    SPECS_TEST_HEADER()
    {
        // Raw engine outputs: fully specified, exact on every conforming standard library
        const std::vector<uint64_t> raw = {
            14911875552587323184ull, 1507072823243658211ull, 9491949317446420182ull, 16610011135698306531ull,
        };
        EXPECT_EQ(raw_outputs(0, "ROOT/pdet1"), raw);
        EXPECT_EQ(raw_outputs(12345, "ROOT/pdet1", 1), std::vector<uint64_t>{16168471327365203156ull});
        EXPECT_EQ(raw_outputs(0, "", 1), std::vector<uint64_t>{9826868804458059804ull});

        // uniform(): top 53 bits of one raw output, exact
        spx::Rng u = spx::rng_for(0, "ROOT/pdet1");
        for (const uint64_t r : raw)
            EXPECT_EQ(u.uniform(), static_cast<double>(r >> 11) * 0x1.0p-53);
        EXPECT_EQ(spx::rng_for(0, "ROOT/pdet1").uniform(), 0.8083743934974328);

        // normal(): polar method (std::log is libm-dependent, hence DOUBLE_EQ, 4 ulps)
        const std::vector<double> expected = {
            0.034194908378117762, 0.94045723692920813, 0.22105967521591038,
            0.40699240732588088, 1.3402206996720596, 2.7719631316303617,
        };
        const std::vector<double> got = normals(spx::rng_for(0, "ROOT/pdet1"), expected.size());
        for (size_t i = 0; i < expected.size(); ++i)
            EXPECT_DOUBLE_EQ(got[i], expected[i]) << "normal #" << i;
        EXPECT_DOUBLE_EQ(spx::rng_for(12345, "ROOT/pdet1").normal(), 1.0467244674852054);
    }
    SPECS_TEST_FOOTER();
}

TEST(Rng, SeedWordsEncoding)
{
    SPECS_TEST_HEADER()
    {
        // seed (low, high), length (low, high), bytes little-endian, last word zero-padded
        const std::vector<uint32_t> expected = {
            0x89ABCDEF, 0x01234567, 10, 0, 0x544F4F52 /* ROOT */, 0x6564702F /* /pde */, 0x00003174 /* t1 */,
        };
        EXPECT_EQ(spx::Rng::seed_words(0x0123456789ABCDEFull, "ROOT/pdet1"), expected);
        EXPECT_EQ(spx::Rng::seed_words(0, ""), (std::vector<uint32_t>{0, 0, 0, 0}));
        // bytes above 0x7F are encoded as unsigned, whether char is signed or not
        EXPECT_EQ(spx::Rng::seed_words(0, "\xE9"), (std::vector<uint32_t>{0, 0, 1, 0, 0xE9}));
    }
    SPECS_TEST_FOOTER();
}

TEST(Rng, SameSeedAndNameGiveTheSameSequence)
{
    SPECS_TEST_HEADER()
    {
        EXPECT_EQ(raw_outputs(42, "ROOT/X1/pdet1", 1000), raw_outputs(42, "ROOT/X1/pdet1", 1000));
        EXPECT_EQ(normals(spx::rng_for(42, "ROOT/X1/pdet1"), 1000),
                  normals(spx::rng_for(42, "ROOT/X1/pdet1"), 1000));

        // the default-constructed stream is rng_for(0, "")
        spx::Rng def;
        EXPECT_EQ(def.next(), raw_outputs(0, "", 1)[0]);

        // a copy continues the same sequence, cached normal value included
        spx::Rng a = spx::rng_for(7, "a");
        a.normal(); // leaves the second value of the pair cached
        spx::Rng b = a;
        EXPECT_EQ(normals(a, 11), normals(b, 11));

        // seed() restarts the stream and drops the cached normal value
        spx::Rng c = spx::rng_for(7, "a");
        const double first = c.normal();
        c.seed(7, "a");
        EXPECT_EQ(c.normal(), first);
    }
    SPECS_TEST_FOOTER();
}

TEST(Rng, DifferentSeedsOrNamesGiveDifferentSequences)
{
    SPECS_TEST_HEADER()
    {
        typedef std::pair<uint64_t, std::string> Stream;
        const std::vector<std::pair<Stream, Stream>> pairs = {
            // seeds, including the high 32-bit word
            {{0, "ROOT/pdet1"}, {1, "ROOT/pdet1"}},
            {{0, "ROOT/pdet1"}, {1ull << 32, "ROOT/pdet1"}},
            {{1, "ROOT/pdet1"}, {(1ull << 32) | 1, "ROOT/pdet1"}},
            {{0, "ROOT/pdet1"}, {UINT64_MAX, "ROOT/pdet1"}},
            // names that differ only at the end, or are a prefix of the other
            {{0, "ROOT/pdet1"}, {0, "ROOT/pdet2"}},
            {{0, "ROOT/pdet1"}, {0, "ROOT/pdet10"}},
            {{0, "ROOT/pdet"}, {0, "ROOT/pdet1"}},
            {{0, "ROOT/X1/pdet1"}, {0, "ROOT/X1/pdet1/"}},
            // zero padding of the last word is not ambiguous (the length is encoded)
            {{0, ""}, {0, std::string("\0", 1)}},
            {{0, "abc"}, {0, std::string("abc\0", 4)}},
            {{0, "abcd"}, {0, std::string("abcd\0", 5)}},
            // the seed / name boundary cannot shift ("ab" + "c" vs "a" + "bc" style)
            {{0x63, "ab"}, {0, "abc"}},
            {{0x6261, ""}, {0, "ab"}},
            {{3, ""}, {0, std::string("\3", 1)}},
        };
        for (const auto &p : pairs)
        {
            const Stream &x = p.first;
            const Stream &y = p.second;
            EXPECT_NE(spx::Rng::seed_words(x.first, x.second), spx::Rng::seed_words(y.first, y.second))
                << x.first << " '" << x.second << "' vs " << y.first << " '" << y.second << "'";
            EXPECT_NE(raw_outputs(x.first, x.second), raw_outputs(y.first, y.second))
                << x.first << " '" << x.second << "' vs " << y.first << " '" << y.second << "'";
            EXPECT_NE(normals(spx::rng_for(x.first, x.second), 4), normals(spx::rng_for(y.first, y.second), 4));
        }
    }
    SPECS_TEST_FOOTER();
}

TEST(Rng, UniformIsInUnitInterval)
{
    SPECS_TEST_HEADER()
    {
        spx::Rng rng = spx::rng_for(0, "uniform");
        const size_t n = 200000;
        double sum = 0, min = 1, max = 0;
        for (size_t i = 0; i < n; ++i)
        {
            const double x = rng.uniform();
            ASSERT_GE(x, 0.0);
            ASSERT_LT(x, 1.0);
            // a multiple of 2^-53
            ASSERT_EQ(std::ldexp(x, 53), std::trunc(std::ldexp(x, 53)));
            sum += x;
            min = std::min(min, x);
            max = std::max(max, x);
        }
        // mean 1/2, standard error 1 / sqrt(12 n) = 6.5e-4
        EXPECT_NEAR(sum / n, 0.5, 0.004);
        EXPECT_LT(min, 1e-4);
        EXPECT_GT(max, 1 - 1e-4);
    }
    SPECS_TEST_FOOTER();
}

TEST(Rng, NormalHasZeroMeanAndUnitVariance)
{
    SPECS_TEST_HEADER()
    {
        const size_t n = 200000;
        for (const char *name : {"ROOT/pdet1", "ROOT/pdet2", "normal"})
        {
            spx::Rng rng = spx::rng_for(0, name);
            double sum = 0, sum2 = 0;
            size_t n_beyond_3sd = 0;
            for (size_t i = 0; i < n; ++i)
            {
                const double x = rng.normal();
                ASSERT_TRUE(std::isfinite(x));
                sum += x;
                sum2 += x * x;
                if (std::abs(x) > 3)
                    n_beyond_3sd++;
            }
            const double mean = sum / n;
            const double var = sum2 / n - mean * mean;
            // standard errors: mean 1 / sqrt(n) = 2.2e-3, variance sqrt(2 / n) = 3.2e-3
            EXPECT_NEAR(mean, 0.0, 0.012) << name;
            EXPECT_NEAR(var, 1.0, 0.016) << name;
            // P(|x| > 3) = 0.0027: 540 expected, standard deviation 23
            EXPECT_NEAR(static_cast<double>(n_beyond_3sd), 0.0027 * n, 150) << name;
        }
    }
    SPECS_TEST_FOOTER();
}

TEST(Rng, ConfigSeedDefaultsToZero)
{
    SPECS_TEST_HEADER()
    {
        EXPECT_EQ(SPECSConfig::DEFAULT_SEED, 0u);
        EXPECT_EQ(specsGlobalConfig.seed, 0u);
        EXPECT_FALSE(specsGlobalConfig.seed_explicit);
    }
    SPECS_TEST_FOOTER();
}
