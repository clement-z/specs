// F-28: the .op/.dc fixed point runs at most SPECSConfig::max_iterations delta cycles
// (runFixedPoint()); reportNonConvergence() then stops the run with the residual.
#include <gtest/gtest.h>
#include <systemc.h>

#include <cmath>
#include <complex>

#include "devices/cw_source.h"
#include "devices/waveguide.h"
#include "optical_output_port.h"
#include "specs.h"
#include "test/utils.h"

// The steps of runOPAnalysis() up to the fixed point
static void startOperatingPoint(SPECSConfig &c)
{
    c.verifyConfig();
    c.prepareSimulation();
    sc_start();
    for (auto &nodeset_order : c.nodeset_orders)
        nodeset_order.second.first->write(nodeset_order.second.second);
    for (auto oop : sc_get_all_module_by_type<OpticalOutputPort>())
        oop->m_mode = OpticalOutputPortMode::NO_DELAY;
    for (auto cws : sc_get_all_module_by_type<CWSource>())
        cws->enable = sc_logic(1);
}

// Two lossless waveguides in a closed loop a -> b -> a, with a field put on `a` (a nodeset):
// it circulates forever, its phase turning at every hop, so the fixed point never converges
TEST(FixedPoint, LosslessLoopStopsAtTheCapWithTheResidual)
{
    SPECS_TEST_HEADER()
    {
        SPECSConfig &c = specsGlobalConfig;
        c.analysis_type = SPECSConfig::OP;
        c.simulation_mode = OpticalOutputPortMode::FREQUENCY_DOMAIN;
        c.trace_filename = ""; // no trace file
        c.applyEngineResolution();

        spx::oa_signal_type a{"a"};
        spx::oa_signal_type b{"b"};
        WaveguideUni wg1("wg1", 1e-2, 0, 2.2, 4.2); // 100 um, lossless
        wg1.p_in(a);
        wg1.p_out(b);
        WaveguideUni wg2("wg2", 1.3e-2, 0, 2.2, 4.2); // 130 um, lossless
        wg2.p_in(b);
        wg2.p_out(a);
        c.nodeset_orders["a"] = {&a, OpticalSignal(std::polar(1.0, 0.0), 1.55e-6)};

        c.max_iterations = 1000;
        startOperatingPoint(c);
        const sc_dt::uint64 start_delta = sc_delta_count();
        EXPECT_FALSE(c.runFixedPoint());
        EXPECT_TRUE(sc_pending_activity_at_current_time());
        EXPECT_LE(sc_delta_count() - start_delta, 1000u);
        EXPECT_GT(sc_delta_count() - start_delta, 500u);
        EXPECT_EQ(sc_time_stamp(), SC_ZERO_TIME);
        // lossless: the circulating field keeps its modulus
        EXPECT_NEAR(std::abs(a.read().m_field), 1.0, 1e-12);
        EXPECT_NEAR(std::abs(b.read().m_field), 1.0, 1e-12);

        // both waveguides were still emitting, each change far beyond its threshold
        // max(abstol, reltol * |E_last|) = 1e-4 (sorted by that ratio, then by name)
        EXPECT_EXIT(c.reportNonConvergence(".op: the operating point", start_delta), testing::ExitedWithCode(1),
                    "^Error: .op: the operating point did not converge within 1000 iterations \\(delta cycles, "
                    ".options maxiter\\)\n"
                    "Residual: 2 optical output port\\(s\\) changed in the last [0-9]+ delta cycles\\. [^\n]*\n"
                    "  [ab] \\(wg[12]\\) @ 1550 nm: \\|dE\\| = [0-9.e+-]+ > 0\\.0001 \\([0-9.e+-]+ x\\)\n"
                    "  [ab] \\(wg[12]\\) @ 1550 nm: \\|dE\\| = [0-9.e+-]+ > 0\\.0001 \\([0-9.e+-]+ x\\)\n"
                    "A feedback loop [^\n]*maxiter \\(0: no limit\\)[^\n]*\n$");
    }
    SPECS_TEST_FOOTER();
}

// A converging circuit: stopped by a small cap, the fixed point resumes where it was and reaches
// the same operating point; max_iterations = 0 is no limit
TEST(FixedPoint, CapStopsAndResumesConvergingCircuit)
{
    SPECS_TEST_HEADER()
    {
        SPECSConfig &c = specsGlobalConfig;
        c.analysis_type = SPECSConfig::OP;
        c.simulation_mode = OpticalOutputPortMode::FREQUENCY_DOMAIN;
        c.trace_filename = "";
        c.applyEngineResolution();

        spx::oa_signal_type in{"in"};
        spx::oa_signal_type mid{"mid"};
        spx::oa_signal_type out{"out"};
        CWSource cw("cw");
        cw.setWavelength(1.55e-6);
        cw.setPower(1e-3);
        cw.p_out(in);
        WaveguideUni wg1("wg1", 1e-2, 0, 2.2, 4.2);
        wg1.p_in(in);
        wg1.p_out(mid);
        WaveguideUni wg2("wg2", 1e-2, 0, 2.2, 4.2);
        wg2.p_in(mid);
        wg2.p_out(out);

        c.max_iterations = 2;
        startOperatingPoint(c);
        EXPECT_FALSE(c.runFixedPoint());
        EXPECT_EQ(out.read().power(), 0); // not reached yet

        c.max_iterations = 0;
        EXPECT_TRUE(c.runFixedPoint());
        EXPECT_FALSE(sc_pending_activity_at_current_time());
        EXPECT_DOUBLE_EQ(out.read().power(), 1e-3);
        EXPECT_EQ(sc_time_stamp(), SC_ZERO_TIME);
    }
    SPECS_TEST_FOOTER();
}

TEST(FixedPoint, DefaultCap)
{
    EXPECT_EQ(SPECSConfig::DEFAULT_MAX_ITERATIONS, 1000000u);
    EXPECT_EQ(specsGlobalConfig.max_iterations, SPECSConfig::DEFAULT_MAX_ITERATIONS);
    EXPECT_FALSE(specsGlobalConfig.max_iterations_explicit);
}
