// Event-driven photodetector (B-049, B-050): the readout is written to p_readout on input events and
// at an enable; periodic Ts samples only while >= 2 non-zero wavelengths beat, gated by the tolerance.
// Noise is bypassed and the dark current is 0 (Detector defaults), so the readouts are deterministic.
// Noise and dark field (F-01, B-055, B-056): the random draws come from the stream
// spx::rng_for(specsGlobalConfig.seed, name()), so they are reproducible and do not depend on the
// other devices; the tests at the end check this against spx::Rng and across runs (run_in_child()).
#include <gtest/gtest.h>
#include <systemc.h>

#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "devices/cw_source.h"
#include "devices/detector.h"
#include "optical_output_port.h"
#include "specs.h"
#include "utils/rng.h"
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

namespace {
// SystemC elaborates once per process: run the scenario in a forked child and return the values it
// produced, bit-exact (through a pipe), so that one test can compare several simulations
std::vector<double> run_in_child(const std::function<std::vector<double>()> &scenario)
{
    int fds[2];
    if (pipe(fds) != 0)
    {
        ADD_FAILURE() << "pipe() failed";
        return {};
    }
    const pid_t pid = fork();
    if (pid == -1)
    {
        ADD_FAILURE() << "fork() failed";
        return {};
    }
    if (pid == 0)
    {
        close(fds[0]);
        const std::vector<double> values = scenario();
        const char *p = reinterpret_cast<const char *>(values.data());
        size_t left = values.size() * sizeof(double);
        while (left > 0)
        {
            const ssize_t n = write(fds[1], p, left);
            if (n <= 0)
                _exit(3);
            p += n;
            left -= n;
        }
        close(fds[1]);
        std::cout << std::flush;
        _exit(testing::Test::HasFailure() ? 1 : 0);
    }
    close(fds[1]);
    std::string bytes;
    char buf[4096];
    ssize_t n_read;
    while ((n_read = read(fds[0], buf, sizeof(buf))) > 0)
        bytes.append(buf, n_read);
    close(fds[0]);
    int status = 0;
    EXPECT_EQ(waitpid(pid, &status, 0), pid);
    EXPECT_TRUE(WIFEXITED(status) && WEXITSTATUS(status) == 0) << "scenario failed, status " << status;
    std::vector<double> values(bytes.size() / sizeof(double));
    std::memcpy(values.data(), bytes.data(), values.size() * sizeof(double));
    return values;
}

// Standard deviation of Detector::noise_gen() for the noiseless photocurrent I_A, with the Detector
// defaults (fop = 1 GHz, 300 K, 400 Ohm, iTIA = 10 pA/sqrt(Hz)) and no dark current
double noise_sd_A(double I_A)
{
    const double q = 1.60217e-19;
    const double K = 1.38064e-23;
    const double f_Hz = 1e9;
    return std::sqrt(f_Hz * 10e-12 * 10e-12 + f_Hz * 2 * q * I_A + f_Hz * 4 * K * 300 / 400);
}

const double noisy_R_A_W = 0.8;
const unsigned int noisy_n_events = 3;

// Field of the k-th input event of noisy_pair_readouts(), k = 1 .. noisy_n_events
OpticalSignal::field_type noisy_field(unsigned int k)
{
    return std::sqrt(k * 1e-3);
}

// Two detectors with noise (noiseBypass = false), "pdetA" on in_a and "pdetB" on in_b, created in
// the given order, both lit by noisy_n_events input events. Returns pdetA's then pdetB's
// (readout, readout_no_interf) after each event.
std::vector<double> noisy_pair_readouts(uint64_t seed, bool a_first)
{
    init_config();
    specsGlobalConfig.seed = seed;
    std::unique_ptr<Detector> first = std::make_unique<Detector>(a_first ? "pdetA" : "pdetB", noisy_R_A_W, 0, false);
    std::unique_ptr<Detector> second = std::make_unique<Detector>(a_first ? "pdetB" : "pdetA", noisy_R_A_W, 0, false);
    Detector &a = a_first ? *first : *second;
    Detector &b = a_first ? *second : *first;
    spx::oa_signal_type in_a{"in_a"};
    spx::oa_signal_type in_b{"in_b"};
    spx::ea_signal_type out_a{"out_a"};
    spx::ea_signal_type out_b{"out_b"};
    a.p_in(in_a);
    a.p_readout(out_a);
    b.p_in(in_b);
    b.p_readout(out_b);
    sc_start(SC_ZERO_TIME);

    std::vector<double> readouts_a, readouts_b;
    for (unsigned int k = 1; k <= noisy_n_events; ++k)
    {
        in_a.write(OpticalSignal(noisy_field(k), 1550e-9));
        in_b.write(OpticalSignal(noisy_field(k), 1550e-9));
        sc_start(1, SC_NS);
        EXPECT_EQ(out_a.read(), a.m_cur_readout);
        readouts_a.push_back(a.m_cur_readout);
        readouts_a.push_back(a.m_cur_readout_no_interf);
        readouts_b.push_back(b.m_cur_readout);
        readouts_b.push_back(b.m_cur_readout_no_interf);
    }
    readouts_a.insert(readouts_a.end(), readouts_b.begin(), readouts_b.end());
    return readouts_a;
}

// Runs all the delta cycles of the current time
void settle()
{
    do
        sc_start(SC_ZERO_TIME);
    while (sc_pending_activity_at_current_time());
}

// One detector ("uut", R = 1 A/W) that computes its readout with a total field of exactly 0, twice:
// - enabled with an all-zero input memory (at most one wavelength: sum of |E_k|^2 = 0);
// - two wavelengths with opposite fields at t = 0 (both non-zero, they beat, and
//   E1 e^0 + E2 e^0 = 0 exactly).
// Returns (readout, readout_no_interf) after each.
std::vector<double> dark_readouts(uint64_t seed, bool dark_field_injection, bool noise_bypass = true,
                                  double dark_current_A = 0)
{
    init_config();
    specsGlobalConfig.seed = seed;
    Detector uut("uut", 1.0, dark_current_A, noise_bypass);
    uut.m_dark_field_injection = dark_field_injection;
    spx::oa_signal_type in{"in"};
    spx::ea_signal_type readout{"readout"};
    uut.p_in(in);
    uut.p_readout(readout);
    std::vector<double> values;

    // (1) enabled in the dark
    uut.enable = sc_logic(0);
    settle();
    uut.enable = sc_logic(1);
    settle();
    EXPECT_EQ(sc_time_stamp(), SC_ZERO_TIME);
    values.push_back(uut.m_cur_readout);
    values.push_back(uut.m_cur_readout_no_interf);

    // (2) beating fields that cancel at t = 0
    const OpticalSignal::field_type E = std::sqrt(1e-3);
    in.write(OpticalSignal(E, 1550e-9));
    settle();
    in.write(OpticalSignal(-E, 1551e-9));
    settle();
    EXPECT_EQ(sc_time_stamp(), SC_ZERO_TIME);
    EXPECT_TRUE(uut.is_beating());
    values.push_back(uut.m_cur_readout);
    values.push_back(uut.m_cur_readout_no_interf);
    EXPECT_EQ(readout.read(), uut.m_cur_readout);
    return values;
}
}

