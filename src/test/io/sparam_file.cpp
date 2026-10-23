// F-03: S-parameter file loader (src/io/sparam_file.{h,cpp}, doc/sparam.md). Pure parsing and
// interpolation: no SystemC elaboration, so no fork per test.
#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <sstream>
#include <string>

#include "io/sparam_file.h"

namespace {

typedef std::complex<double> cplx;

std::shared_ptr<SParamData> parse(const std::string &text, const std::string &name,
        SParamData::Convention convention = SParamData::CONVENTION_AUTO,
        SParamData::Interp interp = SParamData::INTERP_LINEAR)
{
    std::istringstream in(text);
    return SParamData::parse(in, name, SParamData::FORMAT_AUTO, convention, interp);
}

void expect_near(cplx a, cplx b, double tol = 1e-12)
{
    EXPECT_NEAR(a.real(), b.real(), tol) << a << " vs " << b;
    EXPECT_NEAR(a.imag(), b.imag(), tol) << a << " vs " << b;
}

const double c = SParamData::c;

} // namespace

TEST(SParamFile, FormatFromExtension)
{
    EXPECT_EQ(SParamData::formatFromExtension("a/b.sparam"), SParamData::FORMAT_SPARAM);
    EXPECT_EQ(SParamData::formatFromExtension("b.DAT"), SParamData::FORMAT_SPARAM);
    EXPECT_EQ(SParamData::formatFromExtension("b.s2p"), SParamData::FORMAT_TOUCHSTONE);
    EXPECT_EQ(SParamData::formatFromExtension("b.S12P"), SParamData::FORMAT_TOUCHSTONE);
    EXPECT_EQ(SParamData::formatFromExtension("b.ts"), SParamData::FORMAT_TOUCHSTONE);
    EXPECT_EQ(SParamData::formatFromExtension("b.csv"), SParamData::FORMAT_CSV);
    EXPECT_EQ(SParamData::formatFromExtension("b.taylor"), SParamData::FORMAT_TAYLOR);
    EXPECT_EQ(SParamData::formatFromExtension("b.txt"), SParamData::FORMAT_AUTO);
}

TEST(SParamFile, UnwrapAsNumpy)
{
    std::vector<double> p = {3.0, -3.0, -0.5, 2.9, -3.1};
    SParamData::unwrap(p);
    // numpy.unwrap([3, -3, -0.5, 2.9, -3.1])
    EXPECT_DOUBLE_EQ(p[0], 3.0);
    EXPECT_DOUBLE_EQ(p[1], -3.0 + 2 * M_PI);
    EXPECT_DOUBLE_EQ(p[2], -0.5 + 2 * M_PI);
    EXPECT_DOUBLE_EQ(p[3], 2.9);
    EXPECT_DOUBLE_EQ(p[4], -3.1 + 2 * M_PI);
}

TEST(SParamFile, LumericalBlocksPortsAndPhases)
{
    // header: output port first; ports in order of first appearance (out, in); wrapped phases
    const std::string text =
        "('port 2','TE',1,'port 1',1,'transmission')\n"
        "(3,3)\n"
        "1.9e14 0.5 3.0\n"
        "2.0e14 0.6 -3.0\n"
        "2.1e14 0.7 -0.5\n"
        "(\"port 1\",\"TE\",1,\"port 2\",1,\"transmission\")\n"
        "(3,3)\n"
        "2.1e14 0.1 0.3\n"   // rows in any order
        "1.9e14 0.1 0.1\n"
        "2.0e14 0.1 0.2\n";
    auto d = parse(text, "x.sparam");
    ASSERT_EQ(d->ports(), 2u);
    EXPECT_EQ(d->portNames()[0], "port 2");
    EXPECT_EQ(d->portNames()[1], "port 1");
    EXPECT_FALSE(d->conjugated());
    EXPECT_TRUE(d->active(0, 1));  // port 2 <- port 1
    EXPECT_TRUE(d->active(1, 0));
    EXPECT_FALSE(d->active(0, 0));
    EXPECT_EQ(d->portIndex("port 1"), 1);
    EXPECT_EQ(d->portIndex("1"), 0);
    EXPECT_EQ(d->portIndex("3"), -1);
    // values at the grid points: as in the file (physics convention, not conjugated)
    expect_near(d->S(0, 1, c / 2.0e14), std::polar(0.6, -3.0));
    expect_near(d->S(1, 0, c / 2.1e14), std::polar(0.1, 0.3));
    EXPECT_EQ(d->S(0, 0, c / 2.0e14), cplx(0));
    // between grid points: magnitude and unwrapped phase interpolated linearly in frequency
    const double f = 1.95e14;
    const cplx s = d->S(0, 1, c / f);
    EXPECT_NEAR(std::abs(s), 0.55, 1e-12);
    EXPECT_NEAR(std::arg(s * std::polar(1.0, -(3.0 + (-3.0 + 2 * M_PI)) / 2)), 0, 1e-9);
    // range: the overlap of the grids, as wavelengths
    EXPECT_DOUBLE_EQ(d->minWavelength(), c / 2.1e14);
    EXPECT_DOUBLE_EQ(d->maxWavelength(), c / 1.9e14);
    EXPECT_TRUE(d->inRange(c / 2.1e14));
    EXPECT_TRUE(d->inRange(c / 2.1e14 * (1 - 1e-13)));
    EXPECT_FALSE(d->inRange(c / 2.11e14));
}

