// B-048: a disabled PROBE / MLPROBE samples the current value of its net when it is enabled again,
// whether or not the net changed while it was disabled (runTRANAnalysis() disables the probes
// during the OP and enables them at TRAN start). The TRAN start sequence itself is tested in
// test/tran_start.cpp.
// B-184: an MLPROBE also traces, when it is enabled, the last value of every listed wavelength its
// net carried while it was disabled.
#include <gtest/gtest.h>
#include <systemc.h>

#include <cmath>
#include <complex>
#include <string>

#include "devices/probe.h"
#include "optical_output_port.h"
#include "specs.h"
#include "test/utils.h"

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
