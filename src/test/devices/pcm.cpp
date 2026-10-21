// PCMElement parameter validation (B-060): n > 0 and 0 <= k <= n (k = initial state). check_parameters() is
// called by the netlist parser and again by start_of_simulation() for C++ users. The model is not tested here.
#include <gtest/gtest.h>
#include <systemc.h>

#include "devices/pcm_device.h"
#include "optical_output_port.h"
#include "specs.h"
#include "test/utils.h"

TEST(PCMElementDeathTest, CheckParametersRejectsInvalidStates)
{
    SPECS_TEST_HEADER()
    {
        const auto rejected = testing::ExitedWithCode(1);
        PCMElement pcm("pcm", 1e-12, 10, 0, 0.85, 0.95);

        // valid: k = 0 and k = n
        pcm.check_parameters();
        pcm.m_stateCurrent = 10;
        pcm.check_parameters();
        pcm.m_stateCurrent = 0;

        EXPECT_EXIT({ pcm.m_nStates = 0; pcm.check_parameters(); }, rejected, "pcm.: the number of states n must be > 0 .got 0.");
        EXPECT_EXIT({ pcm.m_nStates = -1; pcm.check_parameters(); }, rejected, "pcm.: the number of states n must be > 0 .got -1.");
        EXPECT_EXIT({ pcm.m_stateCurrent = -1; pcm.check_parameters(); }, rejected, "pcm.: the initial state k must be in .0, n. = .0, 10. .got -1.");
        EXPECT_EXIT({ pcm.m_stateCurrent = 11; pcm.check_parameters(); }, rejected, "pcm.: the initial state k must be in .0, n. = .0, 10. .got 11.");
    }
    SPECS_TEST_FOOTER();
}

TEST(PCMElementDeathTest, StartOfSimulationChecksParameters)
{
    SPECS_TEST_HEADER()
    {
        specsGlobalConfig.analysis_type = SPECSConfig::OP;
        specsGlobalConfig.trace_all_optical_nets = false;
        specsGlobalConfig.trace_filename = testing::TempDir() + "specs_test_" + std::to_string(getpid());
        specsGlobalConfig.engine_timescale = SPECSConfig::ONE_FS;
        specsGlobalConfig.applyEngineResolution();

        spx::oa_signal_type in{"in"};
        spx::oa_signal_type out{"out"};
        PCMElement pcm("pcm", 1e-12, 10, 11, 0.85, 0.95); // k > n: not checked by the constructor
        pcm.p_in(in);
        pcm.p_out(out);

        EXPECT_EXIT({ specsGlobalConfig.prepareSimulation(); sc_start(SC_ZERO_TIME); },
                    testing::ExitedWithCode(1), "pcm.: the initial state k must be in .0, n. = .0, 10. .got 11.");

        // k = n is valid: the simulation starts
        pcm.m_stateCurrent = 10;
        specsGlobalConfig.prepareSimulation();
        sc_start(SC_ZERO_TIME);
    }
    SPECS_TEST_FOOTER();
}