TEST(SParamFile, LumericalPortDeclarationsGiveTheOrder)
{
    const std::string text =
        "[\"a\",\"LEFT\"]\n"
        "[\"b\",\"RIGHT\"]\n"
        "('b','TE',1,'a',1,'transmission')\n"
        "(2,3)\n"
        "1e14 1 0\n"
        "2e14 1 0\n";
    auto d = parse(text, "x.dat");
    ASSERT_EQ(d->ports(), 2u);
    EXPECT_EQ(d->portNames()[0], "a");
    EXPECT_EQ(d->portNames()[1], "b");
    EXPECT_TRUE(d->active(1, 0));
}

TEST(SParamFile, GroupDelayOfALinearPhase)
{
    // phase = +omega * tau (SPECS convention): tau at every point, also between them
    const double tau = 1.4e-12;
    std::ostringstream text;
    text.precision(17);
    text << "('2','TE',1,'1',1,'transmission')\n(5,3)\n";
    const double f0 = 1.9e14, df = 5e10;
    for (int k = 0; k < 5; ++k)
    {
        const double f = f0 + k * df * (1 + 0.3 * k); // non-uniform grid
        text << f << " 1 " << std::remainder(2 * M_PI * f * tau, 2 * M_PI) << "\n";
    }
    auto d = parse(text.str(), "x.sparam");
    for (double f : {f0, f0 + 0.5 * df, f0 + 3.4 * df})
        EXPECT_NEAR(d->groupDelay(d->portIndex("2"), d->portIndex("1"), c / f), tau, 1e-22) << f;
}

TEST(SParamFile, TouchstoneV1TwoPortOrderUnitsAndConjugation)
{
    // 2-port order S11 S21 S12 S22; GHz; DB with angles in degrees; engineering convention: conjugated
    const std::string text =
        "! Port[1] = {\"in\",\"TE\",1,\"LEFT\"}\n"
        "! Port[2] = out\n"
        "# GHz S DB R 50\n"
        "190000 -20 10   -1 -30   -2 40   -40 0\n"
        "200000 -20 10   -1 -30   -2 40   -40 0\n"
        "! noise data: a lower frequency follows\n"
        "1 2 3 4 5\n";
    auto d = parse(text, "x.s2p");
    ASSERT_EQ(d->ports(), 2u);
    EXPECT_TRUE(d->conjugated());
    EXPECT_EQ(d->portNames()[0], "in");
    EXPECT_EQ(d->portNames()[1], "out");
    const double wl = c / 1.95e14;
    const double deg = M_PI / 180;
    expect_near(d->S(0, 0, wl), std::polar(0.1, -10 * deg));
    expect_near(d->S(1, 0, wl), std::polar(std::pow(10.0, -1 / 20.0), 30 * deg)); // S21
    expect_near(d->S(0, 1, wl), std::polar(std::pow(10.0, -2 / 20.0), -40 * deg)); // S12
    expect_near(d->S(1, 1, wl), std::polar(0.01, 0.0));
    EXPECT_DOUBLE_EQ(d->maxWavelength(), c / 1.9e14);

    // convention=physics: as written
    auto p = parse(text, "x.s2p", SParamData::CONVENTION_PHYSICS);
    EXPECT_FALSE(p->conjugated());
    expect_near(p->S(1, 0, wl), std::polar(std::pow(10.0, -1 / 20.0), -30 * deg));
}

