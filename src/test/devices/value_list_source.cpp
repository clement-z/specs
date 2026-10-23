// Value-list sources (B-066): the per-value prints (emissions, "was enabled", the values read from a
// file and its skipped lines) only appear with --vci (specsGlobalConfig.verbose_component_initialization);
// the values whose time is in the past when the source reaches them are skipped and reported in one
// warning on stderr, printed at the enable, with their count.
#include <gtest/gtest.h>
#include <systemc.h>

#include <cmath>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "devices/electrical_value_list_source.h"
#include "devices/value_list_source.h"
#include "optical_signal.h"
#include "specs.h"
#include "test/utils.h"

namespace {
void init_config(bool vci)
{
    specsGlobalConfig.analysis_type = SPECSConfig::TRAN;
    specsGlobalConfig.trace_all_optical_nets = false;
    specsGlobalConfig.trace_filename = testing::TempDir() + "specs_test_" + std::to_string(getpid());
    specsGlobalConfig.engine_timescale = SPECSConfig::ONE_FS;
    specsGlobalConfig.applyEngineResolution();
    specsGlobalConfig.verbose_component_initialization = vci;
}

// Records every value change of an optical net as (time, power)
class OpticalRecorder : public sc_module {
public:
    sc_in<spx::oa_value_type> p_in;
    std::vector<std::pair<sc_time, double>> m_changes;

    void on_p_in_changed()
    {
        m_changes.emplace_back(sc_time_stamp(), p_in.read().power());
    }

    OpticalRecorder(sc_module_name name)
        : sc_module(name)
    {
        SC_METHOD(on_p_in_changed);
        sensitive << p_in;
        dont_initialize();
    }
};

// Records every value change of an electrical net as (time, value)
class ElectricalRecorder : public sc_module {
public:
    sc_in<spx::ea_value_type> p_in;
    std::vector<std::pair<sc_time, double>> m_changes;

    void on_p_in_changed()
    {
        m_changes.emplace_back(sc_time_stamp(), p_in.read());
    }

    ElectricalRecorder(sc_module_name name)
        : sc_module(name)
    {
        SC_METHOD(on_p_in_changed);
        sensitive << p_in;
        dont_initialize();
    }
};

// 1 mW at 1 ps, 2 mW at 2 ps, 0 W at 3 ps (1550 nm)
std::vector<VLSource::time_value_pair_type> optical_values()
{
    return { { 1e-12, OpticalSignal(std::sqrt(1e-3), 1550e-9) },
             { 2e-12, OpticalSignal(std::sqrt(2e-3), 1550e-9) },
             { 3e-12, OpticalSignal(0.0, 1550e-9) } };
}

// Runs a VLSource enabled at enable_time for 1 ns and returns (stdout, stderr) of the run
std::pair<std::string, std::string> run_vlsource(VLSource &src, const sc_time &enable_time)
{
    specsGlobalConfig.prepareSimulation();
    sc_start(SC_ZERO_TIME);
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    if (enable_time != SC_ZERO_TIME)
        sc_start(enable_time);
    src.enable = sc_logic(1);
    sc_start(1, SC_NS);
    std::cout << std::flush;
    std::string out = testing::internal::GetCapturedStdout();
    std::string err = testing::internal::GetCapturedStderr();
    return { out, err };
}

std::string write_value_file(const std::string &tag, const std::string &contents)
{
    const std::string filename = testing::TempDir() + "specs_test_" + tag + "_" + std::to_string(getpid()) + ".csv";
    std::ofstream f(filename);
    f << contents;
    return filename;
}
}

TEST(VLSource, EmitsWithoutPrintingByDefault)
{
    SPECS_TEST_HEADER()
    {
        init_config(false);
        spx::oa_signal_type net{"net"};
        VLSource src("vlsrc", optical_values());
        src.p_out(net);
        OpticalRecorder rec("rec");
        rec.p_in(net);

        const auto [out, err] = run_vlsource(src, SC_ZERO_TIME);
        EXPECT_EQ(out, "");
        EXPECT_EQ(err, "");
        // the initial 0 at the enable is not a change of the net: three changes, at their times
        ASSERT_EQ(rec.m_changes.size(), 3u);
        EXPECT_EQ(rec.m_changes[0].first, sc_time(1, SC_PS));
        EXPECT_DOUBLE_EQ(rec.m_changes[0].second, 1e-3);
        EXPECT_EQ(rec.m_changes[1].first, sc_time(2, SC_PS));
        EXPECT_DOUBLE_EQ(rec.m_changes[1].second, 2e-3);
        EXPECT_EQ(rec.m_changes[2].first, sc_time(3, SC_PS));
        EXPECT_EQ(rec.m_changes[2].second, 0.0);
    }
    SPECS_TEST_FOOTER();
}

TEST(VLSource, PrintsEnableAndEmissionsWithVci)
{
    SPECS_TEST_HEADER()
    {
        init_config(true);
        spx::oa_signal_type net{"net"};
        VLSource src("vlsrc", optical_values());
        src.p_out(net);
        OpticalRecorder rec("rec");
        rec.p_in(net);

        const auto [out, err] = run_vlsource(src, SC_ZERO_TIME);
        EXPECT_EQ(err, "");
        EXPECT_NE(out.find("vlsrc was enabled\n"), std::string::npos) << out;
        EXPECT_NE(out.find("@0 s, vlsrc emitted: "), std::string::npos) << out;
        EXPECT_NE(out.find("@1 ps, vlsrc emitted: "), std::string::npos) << out;
        EXPECT_NE(out.find("@2 ps, vlsrc emitted: "), std::string::npos) << out;
        EXPECT_NE(out.find("@3 ps, vlsrc emitted: "), std::string::npos) << out;
        EXPECT_EQ(rec.m_changes.size(), 3u);
    }
    SPECS_TEST_FOOTER();
}

