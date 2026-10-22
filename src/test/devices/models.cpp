// Device S-parameter unit tests (B-136): expected values come from the closed-form model, cross-checked against
// the self-checks of the legacy testbenches src/tb/{wg,dc,splitter,merger}_tb.cpp (all of which pass 3/3, 3/3, 3/3, 4/4).
// They pin the current behaviour; the sign of the coupler cross phase (a convention) is deliberately not pinned.
#include <gtest/gtest.h>
#include <systemc.h>

#include "devices/waveguide.h"
#include "devices/directional_coupler.h"
#include "devices/splitter.h"
#include "devices/merger.h"
#include "optical_output_port.h"
#include "specs.h"
#include "test/utils.h"

namespace {
void init_op(bool trace = false)
{
    specsGlobalConfig.analysis_type = SPECSConfig::OP;
    specsGlobalConfig.trace_all_optical_nets = trace;
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
OpticalSignal cw(double field_re, double field_im, double wl = 1550e-9)
{
    return OpticalSignal(OpticalSignal::field_type{field_re, field_im}, wl);
}
}

TEST(WaveguideModel, LossAndPhaseZeroSlope)
{
    SPECS_TEST_HEADER()
    {
        init_op();
        WaveguideBi uut("uut");
        uut.m_attenuation_dB_cm = 3.0103; uut.m_neff = 1; uut.m_ng = 1; uut.m_length_cm = 1;
        spx::oa_signal_type a_in("a_in"), a_out("a_out"), b_in("b_in"), b_out("b_out");
        uut.p0_in(a_in); uut.p0_out(a_out); uut.p1_in(b_in); uut.p1_out(b_out);
        run_op();
        a_in.write(cw(1, 0));
        specsGlobalConfig.runOPAnalysis();
        EXPECT_NEAR(b_out.read().power(), 0.5, 1e-4);                     // 3.0103 dB/cm * 1 cm = half power
        EXPECT_ANGLE_EQ(b_out.read(), 2 * pi * 1e-2 / 1550e-9);           // 2 pi neff L / lambda
        EXPECT_NEAR(a_out.read().power(), 0.0, 1e-12);                    // nothing reflected
    }
    SPECS_TEST_FOOTER();
}

TEST(WaveguideModel, GroupDelay)
{
    SPECS_TEST_HEADER()
    {
        init_op();
        specsGlobalConfig.simulation_mode = OpticalOutputPortMode::EVENT_DRIVEN;
        WaveguideBi uut("uut");
        uut.m_attenuation_dB_cm = 0; uut.m_neff = 2.0; uut.m_ng = 4.3; uut.m_length_cm = 1;   // L*ng/c = 143.43 ps
        spx::oa_signal_type a_in("a_in"), a_out("a_out"), b_in("b_in"), b_out("b_out");
        uut.p0_in(a_in); uut.p0_out(a_out); uut.p1_in(b_in); uut.p1_out(b_out);
        specsGlobalConfig.prepareSimulation();
        sc_start(SC_ZERO_TIME);
        a_in.write(cw(1, 0));
        sc_start(143, SC_PS);
        EXPECT_NEAR(b_out.read().power(), 0.0, 1e-12) << "output must not change before L*ng/c = 143.43 ps";
        sc_start(1, SC_PS);
        EXPECT_NEAR(b_out.read().power(), 1.0, 1e-9) << "output must have arrived by 144 ps";
    }
    SPECS_TEST_FOOTER();
}

TEST(DirectionalCouplerModel, Smatrix)
{
    SPECS_TEST_HEADER()
    {
        init_op();
        DirectionalCouplerBi uut("uut", 0.5, 0);
        uut.m_dc_through_coupling_power = 0.1;   // 10 % through, 90 % cross, 3.0103 dB excess loss
        uut.m_dc_loss = 3.0103;
        spx::oa_signal_type in0("in0"), in1("in1"), in2("in2"), in3("in3"), o0("o0"), o1("o1"), o2("o2"), o3("o3");
        uut.p0_in(in0); uut.p1_in(in1); uut.p2_in(in2); uut.p3_in(in3);
        uut.p0_out(o0); uut.p1_out(o1); uut.p2_out(o2); uut.p3_out(o3);
        run_op();
        in0.write(cw(1, 0));
        specsGlobalConfig.runOPAnalysis();
        EXPECT_NEAR(o2.read().power(), 0.05, 1e-4);      // 0 -> 2 through
        EXPECT_NEAR(o3.read().power(), 0.45, 1e-4);      // 0 -> 3 cross
        EXPECT_ANGLE_EQ(o2.read(), 0.0);
        // cross and through outputs in quadrature, either sign (the +-pi/2 convention needs a human decision)
        EXPECT_NEAR(std::abs(std::remainder(o3.read().phase() - o2.read().phase(), 2 * pi)), pi / 2, 1e-9);
        EXPECT_NEAR(o0.read().power() + o1.read().power(), 0.0, 1e-12);   // no back-reflection / port 0 -> 1 leakage
    }
    SPECS_TEST_FOOTER();
}

TEST(DirectionalCouplerModel, LosslessUnitarity)
{
    SPECS_TEST_HEADER()
    {
        init_op();
        DirectionalCouplerBi uut("uut", 0.5, 0);
        uut.m_dc_through_coupling_power = 0.3;
        uut.m_dc_loss = 0;
        spx::oa_signal_type in0("in0"), in1("in1"), in2("in2"), in3("in3"), o0("o0"), o1("o1"), o2("o2"), o3("o3");
        uut.p0_in(in0); uut.p1_in(in1); uut.p2_in(in2); uut.p3_in(in3);
        uut.p0_out(o0); uut.p1_out(o1); uut.p2_out(o2); uut.p3_out(o3);
        run_op();
        const OpticalSignal s0 = cw(0.6, 0.2), s1 = cw(-0.1, 0.5);
        in0.write(s0); in1.write(s1);
        specsGlobalConfig.runOPAnalysis();
        EXPECT_NEAR(o2.read().power() + o3.read().power(), s0.power() + s1.power(), 1e-12);
    }
    SPECS_TEST_FOOTER();
}

TEST(SplitterModel, EqualSplitWithLoss)
{
    SPECS_TEST_HEADER()
    {
        init_op();
        Splitter uut("uut");
        uut.m_attenuation_dB = 3.0103;
        spx::oa_signal_type in("in"), o1("o1"), o2("o2");
        uut.p_in(in); uut.p_out1(o1); uut.p_out2(o2);
        run_op();
        in.write(cw(std::sqrt(3.0), 0));                 // 3 W
        specsGlobalConfig.runOPAnalysis();
        EXPECT_NEAR(o1.read().power(), 0.75, 1e-4);      // 3 W * 10^(-0.30103) / 2
        EXPECT_NEAR(o2.read().power(), 0.75, 1e-4);
        EXPECT_ANGLE_EQ(o1.read(), 0.0);
        EXPECT_ANGLE_EQ(o2.read(), 0.0);
    }
    SPECS_TEST_FOOTER();
}

TEST(MergerModel, CoherentSum)
{
    SPECS_TEST_HEADER()
    {
        init_op();
        Merger uut("uut", 0);
        uut.m_attenuation_dB = 0;
        spx::oa_signal_type in1("in1"), in2("in2"), out("out");
        uut.p_in1(in1); uut.p_in2(in2); uut.p_out(out);
        run_op();
        in1.write(cw(1, 0));
        specsGlobalConfig.runOPAnalysis();
        EXPECT_NEAR(out.read().power(), 0.5, 1e-4);      // one input: 3 dB intrinsic loss
        in2.write(cw(1, 0));
        specsGlobalConfig.runOPAnalysis();
        EXPECT_NEAR(out.read().power(), 2.0, 1e-4);      // two in-phase 1 W inputs: (1+1)^2 / 2
        in2.write(cw(-1, 0));
        specsGlobalConfig.runOPAnalysis();
        EXPECT_NEAR(out.read().power(), 0.0, 1e-4);      // opposite phase cancels
    }
    SPECS_TEST_FOOTER();
}