TEST(DetectorNoise, NoiseIsDrawnFromTheStreamOfTheDetectorName)
{
    SPECS_TEST_HEADER()
    {
        const uint64_t seed = 12345;
        const std::vector<double> readouts = noisy_pair_readouts(seed, false);
        ASSERT_EQ(readouts.size(), 4 * noisy_n_events);

        // Per computed readout: one normal value for readout, one for readout_no_interf (the input
        // is never dark: no dark-field draw)
        const char *names[] = {"pdetA", "pdetB"};
        for (unsigned int d = 0; d < 2; ++d)
        {
            spx::Rng rng = spx::rng_for(seed, names[d]);
            for (unsigned int k = 1; k <= noisy_n_events; ++k)
            {
                const double I_A = std::norm(noisy_field(k)) * noisy_R_A_W;
                const double expected = I_A + noise_sd_A(I_A) * rng.normal();
                const double expected_no_interf = I_A + noise_sd_A(I_A) * rng.normal();
                const size_t i = d * 2 * noisy_n_events + 2 * (k - 1);
                // noise sd about 0.7 uA: 1e-15 A is far below one draw's contribution
                EXPECT_NEAR(readouts[i], expected, 1e-15) << names[d] << " event " << k;
                EXPECT_NEAR(readouts[i + 1], expected_no_interf, 1e-15) << names[d] << " event " << k;
                EXPECT_NE(readouts[i], I_A);
            }
        }
    }
    SPECS_TEST_FOOTER();
}

TEST(DetectorNoise, StreamsDependOnTheNameNotOnTheCreationOrder)
{
    SPECS_TEST_HEADER()
    {
        const std::vector<double> ab = run_in_child([] { return noisy_pair_readouts(0, true); });
        const std::vector<double> ba = run_in_child([] { return noisy_pair_readouts(0, false); });
        ASSERT_EQ(ab.size(), 4 * noisy_n_events);
        EXPECT_EQ(ab, ba);

        // pdetA and pdetB see the same inputs but have independent streams
        const size_t half = 2 * noisy_n_events;
        for (size_t i = 0; i < half; ++i)
            EXPECT_NE(ab[i], ab[half + i]) << i;
    }
    SPECS_TEST_FOOTER();
}

