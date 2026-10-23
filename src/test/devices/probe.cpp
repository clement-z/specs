// B-048: a disabled PROBE / MLPROBE samples the current value of its net when it is enabled again,
// whether or not the net changed while it was disabled (runTRANAnalysis() disables the probes
// during the OP and enables them at TRAN start). The TRAN start sequence itself is tested in
// test/tran_start.cpp.
// B-184: an MLPROBE also traces, when it is enabled, the last value of every listed wavelength its
// net carried while it was disabled.
// PD5: the wavelength latch of a PROBE run by a netlist analysis (Probe::LatchMode, wl=).
#include <gtest/gtest.h>
#include <systemc.h>

#include <cmath>
#include <complex>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "devices/probe.h"
#include "optical_output_port.h"
#include "specs.h"
#include "test/utils.h"
#include "trace_table.h"

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

        // The sample taken at the enable is skipped silently: no
        // "received unsupported signal (lambda = 1.3e-06m)"
        testing::internal::CaptureStdout();
        probe.enable = sc_logic(1);
        sc_start();
        std::cout << std::flush;
        EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
        expectMLProbeTraces(probe, 1.55e-6, 0, 0);

        // A real event at a wavelength the probe does not list is still reported
        testing::internal::CaptureStdout();
        net.write(unsupported);
        sc_start();
        std::cout << std::flush;
        EXPECT_EQ(testing::internal::GetCapturedStdout(), "mlprobe received unsupported signal (lambda = 1.3e-06m)\n");
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
        // net (unlisted) is reported as before (B-048)
        testing::internal::CaptureStdout();
        probe.enable = sc_logic(1);
        sc_start();
        std::cout << std::flush;
        EXPECT_EQ(testing::internal::GetCapturedStdout(), "mlprobe received unsupported signal (lambda = 1.31e-06m)\n");
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

            // First time step that brings a wavelength: 1560 nm, then 1550 nm in the next delta cycle.
            // The smallest is latched; the 1560 nm sample leaves the table
            net.write(sig(2, 0, 1.56e-6));
            settle();
            expectProbeTraces(probe, 4, 0, 1.56e-6);
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

            // The warning reports the first wavelength ignored
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

TEST(Probe, SelectedWavelengthIsTheOnlyOneTraced)
{
    SPECS_TEST_HEADER()
    {
        setupConfig();

        spx::oa_signal_type net{"net"};
        Probe probe("probe");
        probe.p_in(net);
        probe.m_latch_mode = Probe::LATCH_ONCE; // .tran
        probe.m_wavelength_m = 1.56e-6;         // wl=1.56e-6

        sc_start(); // initialise the threads
        probe.enable = sc_logic(0);
        sc_start();
        net.write(OpticalSignal(std::polar(std::sqrt(4e-3), 0.25), 1.56e-6));
        sc_start();
        net.write(OpticalSignal(std::polar(std::sqrt(1e-3), 0.5), 1.55e-6));
        sc_start();

        // Enabled: the selected wavelength's value from the OP, although 1550 nm is smaller and current
        probe.enable = sc_logic(1);
        sc_start();
        expectProbeTraces(probe, 4e-3, 0.25, 1.56e-6);
        EXPECT_EQ(probe.m_ignored_wavelength_m, 1.55e-6);
        EXPECT_EQ(probe.m_traced_when_ignored_m, 1.56e-6);

        net.write(OpticalSignal(std::polar(std::sqrt(2e-3), 0.0), 1.55e-6));
        sc_start();
        expectProbeTraces(probe, 4e-3, 0.25, 1.56e-6);
        net.write(OpticalSignal(std::polar(std::sqrt(5e-3), -1.0), 1.56e-6));
        sc_start();
        expectProbeTraces(probe, 5e-3, -1.0, 1.56e-6);
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
