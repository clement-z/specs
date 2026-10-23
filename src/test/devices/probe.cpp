// B-048: a disabled PROBE / MLPROBE samples the current value of its net when it is enabled again,
// whether or not the net changed while it was disabled (runTRANAnalysis() disables the probes
// during the OP and enables them at TRAN start). The TRAN start sequence itself is tested in
// test/tran_start.cpp.
// B-184: an MLPROBE also traces, when it is enabled, the last value of every listed wavelength its
// net carried while it was disabled.
// PD5: the wavelength latch of a PROBE run by a netlist analysis (Probe::LatchMode).
// PD6: a netlist PROBE always traces its wavelength; wl=0 is ignored, with one warning.
// B-186: an MLPROBE traces each .dc point as a whole, 0 for a listed wavelength absent at that point.
#include <gtest/gtest.h>
#include <systemc.h>

#include <cmath>
#include <complex>
#include <cstdio>
#include <fstream>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <string>

#include "devices/probe.h"
#include "optical_output_port.h"
#include "parser/parse_tree.h"
#include "parser/parser_state.h"
#include "specs.h"
#include "test/utils.h"
#include "trace_table.h"

#include "parser.h"
#include "scanner.h"

namespace {

void setupConfig()
{
    specsGlobalConfig.analysis_type = SPECSConfig::TRAN;
    specsGlobalConfig.trace_all_optical_nets = false;
    specsGlobalConfig.trace_filename = testing::TempDir() + "specs_test_" + std::to_string(getpid());
    specsGlobalConfig.engine_timescale = SPECSConfig::ONE_FS;
    specsGlobalConfig.applyEngineResolution();
}

void expectProbeTraces(const Probe &probe, double power, double phase, double wavelength)
{
    EXPECT_DOUBLE_EQ(probe.m_trace_sig_power.read(), power);
    EXPECT_DOUBLE_EQ(probe.m_trace_sig_modulus.read(), std::sqrt(power));
    EXPECT_DOUBLE_EQ(probe.m_trace_sig_phase.read(), phase);
    EXPECT_DOUBLE_EQ(probe.m_trace_sig_wavelength.read(), wavelength);
}

void expectProbeTracesZero(const Probe &probe)
{
    EXPECT_EQ(probe.m_trace_sig_power.read(), 0);
    EXPECT_EQ(probe.m_trace_sig_modulus.read(), 0);
    EXPECT_EQ(probe.m_trace_sig_phase.read(), 0);
    EXPECT_EQ(probe.m_trace_sig_wavelength.read(), 0);
}

std::string readFile(const std::string &filename)
{
    std::ifstream f(filename);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Runs the delta cycles of the current time step, without advancing the time
void settle()
{
    while (sc_pending_activity_at_current_time())
        sc_start(SC_ZERO_TIME);
}

OpticalSignal sig(double re, double im, double wavelength)
{
    return OpticalSignal(OpticalSignal::field_type(re, im), wavelength);
}

// power, abs, phase traced by an MLPROBE at `wavelength`
void expectMLProbeTraces(const MLambdaProbe &probe, double wavelength, double power, double phase)
{
    const auto &sigs = probe.m_lambda_signals.at(wavelength);
    EXPECT_DOUBLE_EQ(sigs[0]->read(), power);
    EXPECT_DOUBLE_EQ(sigs[1]->read(), std::sqrt(power));
    EXPECT_DOUBLE_EQ(sigs[2]->read(), phase);
}

}

TEST(Probe, SamplesEventReceivedWhileDisabledWhenEnabled)
{
    SPECS_TEST_HEADER()
    {
        setupConfig();

        spx::oa_signal_type net{"net"};
        Probe probe("probe");
        probe.p_in(net);

        sc_start(); // initialise the threads
        probe.enable = sc_logic(0);
        sc_start();

        // The net changes while the probe is disabled: not traced
        net.write(OpticalSignal(std::polar(std::sqrt(1e-3), 0.5), 1.55e-6));
        sc_start();
        expectProbeTracesZero(probe);

        // Enabled: the probe samples the current value of the net (the old code skipped this sample)
        probe.enable = sc_logic(1);
        sc_start();
        expectProbeTraces(probe, 1e-3, 0.5, 1.55e-6);

        // ... and traces every event as before
        net.write(OpticalSignal(std::polar(std::sqrt(2e-3), -0.25), 1.55e-6));
        sc_start();
        expectProbeTraces(probe, 2e-3, -0.25, 1.55e-6);
    }
    SPECS_TEST_FOOTER();
}

TEST(Probe, SamplesUnchangedNetWhenEnabled)
{
    SPECS_TEST_HEADER()
    {
        setupConfig();

        // Initialised net: it never changes, so the probe gets no event at all
        spx::oa_signal_type net{"net", OpticalSignal(std::polar(std::sqrt(2e-3), -1.0), 1.31e-6)};
        Probe probe("probe");
        probe.p_in(net);

        sc_start(); // initialise the threads
        probe.enable = sc_logic(0);
        sc_start();
        expectProbeTracesZero(probe);

        // Enabled: the probe is woken by the enable and samples the net
        probe.enable = sc_logic(1);
        sc_start();
        expectProbeTraces(probe, 2e-3, -1.0, 1.31e-6);
    }
    SPECS_TEST_FOOTER();
}

TEST(Probe, UndrivenNetTracesNothingWhenEnabled)
{
    SPECS_TEST_HEADER()
    {
        setupConfig();

        // Never written: default OpticalSignal (wavelength id 0)
        spx::oa_signal_type net{"net"};
        Probe probe("probe");
        probe.p_in(net);

        sc_start(); // initialise the threads
        probe.enable = sc_logic(0);
        sc_start();

        // No wavelength registered yet: reading the wavelength of the default signal would exit
        probe.enable = sc_logic(1);
        sc_start();
        expectProbeTracesZero(probe);

        // Wavelength id 0 now aliases 1.31 um (B-012): the probe still traces nothing
        const OpticalSignal other(std::polar(std::sqrt(1e-3), 0.0), 1.31e-6);
        probe.enable = sc_logic(0);
        sc_start();
        probe.enable = sc_logic(1);
        sc_start();
        expectProbeTracesZero(probe);
    }
    SPECS_TEST_FOOTER();
}

TEST(MLambdaProbe, SamplesEventReceivedWhileDisabledWhenEnabled)
{
    SPECS_TEST_HEADER()
    {
        setupConfig();

        spx::oa_signal_type net{"net"};
        MLambdaProbe probe("mlprobe", {1.55e-6});
        probe.p_in(net);

        // Opens the trace file and gives it to the probe, which creates its per-wavelength signals
        specsGlobalConfig.prepareSimulation();
        sc_start(); // initialise the threads
        probe.enable = sc_logic(0);
        sc_start();

        // The net changes while the probe is disabled: not traced
        net.write(OpticalSignal(std::polar(std::sqrt(1e-3), 0.5), 1.55e-6));
        sc_start();
        expectMLProbeTraces(probe, 1.55e-6, 0, 0);

        // Enabled: the probe samples the current value of the net (the old code skipped this sample)
        probe.enable = sc_logic(1);
        sc_start();
        expectMLProbeTraces(probe, 1.55e-6, 1e-3, 0.5);

        // ... and traces every event as before
        net.write(OpticalSignal(std::polar(std::sqrt(2e-3), -0.25), 1.55e-6));
        sc_start();
        expectMLProbeTraces(probe, 1.55e-6, 2e-3, -0.25);
    }
    SPECS_TEST_FOOTER();
}

TEST(MLambdaProbe, SamplesUnchangedNetWhenEnabled)
{
    SPECS_TEST_HEADER()
    {
        setupConfig();

        // Initialised net: it never changes, so the probe gets no event at all
        spx::oa_signal_type net{"net", OpticalSignal(std::polar(std::sqrt(2e-3), -1.0), 1.31e-6)};
        MLambdaProbe probe("mlprobe", {1.31e-6, 1.55e-6});
        probe.p_in(net);

        specsGlobalConfig.prepareSimulation();
        sc_start(); // initialise the threads
        probe.enable = sc_logic(0);
        sc_start();
        expectMLProbeTraces(probe, 1.31e-6, 0, 0);

        // Enabled: the probe is woken by the enable and samples the net
        probe.enable = sc_logic(1);
        sc_start();
        expectMLProbeTraces(probe, 1.31e-6, 2e-3, -1.0);
        expectMLProbeTraces(probe, 1.55e-6, 0, 0);
    }
    SPECS_TEST_FOOTER();
}

TEST(MLambdaProbe, UndrivenNetPrintsNothingWhenEnabled)
{
    SPECS_TEST_HEADER()
    {
        setupConfig();

        // Registered first: wavelength id 0, which the default OpticalSignal aliases (B-012)
        const OpticalSignal unsupported(std::polar(std::sqrt(1e-3), 0.0), 1.3e-6);

        // Never written: default OpticalSignal
        spx::oa_signal_type net{"net"};
        MLambdaProbe probe("mlprobe", {1.55e-6});
        probe.p_in(net);

        specsGlobalConfig.prepareSimulation();
        sc_start(); // initialise the threads
        probe.enable = sc_logic(0);
        sc_start();

        // The sample taken at the enable is skipped silently: 1.3e-06 m is not noted as ignored
        testing::internal::CaptureStdout();
        probe.enable = sc_logic(1);
        sc_start();
        std::cout << std::flush;
        EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
        EXPECT_TRUE(probe.m_ignored_wavelengths_m.empty());
        expectMLProbeTraces(probe, 1.55e-6, 0, 0);

        // A real event at a wavelength the probe does not list is noted, without a print (B-062)
        testing::internal::CaptureStdout();
        testing::internal::CaptureStderr();
        net.write(unsupported);
        sc_start();
        std::cout << std::flush;
        EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
        EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
        EXPECT_EQ(probe.m_ignored_wavelengths_m, std::set<double>{1.3e-6});
    }
    SPECS_TEST_FOOTER();
}

TEST(MLambdaProbe, TracesEveryListedWavelengthReceivedWhileDisabledWhenEnabled)
{
    SPECS_TEST_HEADER()
    {
        setupConfig();

        spx::oa_signal_type net{"net"};
        MLambdaProbe probe("mlprobe", {1.55e-6, 1.56e-6});
        probe.p_in(net);

        specsGlobalConfig.prepareSimulation();
        sc_start(); // initialise the threads
        probe.enable = sc_logic(0);
        sc_start();

        // While disabled, the net carries both listed wavelengths, then an unlisted one: nothing
        // is traced or printed (B-184)
        testing::internal::CaptureStdout();
        net.write(OpticalSignal(std::polar(std::sqrt(1e-3), 0.5), 1.55e-6));
        sc_start();
        net.write(OpticalSignal(std::polar(std::sqrt(4e-3), 0.25), 1.56e-6));
        sc_start();
        net.write(OpticalSignal(std::polar(std::sqrt(2e-3), -0.5), 1.55e-6));
        sc_start();
        net.write(OpticalSignal(std::polar(std::sqrt(3e-3), 0.0), 1.31e-6));
        sc_start();
        std::cout << std::flush;
        EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
        expectMLProbeTraces(probe, 1.55e-6, 0, 0);
        expectMLProbeTraces(probe, 1.56e-6, 0, 0);

        // Enabled: the last value of each listed wavelength is traced; the current value of the
        // net (unlisted) is noted as ignored (B-048, B-062)
        testing::internal::CaptureStdout();
        probe.enable = sc_logic(1);
        sc_start();
        std::cout << std::flush;
        EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
        EXPECT_EQ(probe.m_ignored_wavelengths_m, std::set<double>{1.31e-6});
        expectMLProbeTraces(probe, 1.55e-6, 2e-3, -0.5);
        expectMLProbeTraces(probe, 1.56e-6, 4e-3, 0.25);

        // ... and every event afterwards, as before
        net.write(OpticalSignal(std::polar(std::sqrt(5e-3), 1.0), 1.56e-6));
        sc_start();
        expectMLProbeTraces(probe, 1.55e-6, 2e-3, -0.5);
        expectMLProbeTraces(probe, 1.56e-6, 5e-3, 1.0);

        // Disabled and enabled again: the values of the first disabled period are not traced again
        probe.enable = sc_logic(0);
        sc_start();
        net.write(OpticalSignal(std::polar(std::sqrt(6e-3), 0.0), 1.55e-6));
        sc_start();
        probe.enable = sc_logic(1);
        sc_start();
        expectMLProbeTraces(probe, 1.55e-6, 6e-3, 0.0);
        expectMLProbeTraces(probe, 1.56e-6, 5e-3, 1.0);
    }
    SPECS_TEST_FOOTER();
}

// B-186: in a .dc, an MLPROBE traces each point as a whole (SPECSConfig::runDCAnalysis() calls
// tracePoint() once the point converged): the last value of each listed wavelength received at this point,
// and 0 for a listed wavelength its net did not carry at this point (fresh steady state), in the VCD
// signals and in the table, in wavelength order
TEST(MLambdaProbe, DcPointTracesEveryListedWavelengthOrZero)
{
    SPECS_TEST_HEADER()
    {
        setupConfig();
        const std::string csv = testing::TempDir() + "specs_test_b186_" + std::to_string(getpid()) + ".csv";
        {
            TraceTable table(csv, "");
            spx::oa_signal_type net{"net"};
            MLambdaProbe probe("mlprobe", {1.55e-6, 1.56e-6, 1.57e-6});
            probe.p_in(net);

            specsGlobalConfig.prepareSimulation();
            probe.setTraceTable(&table);
            sc_start(); // initialise the threads
            probe.m_per_point = true;

            // Point 1: two listed wavelengths (1.55 twice: the last value counts) and an unlisted one;
            // nothing is traced before tracePoint()
            for (const auto &s : {sig(1, 0, 1.55e-6), sig(3, 0, 1.56e-6), sig(0, 2, 1.55e-6), sig(1, 0, 1.31e-6)})
            {
                net.write(s);
                settle();
            }
            expectMLProbeTraces(probe, 1.55e-6, 0, 0);
            expectMLProbeTraces(probe, 1.56e-6, 0, 0);
            probe.tracePoint();
            sc_start(sc_time::from_value(1));
            expectMLProbeTraces(probe, 1.55e-6, 4, std::atan2(2, 0));
            expectMLProbeTraces(probe, 1.56e-6, 9, 0);
            expectMLProbeTraces(probe, 1.57e-6, 0, 0);

            // Point 2: only 1.56; 1.55, not carried at this point, reads 0
            net.write(sig(1, 1, 1.56e-6));
            settle();
            probe.tracePoint();
            sc_start(sc_time::from_value(1));
            expectMLProbeTraces(probe, 1.55e-6, 0, 0);
            expectMLProbeTraces(probe, 1.56e-6, 2, std::atan2(1, 1));
            expectMLProbeTraces(probe, 1.57e-6, 0, 0);
            EXPECT_EQ(probe.m_ignored_wavelengths_m, std::set<double>{1.31e-6});
            probe.m_ignored_wavelengths_m.clear();

            EXPECT_TRUE(table.close());
            EXPECT_EQ(table.rows(), 6u);
        }
        EXPECT_EQ(readFile(csv),
                  "time_s,signal,wavelength_m,power_W,field_re,field_im\n"
                  "0,mlprobe,1.55e-06,4,0,2\n"
                  "0,mlprobe,1.56e-06,9,3,0\n"
                  "0,mlprobe,1.57e-06,0,0,0\n"
                  "1e-15,mlprobe,1.55e-06,0,0,0\n"
                  "1e-15,mlprobe,1.56e-06,2,1,1\n"
                  "1e-15,mlprobe,1.57e-06,0,0,0\n");
        std::remove(csv.c_str());
    }
    SPECS_TEST_FOOTER();
}

// B-062: an MLPROBE matches its listed wavelengths within OpticalSignal::WAVELENGTH_MATCH_RTOL, notes
// the wavelengths it does not list and reports them in one warning on stderr, with every digit needed
// to tell them from the listed ones; the destructor reports what was not reported yet, so the probes of
// testbenches still warn
TEST(MLambdaProbe, ReportsIgnoredWavelengthsOnce)
{
    SPECS_TEST_HEADER()
    {
        setupConfig();

        spx::oa_signal_type net{"net"};
        auto probe = std::make_unique<MLambdaProbe>("mlprobe", std::set<double>{1.5515e-6, 1.56e-6});
        probe->p_in(net);

        specsGlobalConfig.prepareSimulation();
        sc_start(); // initialise the threads

        // 1.55e-6 + 1.5e-9 is not 1.5515e-6 but matches it within 1e-12 (traced at 1.5515e-6); a
        // wavelength 1e-11 away from it is not listed: ignored, like 1.31e-6; each one is noted once
        const double near_listed = 1.55e-6 + 1.5e-9;
        const double off_listed = 1.5515e-6 * (1 + 1e-11);
        ASSERT_NE(near_listed, 1.5515e-6);
        testing::internal::CaptureStdout();
        testing::internal::CaptureStderr();
        for (double wl : {off_listed, 1.31e-6, off_listed, 1.56e-6, 1.31e-6})
        {
            net.write(OpticalSignal(std::polar(std::sqrt(1e-3), 0.0), wl));
            sc_start();
        }
        net.write(OpticalSignal(std::polar(std::sqrt(2e-3), 0.0), near_listed));
        sc_start();
        std::cout << std::flush;
        EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
        EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
        EXPECT_EQ(probe->m_ignored_wavelengths_m, (std::set<double>{1.31e-6, off_listed}));
        expectMLProbeTraces(*probe, 1.56e-6, 1e-3, 0.0);
        expectMLProbeTraces(*probe, 1.5515e-6, 2e-3, 0.0);

        testing::internal::CaptureStderr();
        probe->reportIgnoredWavelengths();
        probe->reportIgnoredWavelengths(); // nothing new: no second warning
        EXPECT_EQ(testing::internal::GetCapturedStderr(),
                  "Warning: mlprobe ignored the wavelengths of its net net that it does not list "
                  "(1.31e-06 m, 1.5515000000155152e-06 m; listed: 1.5515e-06 m, 1.56e-06 m)\n");

        // Noted after the report: reported by the destructor
        net.write(OpticalSignal(std::polar(std::sqrt(1e-3), 0.0), 1.6e-6));
        sc_start();
        testing::internal::CaptureStderr();
        probe.reset();
        EXPECT_EQ(testing::internal::GetCapturedStderr(),
                  "Warning: mlprobe ignored the wavelengths of its net net that it does not list "
                  "(1.6e-06 m; listed: 1.5515e-06 m, 1.56e-06 m)\n");
    }
    SPECS_TEST_FOOTER();
}

TEST(Probe, LatchOnceKeepsTheSmallestWavelengthOfTheFirstTimeStep)
{
    SPECS_TEST_HEADER()
    {
        setupConfig();
        const std::string csv = testing::TempDir() + "specs_test_latch_" + std::to_string(getpid()) + ".csv";
        {
            TraceTable table(csv, "");
            spx::oa_signal_type net{"net"};
            Probe probe("probe");
            probe.p_in(net);
            probe.setTraceTable(&table);
            probe.m_latch_mode = Probe::LATCH_ONCE; // .tran

            sc_start(); // initialise the threads

            // First time step that brings a wavelength: 1560 nm, 1555 nm, then 1550 nm in later delta
            // cycles. The smallest is latched; the other samples leave the table
            net.write(sig(2, 0, 1.56e-6));
            settle();
            expectProbeTraces(probe, 4, 0, 1.56e-6);
            net.write(sig(3, 0, 1.555e-6));
            settle();
            expectProbeTraces(probe, 9, 0, 1.555e-6);
            net.write(sig(1, 0, 1.55e-6));
            settle();
            expectProbeTraces(probe, 1, 0, 1.55e-6);
            sc_start(1, SC_PS);

            // Later time steps: other wavelengths, even smaller ones, are ignored
            net.write(sig(3, 0, 1.54e-6));
            sc_start(1, SC_PS);
            net.write(sig(3, 0, 1.56e-6));
            sc_start(1, SC_PS);
            expectProbeTraces(probe, 1, 0, 1.55e-6);
            net.write(sig(0, 2, 1.55e-6));
            sc_start(1, SC_PS);
            expectProbeTraces(probe, 4, M_PI / 2, 1.55e-6);

            // The warning reports the first wavelength ignored, and the one latched
            EXPECT_EQ(probe.m_ignored_wavelength_m, 1.56e-6);
            EXPECT_EQ(probe.m_traced_when_ignored_m, 1.55e-6);
            EXPECT_TRUE(table.close());
        }
        EXPECT_EQ(readFile(csv),
                  "time_s,signal,wavelength_m,power_W,field_re,field_im\n"
                  "0,probe,1.55e-06,1,1,0\n"
                  "3e-12,probe,1.55e-06,4,0,2\n");
        std::remove(csv.c_str());
    }
    SPECS_TEST_FOOTER();
}

TEST(Probe, LatchOnceAtEnableTakesTheSmallestWavelengthReceivedWhileDisabled)
{
    SPECS_TEST_HEADER()
    {
        setupConfig();

        spx::oa_signal_type net{"net"};
        Probe probe("probe");
        probe.p_in(net);
        probe.m_latch_mode = Probe::LATCH_ONCE; // .tran

        sc_start(); // initialise the threads
        probe.enable = sc_logic(0);
        sc_start();

        // While disabled (the OP): 1560 nm, 1550 nm, then 1560 nm again (the value the net holds)
        net.write(OpticalSignal(std::polar(std::sqrt(4e-3), 0.25), 1.56e-6));
        sc_start();
        net.write(OpticalSignal(std::polar(std::sqrt(1e-3), 0.5), 1.55e-6));
        sc_start();
        net.write(OpticalSignal(std::polar(std::sqrt(5e-3), 0.0), 1.56e-6));
        sc_start();
        expectProbeTracesZero(probe);

        // Enabled: the smallest wavelength, with its last value, not the net's current value
        probe.enable = sc_logic(1);
        sc_start();
        expectProbeTraces(probe, 1e-3, 0.5, 1.55e-6);
        EXPECT_EQ(probe.m_ignored_wavelength_m, 1.56e-6);

        // Latched for the rest of the run, also against a smaller wavelength in the same time step
        net.write(OpticalSignal(std::polar(std::sqrt(3e-3), 0.0), 1.54e-6));
        sc_start();
        net.write(OpticalSignal(std::polar(std::sqrt(6e-3), 0.0), 1.56e-6));
        sc_start();
        expectProbeTraces(probe, 1e-3, 0.5, 1.55e-6);
        net.write(OpticalSignal(std::polar(std::sqrt(2e-3), -0.5), 1.55e-6));
        sc_start();
        expectProbeTraces(probe, 2e-3, -0.5, 1.55e-6);
    }
    SPECS_TEST_FOOTER();
}

TEST(Probe, LatchPerTimeStepTakesTheSmallestWavelengthOfEachPoint)
{
    SPECS_TEST_HEADER()
    {
        setupConfig();

        spx::oa_signal_type net{"net"};
        Probe probe("probe");
        probe.p_in(net);
        probe.m_latch_mode = Probe::LATCH_PER_TIME_STEP; // .op, .dc

        sc_start(); // initialise the threads

        // Point 1: 1560 nm only
        net.write(sig(2, 0, 1.56e-6));
        sc_start(1, SC_PS);
        expectProbeTraces(probe, 4, 0, 1.56e-6);
        EXPECT_TRUE(std::isnan(probe.m_ignored_wavelength_m));

        // Point 2: 1560 nm, then 1550 nm: 1550 nm, whatever the order
        net.write(sig(3, 0, 1.56e-6));
        settle();
        net.write(sig(1, 0, 1.55e-6));
        settle();
        net.write(sig(5, 0, 1.56e-6));
        sc_start(1, SC_PS);
        expectProbeTraces(probe, 1, 0, 1.55e-6);
        EXPECT_EQ(probe.m_ignored_wavelength_m, 1.56e-6);

        // Point 3: 1560 nm only again (a wavelength sweep: each point has its own)
        net.write(sig(0, 1, 1.56e-6));
        sc_start(1, SC_PS);
        expectProbeTraces(probe, 1, M_PI / 2, 1.56e-6);
    }
    SPECS_TEST_FOOTER();
}

TEST(Probe, LatchPerTimeStepPrefersTheSweptWavelengthOfADcPoint)
{
    SPECS_TEST_HEADER()
    {
        setupConfig();

        spx::oa_signal_type net{"net"};
        Probe probe("probe");
        probe.p_in(net);
        probe.m_latch_mode = Probe::LATCH_PER_TIME_STEP; // .dc

        sc_start(); // initialise the threads

        // Point 1 of a wavelength sweep at 1550 nm: a smaller fixed wavelength comes first, then the
        // swept one, which replaces it
        probe.setSweptWavelength(1.55e-6);
        net.write(sig(2, 0, 1.53e-6));
        settle();
        net.write(sig(1, 0, 1.55e-6));
        settle();
        sc_start(1, SC_PS);
        expectProbeTraces(probe, 1, 0, 1.55e-6);
        EXPECT_EQ(probe.m_ignored_wavelength_m, 1.53e-6);
        EXPECT_EQ(probe.m_traced_when_ignored_m, 1.55e-6);

        // Point 2 at 1560 nm: the swept wavelength first, then a smaller one, which is ignored
        probe.setSweptWavelength(1.56e-6);
        net.write(sig(3, 0, 1.56e-6));
        settle();
        net.write(sig(2, 0, 1.53e-6));
        settle();
        sc_start(1, SC_PS);
        expectProbeTraces(probe, 9, 0, 1.56e-6);

        // Point 3 at 1570 nm: the net does not carry it, the smallest one present is latched
        probe.setSweptWavelength(1.57e-6);
        net.write(sig(1, 0, 1.54e-6));
        settle();
        net.write(sig(0, 1, 1.53e-6));
        settle();
        sc_start(1, SC_PS);
        expectProbeTraces(probe, 1, M_PI / 2, 1.53e-6);

        // Not a wavelength sweep (e.g. power): the smallest one
        probe.setSweptWavelength(std::numeric_limits<double>::quiet_NaN());
        net.write(sig(4, 0, 1.55e-6));
        settle();
        net.write(sig(2, 0, 1.54e-6));
        settle();
        sc_start(1, SC_PS);
        expectProbeTraces(probe, 4, 0, 1.54e-6);
    }
    SPECS_TEST_FOOTER();
}

TEST(Probe, TestbenchProbeTracesEveryWavelength)
{
    SPECS_TEST_HEADER()
    {
        setupConfig();

        // No analysis sets the latch (testbenches, C++ users): every event is traced, as before PD5
        spx::oa_signal_type net{"net"};
        Probe probe("probe");
        probe.p_in(net);
        EXPECT_EQ(probe.m_latch_mode, Probe::LATCH_OFF);

        sc_start(); // initialise the threads
        net.write(sig(1, 0, 1.55e-6));
        sc_start(1, SC_PS);
        net.write(sig(2, 0, 1.56e-6));
        sc_start(1, SC_PS);
        expectProbeTraces(probe, 4, 0, 1.56e-6);
        EXPECT_TRUE(std::isnan(probe.m_ignored_wavelength_m));
    }
    SPECS_TEST_FOOTER();
}

TEST(Probe, NetlistProbeAlwaysTracesTheWavelength)
{
    SPECS_TEST_HEADER()
    {
        setupConfig();

        // PROBE1..3 ask not to trace the wavelength (keyword, keyword alias, 4th argument): ignored, one
        // warning for all, in name order. PROBE6's keyword overrides its false 4th argument (no warning).
        // .save P(a) traces exactly the power
        const std::string netlist =
            "cwsrc1 a wl=1.55e-6 power=1e-3\n"
            "probe3 a 1 1 1 0\n"
            "probe1 a wl=0\n"
            "probe2 a wavelength=0.0\n"
            "probe4 a wl=1\n"
            "probe5 a 0 0 0 1\n"
            "probe6 a 1 1 1 0 wl=1\n"
            ".save P(a)\n";
        ParseTree tree;
        ParserState state;
        state.current_filename = "test.cir";
        yyscan_t scanner;
        yylex_init_extra(&state, &scanner);
        YY_BUFFER_STATE buf = yy_scan_string(netlist.c_str(), scanner);
        yy_switch_to_buffer(buf, scanner);
        yyset_lineno(1, scanner);
        ASSERT_EQ(yyparse(scanner, &tree), 0);
        yylex_destroy(scanner);

        testing::internal::CaptureStderr();
        tree.build_circuit();
        EXPECT_EQ(testing::internal::GetCapturedStderr(),
                  "Warning: the wavelength of a PROBE is always traced since v0.3: wl=0 (or a false 4th argument) "
                  "ignored for ROOT/PROBE1, ROOT/PROBE2, ROOT/PROBE3\n");

        for (const char *name : {"ROOT/PROBE1", "ROOT/PROBE2", "ROOT/PROBE3", "ROOT/PROBE4", "ROOT/PROBE5",
                                 "ROOT/PROBE6"})
        {
            const auto *probe = dynamic_cast<const Probe *>(sc_find_object(name));
            ASSERT_NE(probe, nullptr) << name;
            EXPECT_TRUE(probe->m_trace_wavelength) << name;
        }
        const auto *probe5 = dynamic_cast<const Probe *>(sc_find_object("ROOT/PROBE5"));
        EXPECT_FALSE(probe5->m_trace_power);
        EXPECT_FALSE(probe5->m_trace_modulus);
        EXPECT_FALSE(probe5->m_trace_phase);

        const auto *saved = dynamic_cast<const Probe *>(sc_find_object("PROBE{ROOT/a}"));
        ASSERT_NE(saved, nullptr);
        EXPECT_TRUE(saved->m_trace_power);
        EXPECT_FALSE(saved->m_trace_modulus);
        EXPECT_FALSE(saved->m_trace_phase);
        EXPECT_FALSE(saved->m_trace_wavelength);
    }
    SPECS_TEST_FOOTER();
}
