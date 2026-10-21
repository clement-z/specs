// Crossing model unit tests (B-032, B-033): passive S-matrix t = 10^(-att/20) sqrt(1 - x^2) (phase 0),
// cross j*x with x = 10^(-|xtalk|/20) (0 when xtalk is not given), and per-wavelength memories.
#include <gtest/gtest.h>
#include <systemc.h>

#include "devices/crossing.h"
#include "optical_output_port.h"
#include "specs.h"
#include "test/utils.h"

namespace {
void init_op()
{
    specsGlobalConfig.analysis_type = SPECSConfig::OP;
    specsGlobalConfig.trace_all_optical_nets = false;
    // per-process trace file in the gtest temp dir instead of a fixed /tmp path
    specsGlobalConfig.trace_filename = testing::TempDir() + "specs_test_" + std::to_string(getpid());
    specsGlobalConfig.engine_timescale = SPECSConfig::ONE_FS;
    specsGlobalConfig.applyEngineResolution();
}
void run_op()
{
    specsGlobalConfig.prepareSimulation();
    sc_start(SC_ZERO_TIME);
}
OpticalSignal cw(OpticalSignal::field_type field, double wl = 1550e-9)
{
    return OpticalSignal(field, wl);
}
// Closed-form coefficients: x = 10^(-|xtalk|/20) (0 if NaN), t = 10^(-att/20) sqrt(1 - x^2)
double expected_x(double xtalk_dB)
{
    return std::isnan(xtalk_dB) ? 0 : std::pow(10.0, -std::fabs(xtalk_dB) / 20);
}
double expected_t(double att_dB, double xtalk_dB)
{
    const double x = expected_x(xtalk_dB);
    return std::pow(10.0, -att_dB / 20) * std::sqrt(1 - x * x);
}
}

TEST(CrossingModel, Coefficients)
{
    SPECS_TEST_HEADER()
    {
        struct Case { double att_dB, xtalk_dB, t_power, x_power; };
        // power ratios computed by hand: t^2 = 10^(-att/10) (1 - x^2), x^2 = 10^(-|xtalk|/10)
        const Case cases[] = {
            {0, -20, 0.99, 0.01},
            {1, 10, 0.794328234724281 * 0.9, 0.1},
            {1, -10, 0.794328234724281 * 0.9, 0.1},
            {3, -30, 0.501187233627272 * 0.999, 0.001},
            {0, 0, 0, 1},                                           // xtalk = 0 dB: everything crosses
            {0.5, NAN, 0.891250938133746, 0},                       // no crosstalk: t = 10^(-att/20)
        };
        int i = 0;
        for (const auto &c : cases)
        {
            CrossingBase uut(("uut" + std::to_string(i++)).c_str(), c.att_dB, c.xtalk_dB);
            OpticalSignal::field_type through, cross;
            uut.s_parameters(through, cross);
            SCOPED_TRACE("att = " + std::to_string(c.att_dB) + ", xtalk = " + std::to_string(c.xtalk_dB));
            EXPECT_NEAR(std::norm(through), c.t_power, 1e-12);
            EXPECT_NEAR(std::norm(cross), c.x_power, 1e-12);
            EXPECT_NEAR(std::abs(through), expected_t(c.att_dB, c.xtalk_dB), 1e-12);
            EXPECT_NEAR(std::abs(cross), expected_x(c.xtalk_dB), 1e-12);
            if (c.t_power > 0)
            {
                EXPECT_ANGLE_EQ(through, 0.0);                      // through: phase 0
            }
            if (c.x_power > 0)
            {
                EXPECT_ANGLE_EQ(cross, pi / 2);                     // cross: j*x (quadrature)
            }
            // passive: |t|^2 + |x|^2 = 10^(-att/10) (1 - x^2) + x^2 <= 1 for att >= 0 (= 1 for att = 0)
            EXPECT_LE(std::norm(through) + std::norm(cross), 1 + 1e-15);
            if (c.att_dB == 0)
            {
                EXPECT_NEAR(std::norm(through) + std::norm(cross), 1, 1e-15);
            }
        }
    }
    SPECS_TEST_FOOTER();
}

TEST(CrossingModel, XtalkSignIgnored)
{
    SPECS_TEST_HEADER()
    {
        // a positive xtalk is read as an isolation: xtalk = 20 and xtalk = -20 both give x^2 = 1 %
        for (double xtalk_dB : {3.0, 10.0, 20.0, 40.0})
        {
            CrossingBase pos(("pos" + std::to_string(int(xtalk_dB))).c_str(), 0.7, xtalk_dB);
            CrossingBase neg(("neg" + std::to_string(int(xtalk_dB))).c_str(), 0.7, -xtalk_dB);
            OpticalSignal::field_type t_pos, x_pos, t_neg, x_neg;
            pos.s_parameters(t_pos, x_pos);
            neg.s_parameters(t_neg, x_neg);
            EXPECT_EQ(t_pos, t_neg);
            EXPECT_EQ(x_pos, x_neg);
            EXPECT_NEAR(std::norm(x_pos), std::pow(10.0, -xtalk_dB / 10), 1e-15);
        }
    }
    SPECS_TEST_FOOTER();
}