TEST(VLSource, ValuesInThePastGiveOneWarning)
{
    SPECS_TEST_HEADER()
    {
        init_config(false);
        spx::oa_signal_type net{"net"};
        VLSource src("vlsrc", optical_values());
        src.p_out(net);
        OpticalRecorder rec("rec");
        rec.p_in(net);

        // enabled at 2.5 ps: the values at 1 ps and 2 ps are skipped, the one at 3 ps (0 W) is emitted
        const auto [out, err] = run_vlsource(src, sc_time(2.5, SC_PS));
        EXPECT_EQ(out, "");
        EXPECT_EQ(err, "Warning: vlsrc: invalid time for signal emission (in the past) for 2 of 3 values, "
                       "not emitted\n");
        EXPECT_TRUE(rec.m_changes.empty()); // 0 W at 3 ps: the net does not change
        EXPECT_EQ(net.read().power(), 0.0);
    }
    SPECS_TEST_FOOTER();
}

TEST(VLSource, ValueFilePrintsOnlyWithVci)
{
    SPECS_TEST_HEADER()
    {
        // the first line is always discarded (B-053); then a header, an empty and a commented line
        const std::string filename = write_value_file("vlsrc_values",
            "first line\ntime,power,wavelength\n1e-12,1e-3,1.55e-6\n\n; comment\n2e-12,2e-3,1.55e-6\n");
        init_config(false);
        for (bool vci : { false, true })
        {
            specsGlobalConfig.verbose_component_initialization = vci;
            VLSource src(vci ? "vlsrc_vci" : "vlsrc");
            testing::internal::CaptureStdout();
            src.setValues(filename);
            std::cout << std::flush;
            const std::string out = testing::internal::GetCapturedStdout();
            if (vci)
                EXPECT_EQ(out, "Skipping header line: time,power,wavelength\n"
                               "1e-12: 0.001W @1550nm\n"
                               "Skipping empty line.\n"
                               "Skipping commented line: ; comment\n"
                               "2e-12: 0.002W @1550nm\n");
            else
                EXPECT_EQ(out, "");
            ASSERT_EQ(src.m_values_queue.size(), 2u);
            EXPECT_EQ(src.m_values_queue[1].first, 2e-12);
            EXPECT_DOUBLE_EQ(src.m_values_queue[1].second.power(), 2e-3);
        }
        std::remove(filename.c_str());
    }
    SPECS_TEST_FOOTER();
}

TEST(EVLSource, EmitsWithoutPrintingByDefaultAndWarnsOnceForNegativeTimes)
{
    SPECS_TEST_HEADER()
    {
        init_config(false);
        spx::ea_signal_type net{"net"};
        // EVLSource compares times in seconds: the negative ones are in the past at the enable
        EVLSource src("evlsrc", { { -2e-12, 5.0 }, { -1e-12, 4.0 }, { 1e-12, 1.5 }, { 2e-12, -0.5 } });
        src.p_out(net);
        ElectricalRecorder rec("rec");
        rec.p_in(net);

        specsGlobalConfig.prepareSimulation();
        sc_start(SC_ZERO_TIME);
        testing::internal::CaptureStdout();
        testing::internal::CaptureStderr();
        src.enable = sc_logic(1);
        sc_start(1, SC_NS);
        std::cout << std::flush;
        EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
        EXPECT_EQ(testing::internal::GetCapturedStderr(),
                  "Warning: evlsrc: invalid time for signal emission (in the past) for 2 of 4 values, not emitted\n");
        ASSERT_EQ(rec.m_changes.size(), 2u);
        EXPECT_EQ(rec.m_changes[0], std::make_pair(sc_time(1, SC_PS), 1.5));
        EXPECT_EQ(rec.m_changes[1], std::make_pair(sc_time(2, SC_PS), -0.5));
    }
    SPECS_TEST_FOOTER();
}

TEST(EVLSource, PrintsEnableEmissionsAndValueFileWithVci)
{
    SPECS_TEST_HEADER()
    {
        init_config(true);
        const std::string filename = write_value_file("evlsrc_values", "first line\n1e-12,1.5\n2e-12,-0.5\n");
        spx::ea_signal_type net{"net"};
        EVLSource src("evlsrc");
        testing::internal::CaptureStdout();
        src.setValues(filename);
        std::cout << std::flush;
        EXPECT_EQ(testing::internal::GetCapturedStdout(), "1e-12: 1.5V\n2e-12: -0.5V\n");
        std::remove(filename.c_str());
        src.p_out(net);

        specsGlobalConfig.prepareSimulation();
        sc_start(SC_ZERO_TIME);
        testing::internal::CaptureStdout();
        testing::internal::CaptureStderr();
        src.enable = sc_logic(1);
        sc_start(1, SC_NS);
        std::cout << std::flush;
        const std::string out = testing::internal::GetCapturedStdout();
        EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
        EXPECT_NE(out.find("evlsrc was enabled\n"), std::string::npos) << out;
        EXPECT_NE(out.find("@0 s, evlsrc emitted: 0\n"), std::string::npos) << out;
        EXPECT_NE(out.find("@1 ps, evlsrc emitted: 1.5\n"), std::string::npos) << out;
        EXPECT_NE(out.find("@2 ps, evlsrc emitted: -0.5\n"), std::string::npos) << out;
        EXPECT_EQ(net.read(), -0.5);
    }
    SPECS_TEST_FOOTER();
}
