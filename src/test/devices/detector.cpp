// Event-driven photodetector (B-049, B-050): the readout is written to p_readout on input events and
// at an enable; periodic Ts samples only while >= 2 non-zero wavelengths beat, gated by the tolerance.
// Noise is bypassed and the dark current is 0 (Detector defaults), so the readouts are deterministic.
// No test lets a detector compute with a zero total field (the dark-field injection is random).
#include <gtest/gtest.h>
#include <systemc.h>

#include <cmath>
#include <complex>
#include <utility>
#include <vector>

#include "devices/cw_source.h"
#include "devices/detector.h"
#include "optical_output_port.h"
#include "specs.h"
#include "test/utils.h"

namespace {
void init_config()
{
    specsGlobalConfig.analysis_type = SPECSConfig::TIME_DOMAIN;
    specsGlobalConfig.trace_all_optical_nets = false;
    specsGlobalConfig.trace_filename = testing::TempDir() + "specs_test_" + std::to_string(getpid());
    specsGlobalConfig.engine_timescale = SPECSConfig::ONE_FS;
    specsGlobalConfig.applyEngineResolution();
}

// Records every value change of the readout net (sc_signal only notifies readers on a change)
class ReadoutRecorder : public sc_module {
public:
    sc_in<double> p_in;
    std::vector<std::pair<sc_time, double>> m_changes;

    void on_change()
    {
        m_changes.emplace_back(sc_time_stamp(), p_in.read());
    }

    ReadoutRecorder(sc_module_name name)
        : sc_module(name)
    {
        SC_HAS_PROCESS(ReadoutRecorder);
        SC_METHOD(on_change);
        sensitive << p_in;
        dont_initialize();
    }
};

class DetectorTest {
public:
    DetectorTest(double responsivity_A_W = 0.8, double sampling_time = 1e-12)
        : uut("uut", responsivity_A_W, 0, true, 1e9, 300, 400, 10e-12, sampling_time)
    {
        uut.p_in(in);
        uut.p_readout(readout);
        rec.p_in(readout);
    }

public:
    Detector uut;
    spx::oa_signal_type in{"in"};
    spx::ea_signal_type readout{"readout"};
    ReadoutRecorder rec{"rec"};
};

double freq_Hz(double wl)
{
    return 299792458 / wl;
}

// Readout of two interfering fields at time t: R * |E1 + E2 e^{j 2 pi (f2 - f1) t}|^2
double beat_readout(double R, std::complex<double> E1, double wl1, std::complex<double> E2, double wl2, double t)
{
    const double df = freq_Hz(wl2) - freq_Hz(wl1);
    return R * std::norm(E1 + E2 * std::exp(std::complex<double>(0, 2 * pi * df * t)));
}
}

TEST(Detector, EnabledByDefault)
{
    SPECS_TEST_HEADER()
    {
        init_config();
        DetectorTest tb;
        sc_start(SC_ZERO_TIME);
        EXPECT_EQ(tb.uut.enable.read(), SC_LOGIC_1);
    }
    SPECS_TEST_FOOTER();
}

TEST(Detector, WritesReadoutOnInputEvent)
{
    SPECS_TEST_HEADER()
    {
        init_config();
        DetectorTest tb(0.8);
        sc_start(SC_ZERO_TIME);

        const OpticalSignal::field_type E = std::sqrt(2e-3);
        tb.in.write(OpticalSignal(E, 1550e-9));
        sc_start(1, SC_NS);

        // readout = R * P on the electrical net (was never written, B-049) and in the traced member
        EXPECT_DOUBLE_EQ(tb.readout.read(), 0.8 * 2e-3);
        EXPECT_EQ(tb.readout.read(), tb.uut.m_cur_readout);
        EXPECT_EQ(tb.uut.m_cur_readout_no_interf, tb.uut.m_cur_readout);
        ASSERT_EQ(tb.rec.m_changes.size(), 1u);
        EXPECT_EQ(tb.rec.m_changes[0].first, SC_ZERO_TIME);
    }
    SPECS_TEST_FOOTER();
}

TEST(Detector, EnableWritesStoredReadoutAndDisableStopsWrites)
{
    SPECS_TEST_HEADER()
    {
        init_config();
        DetectorTest tb(1.0);
        tb.uut.enable = sc_logic(0); // before the simulation starts, as runOPAnalysis() does
        sc_start(SC_ZERO_TIME);

        // Disabled: the input memory is updated, but nothing is written
        tb.in.write(OpticalSignal(std::sqrt(1e-3), 1550e-9));
        sc_start(1, SC_NS);
        EXPECT_EQ(tb.readout.read(), 0.0);
        EXPECT_EQ(tb.uut.m_cur_readout, 0.0);
        EXPECT_TRUE(tb.rec.m_changes.empty());

        // Enabled later: the stored readout is written at the enable
        tb.uut.enable = sc_logic(1);
        sc_start(1, SC_NS);
        EXPECT_DOUBLE_EQ(tb.readout.read(), 1e-3);
        ASSERT_EQ(tb.rec.m_changes.size(), 1u);
        EXPECT_EQ(tb.rec.m_changes[0].first, sc_time(1, SC_NS));

        // Disabled during the simulation: input events are stored, not written
        tb.uut.enable = sc_logic(0);
        sc_start(1, SC_NS);
        tb.in.write(OpticalSignal(std::sqrt(3e-3), 1550e-9));
        sc_start(1, SC_NS);
        EXPECT_DOUBLE_EQ(tb.readout.read(), 1e-3);
        EXPECT_EQ(tb.rec.m_changes.size(), 1u);
        EXPECT_FALSE(sc_pending_activity());

        // Re-enabled: the new stored readout is written
        tb.uut.enable = sc_logic(1);
        sc_start(1, SC_NS);
        EXPECT_DOUBLE_EQ(tb.readout.read(), 3e-3);
        ASSERT_EQ(tb.rec.m_changes.size(), 2u);
        EXPECT_EQ(tb.rec.m_changes[1].first, sc_time(4, SC_NS));
    }
    SPECS_TEST_FOOTER();
}