TEST(CrossingModel, DefaultNoCrosstalk)
{
    SPECS_TEST_HEADER()
    {
        // OCTANE builds Crossing(name, loss): xtalk defaults to NaN, i.e. x = 0 and t = 10^(-att/20)
        init_op();
        Crossing uut("uut", 1.5);
        spx::oa_signal_type in1("in1"), in2("in2"), out1("out1"), out2("out2");
        uut.p_in1(in1); uut.p_in2(in2); uut.p_out1(out1); uut.p_out2(out2);
        OpticalSignal::field_type through, cross;
        uut.s_parameters(through, cross);
        EXPECT_CPLX_EQ(through, OpticalSignal::field_type(std::pow(10.0, -1.5 / 20), 0));
        EXPECT_EQ(std::abs(cross), 0.0);
        run_op();
        in1.write(cw(1));
        specsGlobalConfig.runOPAnalysis();
        EXPECT_NEAR(out1.read().power(), std::pow(10.0, -0.15), 1e-12);
        EXPECT_ANGLE_EQ(out1.read(), 0.0);
        EXPECT_EQ(out2.read().power(), 0.0);                       // nothing crosses
    }
    SPECS_TEST_FOOTER();
}

TEST(CrossingModel, SingleInput)
{
    SPECS_TEST_HEADER()
    {
        // att = 1 dB, xtalk = 10 dB (isolation), 1 W into in1:
        // out1 = 10^(-0.1) (1 - 0.1) = 0.714895 W at 0, out2 = 0.1 W at pi/2
        init_op();
        Crossing uut("uut", 1, 10);
        spx::oa_signal_type in1("in1"), in2("in2"), out1("out1"), out2("out2");
        uut.p_in1(in1); uut.p_in2(in2); uut.p_out1(out1); uut.p_out2(out2);
        run_op();
        in1.write(cw(1));
        specsGlobalConfig.runOPAnalysis();
        EXPECT_NEAR(out1.read().power(), 0.794328234724281 * 0.9, 1e-12);
        EXPECT_NEAR(out2.read().power(), 0.1, 1e-12);
        EXPECT_ANGLE_EQ(out1.read(), 0.0);
        EXPECT_ANGLE_EQ(out2.read(), pi / 2);
    }
    SPECS_TEST_FOOTER();
}

TEST(CrossingModel, PassiveForAnyRelativePhase)
{
    SPECS_TEST_HEADER()
    {
        // Orthogonal columns of equal norm: P_out = (|t|^2 + |x|^2) P_in for any input phases,
        // so two coherent inputs never give more output than input power (2.42 W out of 2 W before B-032)
        init_op();
        Crossing lossless("lossless", 0, -10);
        Crossing lossy("lossy", 1, 20);
        spx::oa_signal_type a1("a1"), a2("a2"), a3("a3"), a4("a4");
        spx::oa_signal_type b1("b1"), b2("b2"), b3("b3"), b4("b4");
        lossless.p_in1(a1); lossless.p_in2(a2); lossless.p_out1(a3); lossless.p_out2(a4);
        lossy.p_in1(b1); lossy.p_in2(b2); lossy.p_out1(b3); lossy.p_out2(b4);
        run_op();
        const OpticalSignal::field_type s1(0.6, 0.2);
        for (int k = 0; k < 16; ++k)
        {
            const double phi = 2 * pi * k / 16;
            const OpticalSignal::field_type s2 = std::polar(0.8, phi);
            a1.write(cw(s1)); a2.write(cw(s2));
            b1.write(cw(s1)); b2.write(cw(s2));
            specsGlobalConfig.runOPAnalysis();
            const double p_in = std::norm(s1) + std::norm(s2);
            SCOPED_TRACE("relative phase k = " + std::to_string(k));
            EXPECT_NEAR(a3.read().power() + a4.read().power(), p_in, 1e-12);
            EXPECT_NEAR(b3.read().power() + b4.read().power(), (std::pow(10.0, -0.1) * 0.99 + 0.01) * p_in, 1e-12);
            EXPECT_LE(b3.read().power() + b4.read().power(), p_in);
        }
    }
    SPECS_TEST_FOOTER();
}

