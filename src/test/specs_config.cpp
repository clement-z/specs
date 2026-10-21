#include <gtest/gtest.h>
#include <systemc.h>

#include <limits>

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

        EXPECT_EXIT({ c.default_abstol = 0; c.verifyConfig(); }, rejected, "invalid configuration: abstol");
        EXPECT_EXIT({ c.default_abstol = -1; c.verifyConfig(); }, rejected, "invalid configuration: abstol");
        EXPECT_EXIT({ c.default_abstol = nan; c.verifyConfig(); }, rejected, "invalid configuration: abstol");
        EXPECT_EXIT({ c.default_reltol = 0; c.verifyConfig(); }, rejected, "invalid configuration: reltol");
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