TEST(SParamFile, TouchstoneV2TwoPortDataOrder)
{
    const std::string head =
        "[Version] 2.0\n"
        "# Hz S RI R 50\n"
        "[Number of Ports] 2\n";
    const std::string data =
        "[Number of Frequencies] 2\n"
        "[Network Data]\n"
        "1e14 0.1 0  0.2 0  0.3 0  0.4 0\n"
        "2e14 0.1 0  0.2 0  0.3 0  0.4 0\n"
        "[End]\n";
    auto d12 = parse(head + "[Two-Port Data Order] 12_21\n" + data, "x.ts");
    auto d21 = parse(head + "[Two-Port Data Order] 21_12\n" + data, "x.ts");
    const double wl = c / 1.5e14;
    expect_near(d12->S(0, 1, wl), 0.2); // S12 second
    expect_near(d12->S(1, 0, wl), 0.3);
    expect_near(d21->S(1, 0, wl), 0.2); // S21 second
    expect_near(d21->S(0, 1, wl), 0.3);
    EXPECT_EQ(d12->portNames()[1], "2");
}

TEST(SParamFile, TouchstoneThreePortsRowMajorAndLowerMatrix)
{
    // N >= 3: row by row, S11 S12 S13 S21 ...; values may wrap over lines
    const std::string full =
        "# Hz S MA\n"
        "1e14 0.11 0 0.12 0 0.13 0\n"
        "     0.21 0 0.22 0 0.23 0\n"
        "     0.31 0 0.32 0 0.33 0\n"
        "2e14 0.11 0 0.12 0 0.13 0 0.21 0 0.22 0 0.23 0 0.31 0 0.32 0 0.33 0\n";
    auto d = parse(full, "x.s3p");
    const double wl = c / 1.5e14;
    for (size_t o = 0; o < 3; ++o)
        for (size_t i = 0; i < 3; ++i)
            expect_near(d->S(o, i, wl), 0.1 * (o + 1) + 0.01 * (i + 1));

    const std::string lower =
        "[Version] 2.0\n# Hz S RI\n[Number of Ports] 3\n[Matrix Format] Lower\n[Network Data]\n"
        "1e14 0.11 0  0.21 0 0.22 0  0.31 0 0.32 0 0.33 0\n"
        "2e14 0.11 0  0.21 0 0.22 0  0.31 0 0.32 0 0.33 0\n[End]\n";
    auto l = parse(lower, "x.ts");
    expect_near(l->S(2, 0, wl), 0.31);
    expect_near(l->S(0, 2, wl), 0.31); // symmetric fill
    expect_near(l->S(1, 2, wl), 0.32);
}

TEST(SParamFile, CsvColumnsFromInputToOutput)
{
    // SAX/gplugins key: "<in>@0,<out>@0"; physics convention
    const std::string text =
        "wavelength_m,\"o1@0,o2@0_re\",\"o1@0,o2@0_im\",\"o2@0,o1@0_re\",\"o2@0,o1@0_im\"\n"
        "1.6e-6,0.5,0.1,0.3,0\n"
        "1.5e-6,0.5,0.1,0.3,0\n";
    auto d = parse(text, "x.csv");
    ASSERT_EQ(d->ports(), 2u);
    EXPECT_EQ(d->portNames()[0], "o1");
    EXPECT_FALSE(d->conjugated());
    expect_near(d->S(1, 0, 1.55e-6), cplx(0.5, 0.1)); // o1 -> o2
    expect_near(d->S(0, 1, 1.55e-6), cplx(0.3, 0));
    EXPECT_FALSE(d->active(0, 0));
    EXPECT_DOUBLE_EQ(d->minWavelength(), c / (c / 1.5e-6));
}

TEST(SParamFile, TaylorPolynomialsAndDelay)
{
    const std::string text =
        "# comment\n"
        "specs-taylor 1\n"
        "ports in out\n"
        "center 1.55e-6\n"
        "range 1.5e-6 1.6e-6\n"
        "S out in amp 0.9 1e5      # |S| = 0.9 + 1e5 d\n"
        "S out in phase 1.0 2e6 3e12\n"
        "S 1 2 amp 0.5\n"
        "S 1 2 delay 2e-12 1e-6\n";
    auto d = parse(text, "x.taylor");
    ASSERT_EQ(d->ports(), 2u);
    const double wl = 1.56e-6, dl = wl - 1.55e-6;
    expect_near(d->S(1, 0, wl), std::polar(0.9 + 1e5 * dl, 1.0 + 2e6 * dl + 3e12 * dl * dl));
    // derived delay: -(lambda^2 / (2 pi c)) dphi/dlambda
    EXPECT_NEAR(d->groupDelay(1, 0, wl), -(wl * wl / (2 * M_PI * c)) * (2e6 + 2 * 3e12 * dl), 1e-25);
    // explicit delay; phase defaults to 0
    EXPECT_DOUBLE_EQ(d->groupDelay(0, 1, wl), 2e-12 + 1e-6 * dl);
    expect_near(d->S(0, 1, wl), 0.5);
    EXPECT_FALSE(d->active(0, 0));
    EXPECT_DOUBLE_EQ(d->minWavelength(), 1.5e-6);
    EXPECT_DOUBLE_EQ(d->maxWavelength(), 1.6e-6);
    EXPECT_FALSE(d->inRange(1.61e-6));
}