TEST(CrossingModel, TwoWavelengthsIndependent)
{
    SPECS_TEST_HEADER()
    {
        // in1 = 1 W at 1550 nm, in2 = 1 W at 1560 nm, att = 0, xtalk = -10 dB (x^2 = 0.1):
        // each wavelength propagates on its own (B-033). The output ports keep the last
        // emitted field per wavelength, which checks both wavelengths of each output.
        init_op();
        Crossing uut("uut", 0, -10);
        spx::oa_signal_type in1("in1"), in2("in2"), out1("out1"), out2("out2");
        uut.p_in1(in1); uut.p_in2(in2); uut.p_out1(out1); uut.p_out2(out2);
        run_op();
        const OpticalSignal s1 = cw(1, 1550e-9), s2 = cw(1, 1560e-9);
        const uint32_t wl1 = s1.m_wavelength_id, wl2 = s2.m_wavelength_id;
        ASSERT_NE(wl1, wl2);
        in1.write(s1);
        in2.write(s2);
        specsGlobalConfig.runOPAnalysis();
        auto &out1_fields = uut.m_out1_writer.m_emitted_fields;
        auto &out2_fields = uut.m_out2_writer.m_emitted_fields;
        EXPECT_NEAR(std::norm(out1_fields[wl1]), 0.9, 1e-12);     // in1 through, 1550 nm
        EXPECT_NEAR(std::norm(out1_fields[wl2]), 0.1, 1e-12);     // in2 cross, 1560 nm
        EXPECT_NEAR(std::norm(out2_fields[wl1]), 0.1, 1e-12);     // in1 cross, 1550 nm
        EXPECT_NEAR(std::norm(out2_fields[wl2]), 0.9, 1e-12);     // in2 through, 1560 nm
        EXPECT_ANGLE_EQ(out1_fields[wl1], 0.0);
        EXPECT_ANGLE_EQ(out1_fields[wl2], pi / 2);
        EXPECT_ANGLE_EQ(out2_fields[wl1], pi / 2);
        EXPECT_ANGLE_EQ(out2_fields[wl2], 0.0);
        // in1 changes at 1550 nm only: the 1560 nm outputs keep their values
        in1.write(cw(std::sqrt(4.0), 1550e-9));
        specsGlobalConfig.runOPAnalysis();
        EXPECT_NEAR(std::norm(out1_fields[wl1]), 3.6, 1e-12);
        EXPECT_NEAR(std::norm(out2_fields[wl1]), 0.4, 1e-12);
        EXPECT_NEAR(std::norm(out1_fields[wl2]), 0.1, 1e-12);
        EXPECT_NEAR(std::norm(out2_fields[wl2]), 0.9, 1e-12);
    }
    SPECS_TEST_FOOTER();
}

TEST(CrossingModel, BidirectionalReciprocal)
{
    SPECS_TEST_HEADER()
    {
        // CrossingBi (p0 = in1, p1 = in2, p2 = out1, p3 = out2), att = 0.5 dB, xtalk = -10 dB:
        // forward and backward light at the same time, same coefficients in both directions
        init_op();
        CrossingBi uut("uut", 0.5, -10);
        spx::oa_signal_type in0("in0"), in1("in1"), in2("in2"), in3("in3"), o0("o0"), o1("o1"), o2("o2"), o3("o3");
        uut.p0_in(in0); uut.p1_in(in1); uut.p2_in(in2); uut.p3_in(in3);
        uut.p0_out(o0); uut.p1_out(o1); uut.p2_out(o2); uut.p3_out(o3);
        run_op();
        const double t2 = std::pow(10.0, -0.05) * 0.9, x2 = 0.1;   // |t|^2, |x|^2
        in0.write(cw(1));                                           // 1 W forward into in1
        in2.write(cw(std::sqrt(4.0)));                              // 4 W backward into out1
        specsGlobalConfig.runOPAnalysis();
        EXPECT_NEAR(o2.read().power(), t2, 1e-12);                  // p0 -> p2 through
        EXPECT_NEAR(o3.read().power(), x2, 1e-12);                  // p0 -> p3 cross
        EXPECT_NEAR(o0.read().power(), 4 * t2, 1e-12);              // p2 -> p0 through
        EXPECT_NEAR(o1.read().power(), 4 * x2, 1e-12);              // p2 -> p1 cross
        EXPECT_ANGLE_EQ(o2.read(), 0.0);
        EXPECT_ANGLE_EQ(o3.read(), pi / 2);
        EXPECT_ANGLE_EQ(o0.read(), 0.0);
        EXPECT_ANGLE_EQ(o1.read(), pi / 2);
        // light into in2 and out2: p1 -> p3 through, p1 -> p2 cross, p3 -> p1 through, p3 -> p0 cross
        in0.write(cw(0));
        in2.write(cw(0));
        in1.write(cw(1));
        in3.write(cw(std::sqrt(4.0)));
        specsGlobalConfig.runOPAnalysis();
        EXPECT_NEAR(o3.read().power(), t2, 1e-12);
        EXPECT_NEAR(o2.read().power(), x2, 1e-12);
        EXPECT_NEAR(o1.read().power(), 4 * t2, 1e-12);
        EXPECT_NEAR(o0.read().power(), 4 * x2, 1e-12);
        EXPECT_ANGLE_EQ(o3.read(), 0.0);
        EXPECT_ANGLE_EQ(o2.read(), pi / 2);
        EXPECT_ANGLE_EQ(o1.read(), 0.0);
        EXPECT_ANGLE_EQ(o0.read(), pi / 2);
    }
    SPECS_TEST_FOOTER();
}
