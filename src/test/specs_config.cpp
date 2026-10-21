#include <gtest/gtest.h>
#include <systemc.h>

#include <algorithm>
#include <limits>
#include <string>

#include "specs.h"

#include "test/utils.h"

TEST(SPECSConfig, VerifyConfigAcceptsValidValues)
{
    SPECS_TEST_HEADER()
    {
        SPECSConfig &c = specsGlobalConfig;
        c.verifyConfig(); // defaults

        c.engine_timescale = SPECSConfig::ONE_FS;
        c.verifyConfig();
        c.engine_timescale = SPECSConfig::ONE_SEC;
        c.verifyConfig();

        c.default_snap_factor = 0; // disables the snap to 0 (B-021)
        c.verifyConfig();

        // zero tolerances are allowed (B-153, decision 2026-10-21); both at 0: ZeroTolerancesWarnOnce
        c.default_abstol = 0;
        c.verifyConfig();
        c.default_abstol = 1e-8;
        c.default_reltol = 0;
        c.verifyConfig();
    }
    SPECS_TEST_FOOTER();
}

// verifyConfig() must reject out-of-range values with a clean error (exit 1),
// also when NDEBUG is defined (B-018: it was assert-only).
TEST(SPECSConfigDeathTest, VerifyConfigRejectsInvalidValues)
{
    SPECS_TEST_HEADER()
    {
        SPECSConfig &c = specsGlobalConfig;
        const auto rejected = testing::ExitedWithCode(1);
        const double nan = std::numeric_limits<double>::quiet_NaN();

        // tolerances: 0 is allowed (B-153), negative values and NaN are not
        EXPECT_EXIT({ c.default_abstol = -1; c.verifyConfig(); }, rejected, "invalid configuration: abstol must be >= 0");
        EXPECT_EXIT({ c.default_abstol = -1e-300; c.verifyConfig(); }, rejected, "invalid configuration: abstol must be >= 0");
        EXPECT_EXIT({ c.default_abstol = nan; c.verifyConfig(); }, rejected, "invalid configuration: abstol must be >= 0");
        EXPECT_EXIT({ c.default_reltol = -1e-3; c.verifyConfig(); }, rejected, "invalid configuration: reltol must be >= 0");
        EXPECT_EXIT({ c.default_reltol = nan; c.verifyConfig(); }, rejected, "invalid configuration: reltol must be >= 0");
        EXPECT_EXIT({ c.default_snap_factor = -1; c.verifyConfig(); }, rejected, "invalid configuration: snap factor");
        EXPECT_EXIT({ c.default_snap_factor = nan; c.verifyConfig(); }, rejected, "invalid configuration: snap factor");
        EXPECT_EXIT({ c.default_resolution_multiplier = 0; c.verifyConfig(); }, rejected, "invalid configuration: resolution");
        EXPECT_EXIT({ c.engine_timescale = SPECSConfig::EngineTimescale(-16); c.verifyConfig(); }, rejected, "invalid configuration: timescale");
        EXPECT_EXIT({ c.engine_timescale = SPECSConfig::EngineTimescale(1); c.verifyConfig(); }, rejected, "invalid configuration: timescale");
        EXPECT_EXIT({ c.simulation_mode = PORT_MODE_MAXVAL; c.verifyConfig(); }, rejected, "invalid configuration: .*simulation mode");
        // B-002: not implemented (delayedWriteSampledTime() drops every write)
        EXPECT_EXIT({ c.simulation_mode = SAMPLED_TIME; c.verifyConfig(); }, rejected, "invalid configuration: sampled-time simulation mode is not implemented");
        EXPECT_EXIT({ c.analysis_type = SPECSConfig::ANALYSIS_TYPE_MAXVAL; c.verifyConfig(); }, rejected, "invalid configuration: .*analysis type");
    }
    SPECS_TEST_FOOTER();
}

// Zero tolerances (B-153): with abstol and reltol both 0, verifyConfig() warns once per run, although
// a netlist run calls it twice (runAnalysis(), then prepareSimulation()); with only one of them at 0
// it does not warn
TEST(SPECSConfig, ZeroTolerancesWarnOnce)
{
    SPECS_TEST_HEADER()
    {
        SPECSConfig &c = specsGlobalConfig;

        testing::internal::CaptureStderr();
        c.default_abstol = 0;
        c.verifyConfig();
        c.default_abstol = 1e-8;
        c.default_reltol = 0;
        c.verifyConfig();
        EXPECT_EQ(testing::internal::GetCapturedStderr(), "") << "one tolerance at 0: no warning";
        EXPECT_FALSE(c.zero_tolerances_warned);

        testing::internal::CaptureStderr();
        c.default_abstol = 0;
        c.verifyConfig(); // runAnalysis()
        c.verifyConfig(); // prepareSimulation()
        const string err = testing::internal::GetCapturedStderr();
        EXPECT_EQ(std::count(err.begin(), err.end(), '\n'), 1) << "exactly one line:\n" << err;
        EXPECT_EQ(err.rfind("Warning: abstol = 0 and reltol = 0: the emission gate is off", 0), 0u) << err;
        EXPECT_NE(err.find("maxiter"), string::npos) << err;
        EXPECT_TRUE(c.zero_tolerances_warned);
    }
    SPECS_TEST_FOOTER();
}