TEST(SParamFile, PchipIsExactAtNodesAndMonotone)
{
    const std::string text =
        "('2','TE',1,'1',1,'transmission')\n(4,3)\n"
        "1e14 0.1 0\n2e14 0.2 0\n3e14 0.9 0\n4e14 1.0 0\n";
    auto d = parse(text, "x.sparam", SParamData::CONVENTION_AUTO, SParamData::INTERP_PCHIP);
    // ports in order of first appearance, output first: '2' is 0, '1' is 1
    EXPECT_DOUBLE_EQ(std::abs(d->S(0, 1, c / 3e14)), 0.9);
    double last = 0;
    for (int k = 0; k <= 300; ++k)
    {
        const double m = std::abs(d->S(0, 1, c / (1e14 + k * 1e12)));
        EXPECT_GE(m, last - 1e-15);
        EXPECT_LE(m, 1.0 + 1e-15);
        last = m;
    }
}

TEST(SParamFile, PassivityReportsTheWorstInput)
{
    const std::string text =
        "('2','TE',1,'1',1,'transmission')\n(2,3)\n1e14 1.05 0\n2e14 0.9 0\n"
        "('1','TE',1,'2',1,'transmission')\n(2,3)\n1e14 0.5 0\n2e14 0.5 0\n";
    auto d = parse(text, "x.sparam");
    const auto r = d->passivity();
    EXPECT_NEAR(r.max_power_sum, 1.05 * 1.05, 1e-12);
    EXPECT_EQ(d->portNames()[r.input], "1");
    EXPECT_DOUBLE_EQ(r.wavelength_m, c / 1e14);
    EXPECT_NEAR(d->maxMagnitude(d->portIndex("1"), d->portIndex("2")), 0.5, 1e-15);
}

TEST(SParamFileDeathTest, Errors)
{
    const auto failed = testing::ExitedWithCode(1);
    EXPECT_EXIT(parse("('2','TE',1,'1',1,'transmission',1e-12)\n(1,3)\n1e14 1 0\n", "x.sparam"), failed,
                "group delay");
    EXPECT_EXIT(parse("('2','TE',1,'1',1,'transmission')\n(1,3)\n1e14 1 0\n"
                      "('1','TM',1,'2',1,'transmission')\n(1,3)\n1e14 1 0\n", "x.sparam"), failed, "two modes");
    EXPECT_EXIT(parse("('2','TE',1,'1',1,'transmission')\n(1,4)\n1e14 1 0 0\n", "x.sparam"), failed, "columns");
    EXPECT_EXIT(parse("('2','TE',1,'1',1,'transmission')\n(2,3)\n1e14 1 0\n", "x.sparam"), failed, "rows");
    EXPECT_EXIT(parse("[Version] 2.0\n# Hz S RI\n[Number of Ports] 2\n[Network Data]\n1e14 0 0 0 0 0 0 0 0\n[End]\n", "x.ts"),
                failed, "Two-Port Data Order");
    EXPECT_EXIT(parse("# Hz Y RI\n1e14 0 0 0 0 0 0 0 0\n", "x.s2p"), failed, "only S-parameters");
    EXPECT_EXIT(parse("# Hz S RI\n1e14 0 0 0 0 0 0 0\n", "x.s2p"), failed, "incomplete");
    EXPECT_EXIT(parse("# Hz S RI\n1e14 0 0\n", "x.txt"), failed, "unknown S-parameter file extension");
    EXPECT_EXIT(parse("wavelength_m,\"o1@1,o2@0_re\",\"o1@1,o2@0_im\"\n1e-6,0,0\n", "x.csv"), failed, "mode 0");
    EXPECT_EXIT(parse("wavelength_m,\"o1@0,o2@0_re\"\n1e-6,0\n", "x.csv"), failed, "_im");
    EXPECT_EXIT(parse("specs-taylor 1\nports 2\ncenter 1.55e-6\nS 2 1 amp 1\n", "x.taylor"), failed, "range");
    EXPECT_EXIT(parse("specs-taylor 1\nports 2\ncenter 1.55e-6\nrange 1e-6 2e-6\nS 3 1 amp 1\n", "x.taylor"), failed,
                "unknown port");
    EXPECT_EXIT(parse("specs-taylor 2\n", "x.taylor"), failed, "specs-taylor 1");
}