TEST(Detector, SingleWavelengthWritesOncePerInputEvent)
{
    SPECS_TEST_HEADER()
    {
        init_config();
        DetectorTest tb(1.0);
        // a second detector on the same input: two detectors kept each other alive (B-050)
        Detector uut2("uut2");
        spx::ea_signal_type readout2{"readout2"};
        uut2.p_in(tb.in);
        uut2.p_readout(readout2);
        sc_start(SC_ZERO_TIME);

        tb.in.write(OpticalSignal(std::sqrt(1e-3), 1550e-9));
        sc_start(1, SC_NS);
        // constant readout, exact (sum of |E|^2, no ulp jitter), one change, nothing scheduled
        EXPECT_EQ(tb.readout.read(), std::norm(std::sqrt(1e-3)));
        EXPECT_EQ(readout2.read(), tb.readout.read());
        EXPECT_EQ(tb.rec.m_changes.size(), 1u);
        EXPECT_FALSE(sc_pending_activity());

        // A zero field at another wavelength does not make a beat
        tb.in.write(OpticalSignal(0.0, 1551e-9));
        sc_start(1, SC_NS);
        EXPECT_EQ(tb.readout.read(), std::norm(std::sqrt(1e-3)));
        EXPECT_EQ(tb.rec.m_changes.size(), 1u);
        EXPECT_FALSE(sc_pending_activity());

        tb.in.write(OpticalSignal(std::sqrt(2e-3), 1550e-9));
        sc_start(1, SC_NS);
        EXPECT_EQ(tb.readout.read(), std::norm(std::sqrt(2e-3)));
        ASSERT_EQ(tb.rec.m_changes.size(), 2u);
        EXPECT_EQ(tb.rec.m_changes[1].first, sc_time(2, SC_NS));
        EXPECT_FALSE(sc_pending_activity());

        // An open-ended run returns: the circuit is quiet
        ASSERT_FALSE(sc_pending_activity());
        sc_start();
        EXPECT_EQ(sc_time_stamp(), sc_time(3, SC_NS));
    }
    SPECS_TEST_FOOTER();
}

TEST(Detector, TwoWavelengthsAreSampledWhileBeating)
{
    SPECS_TEST_HEADER()
    {
        init_config();
        const double R = 1.0;
        DetectorTest tb(R, 1e-12);
        sc_start(SC_ZERO_TIME);

        const double wl1 = 1550e-9;
        const double wl2 = 1551e-9; // beat period about 8 ps
        const OpticalSignal::field_type E1 = std::sqrt(1e-3);
        const OpticalSignal::field_type E2 = std::sqrt(0.25e-3);
        tb.in.write(OpticalSignal(E1, wl1));
        sc_start(10, SC_PS);
        EXPECT_EQ(tb.rec.m_changes.size(), 1u);
        EXPECT_FALSE(sc_pending_activity());

        // Second wavelength at 10 ps: written at the input event, then sampled every Ts = 1 ps
        tb.in.write(OpticalSignal(E2, wl2));
        sc_start(sc_time(10.5, SC_PS));
        EXPECT_TRUE(sc_pending_activity());
        ASSERT_EQ(tb.rec.m_changes.size(), 12u); // 0 ps, 10 ps (event), 11 ... 20 ps (samples)
        double min_readout = 1, max_readout = 0;
        for (size_t k = 1; k < tb.rec.m_changes.size(); ++k)
        {
            const auto &change = tb.rec.m_changes[k];
            EXPECT_EQ(change.first, sc_time(10.0 + (k - 1), SC_PS));
            const double expected = beat_readout(R, E1, wl1, E2, wl2, change.first.to_seconds());
            EXPECT_NEAR(change.second, expected, 1e-9 * expected) << "at " << change.first;
            min_readout = std::min(min_readout, change.second);
            max_readout = std::max(max_readout, change.second);
        }
        // within (|E1| - |E2|)^2 R = 0.25 mA and (|E1| + |E2|)^2 R = 2.25 mA
        EXPECT_GE(min_readout, 0.25e-3 * (1 - 1e-9));
        EXPECT_LE(max_readout, 2.25e-3 * (1 + 1e-9));
        EXPECT_EQ(tb.uut.m_cur_readout, tb.readout.read());
        EXPECT_DOUBLE_EQ(tb.uut.m_cur_readout_no_interf, 1.25e-3); // sum of |E_k|^2, no beat

        // Beat ends (second field back to 0): written at the event, no more samples
        tb.in.write(OpticalSignal(0.0, wl2));
        sc_start(10, SC_PS);
        const size_t n_changes = tb.rec.m_changes.size();
        EXPECT_EQ(tb.rec.m_changes.back().first, sc_time(20.5, SC_PS));
        EXPECT_DOUBLE_EQ(tb.readout.read(), 1e-3);
        EXPECT_FALSE(sc_pending_activity());
        sc_start(10, SC_PS);
        EXPECT_EQ(tb.rec.m_changes.size(), n_changes);
    }
    SPECS_TEST_FOOTER();
}