TEST(DetectorNoise, SameSeedGivesTheSameReadoutsAnotherSeedOtherOnes)
{
    SPECS_TEST_HEADER()
    {
        const std::vector<double> seed7 = run_in_child([] { return noisy_pair_readouts(7, true); });
        const std::vector<double> seed7_again = run_in_child([] { return noisy_pair_readouts(7, true); });
        const std::vector<double> seed8 = run_in_child([] { return noisy_pair_readouts(8, true); });
        const std::vector<double> seed_default = run_in_child([] {
            return noisy_pair_readouts(SPECSConfig::DEFAULT_SEED, true);
        });
        ASSERT_EQ(seed7.size(), 4 * noisy_n_events);
        ASSERT_EQ(seed8.size(), seed7.size());
        ASSERT_EQ(seed_default.size(), seed7.size());
        EXPECT_EQ(seed7, seed7_again);
        for (size_t i = 0; i < seed7.size(); ++i)
        {
            EXPECT_NE(seed7[i], seed8[i]) << i;
            EXPECT_NE(seed7[i], seed_default[i]) << i;
        }
    }
    SPECS_TEST_FOOTER();
}

TEST(DetectorNoise, DarkFieldInjectionOffReadsExactlyTheDarkCurrent)
{
    SPECS_TEST_HEADER()
    {
        // noise bypassed: exactly 0, in both branches of compute_readout()
        const std::vector<double> off = run_in_child([] { return dark_readouts(0, false); });
        ASSERT_EQ(off.size(), 4u);
        EXPECT_EQ(off[0], 0.0);
        EXPECT_EQ(off[1], 0.0);
        EXPECT_EQ(off[2], 0.0);
        EXPECT_DOUBLE_EQ(off[3], 2e-3); // readout_no_interf: sum of |E_k|^2, the fields are not 0

        // with a dark current: exactly the dark current
        const std::vector<double> dark = run_in_child([] { return dark_readouts(0, false, true, 100e-12); });
        ASSERT_EQ(dark.size(), 4u);
        EXPECT_EQ(dark[0], 100e-12);
        EXPECT_EQ(dark[1], 100e-12);
        EXPECT_EQ(dark[2], 100e-12);

        // with noise: only the noise (the first draw of the stream: no dark-field draw before it)
        const std::vector<double> noisy = run_in_child([] { return dark_readouts(0, false, false); });
        ASSERT_EQ(noisy.size(), 4u);
        spx::Rng rng = spx::rng_for(0, "uut");
        EXPECT_NEAR(noisy[0], noise_sd_A(0) * rng.normal(), 1e-18);
        EXPECT_NEAR(noisy[1], noise_sd_A(0) * rng.normal(), 1e-18);
    }
    SPECS_TEST_FOOTER();
}

TEST(DetectorNoise, DarkFieldInjectionOnIsReproducible)
{
    SPECS_TEST_HEADER()
    {
        const std::vector<double> on = run_in_child([] { return dark_readouts(0, true); });
        const std::vector<double> on_again = run_in_child([] { return dark_readouts(0, true); });
        const std::vector<double> on_seed1 = run_in_child([] { return dark_readouts(1, true); });
        ASSERT_EQ(on.size(), 4u);
        EXPECT_EQ(on, on_again);
        EXPECT_NE(on[0], on_seed1[0]);
        EXPECT_NE(on[2], on_seed1[2]);

        // (1) enabled in the dark: R |1e-20 n|^2, n = first normal value of the stream of "uut"
        spx::Rng rng = spx::rng_for(0, "uut");
        const double n0 = rng.normal();
        EXPECT_DOUBLE_EQ(on[0], std::norm(OpticalSignal::field_type(1e-20 * n0)));
        EXPECT_GT(on[0], 0.0);
        EXPECT_LT(on[0], 1e-38);
        EXPECT_EQ(on[1], 0.0); // readout_no_interf: sum of |E_k|^2 = 0, no injection
        // (2) the beat cancels at t = 0: the 3rd computed readout, after 1 + 2 + 2 draws (dark field
        // and two noise values at the enable, two noise values at the first input event)
        rng.normal();
        rng.normal();
        rng.normal();
        rng.normal();
        const double n5 = rng.normal();
        EXPECT_DOUBLE_EQ(on[2], std::norm(OpticalSignal::field_type(1e-20 * n5)));
        EXPECT_GT(on[2], 0.0);
        EXPECT_LT(on[2], 1e-38);
        EXPECT_DOUBLE_EQ(on[3], 2e-3);
    }
    SPECS_TEST_FOOTER();
}