TEST(Detector, PeriodicSamplesAreWrittenBeyondTolerance)
{
    SPECS_TEST_HEADER()
    {
        init_config();
        const double R = 1.0;
        const double Ts = 1e-12;
        DetectorTest tb(R, Ts);
        sc_start(SC_ZERO_TIME);

        // Slow beat (1 MHz): a 1 ps sample changes the readout by < 6.3e-9 A, below the tolerance
        // max(abstol = 1e-8, reltol = 1e-4 * |readout| >= 2.5e-8), so most samples are not written.
        // E2 in quadrature so that the readout moves from the start (sin, not cos).
        const double wl1 = 1550e-9;
        const double wl2 = 299792458 / (freq_Hz(wl1) - 1e6);
        const OpticalSignal::field_type E1 = std::sqrt(1e-3);
        const OpticalSignal::field_type E2 = std::polar(std::sqrt(0.25e-3), pi / 2);
        tb.in.write(OpticalSignal(E1, wl1));
        sc_start(10, SC_PS);
        tb.in.write(OpticalSignal(E2, wl2));
        const sc_time t_event(10, SC_PS);
        const unsigned int n_samples = 1000;
        sc_start(sc_time(Ts * n_samples, SC_SEC) + sc_time(0.5, SC_PS));

        const double abstol = specsGlobalConfig.default_abstol;
        const double reltol = specsGlobalConfig.default_reltol;
        const auto &changes = tb.rec.m_changes;
        ASSERT_GE(changes.size(), 2u);
        EXPECT_EQ(changes[1].first, t_event); // the input event is always written

        // Only a fraction of the samples is written, each one a change beyond the tolerance
        const size_t n_writes = changes.size() - 2;
        EXPECT_GT(n_writes, 10u);
        EXPECT_LT(n_writes, n_samples / 10);
        for (size_t k = 2; k < changes.size(); ++k)
        {
            const double last = changes[k - 1].second;
            EXPECT_GT(std::abs(changes[k].second - last), std::max(abstol, reltol * std::abs(last)));
        }

        // At every sample time, the held readout is within the tolerance of the exact one
        size_t i_change = 1;
        for (unsigned int k = 0; k <= n_samples; ++k)
        {
            const sc_time t = t_event + sc_time(Ts * k, SC_SEC);
            while (i_change + 1 < changes.size() && changes[i_change + 1].first <= t)
                i_change++;
            const double held = changes[i_change].second;
            const double exact = beat_readout(R, E1, wl1, E2, wl2, t.to_seconds());
            EXPECT_LE(std::abs(held - exact), std::max(abstol, reltol * std::abs(held)) + 1e-12)
                << "at " << t;
        }
    }
    SPECS_TEST_FOOTER();
}

TEST(Detector, OpAnalysisKeepsDetectorDisabled)
{
    SPECS_TEST_HEADER()
    {
        init_config();
        specsGlobalConfig.analysis_type = SPECSConfig::OP;
        DetectorTest tb(1.0);
        // a CW source drives the input during the OP (runOPAnalysis() enables it), as in a netlist
        CWSource cws("cws", OpticalSignal(std::sqrt(1e-3), 1550e-9));
        cws.p_out(tb.in);
        specsGlobalConfig.prepareSimulation();

        // runOPAnalysis() disables the (default-enabled) detectors before its first sc_start()
        specsGlobalConfig.runOPAnalysis();
        EXPECT_EQ(tb.in.read().m_field, OpticalSignal::field_type(std::sqrt(1e-3)));
        EXPECT_EQ(tb.uut.enable.read(), SC_LOGIC_0);
        EXPECT_EQ(tb.readout.read(), 0.0);
        EXPECT_TRUE(tb.rec.m_changes.empty());

        // as in runTRANAnalysis(): enabling after the OP writes the stored readout
        tb.uut.enable = sc_logic(1);
        sc_start(1, SC_NS);
        EXPECT_DOUBLE_EQ(tb.readout.read(), 1e-3);
        EXPECT_EQ(tb.rec.m_changes.size(), 1u);
    }
    SPECS_TEST_FOOTER();
}
