#include <gtest/gtest.h>
#include <systemc.h>
#include <cmath>
#include <limits>
#include <set>
#include <vector>

#include "optical_output_port.h"
#include "specs.h"

#include "test/utils.h"

// Owns an OpticalOutputPort bound to a signal, so that the process
// OpticalOutputPort::on_data_ready() can be driven directly through its
// (public) queue and event queue. Used for the error paths of B-020.
class OOPHarness : public sc_module {
public:
    spx::oa_port_out_type p_out;
    OpticalOutputPort m_out_writer;
    spx::oa_signal_type out{"out"};

    OOPHarness(sc_module_name name)
        : sc_module(name)
        , m_out_writer("out_writer", p_out)
    {
        p_out(out);
        auto config = make_shared<OpticalOutputPortConfig>();
        m_out_writer.setConfig(config);
    }

    void enqueue(const sc_time &t, const OpticalSignal &s)
    {
        m_out_writer.m_queue.push(std::make_pair(t, s));
    }
};

// A notification without a queued value is an internal error: the port must
// stop the simulation without reading the empty queue (was: UB, SIGSEGV).
TEST(OpticalOutputPort, EmptyQueueStopsWithoutEmitting)
{
    SPECS_TEST_HEADER()
    {
        OOPHarness h("h");
        sc_start(SC_ZERO_TIME);

        h.m_out_writer.notify_pending(sc_time(1, SC_NS));
        sc_start(10, SC_NS);

        EXPECT_EQ(sc_get_status(), SC_STOPPED);
        EXPECT_EQ(h.out.read().m_field, OpticalSignal::field_type(0, 0));
    }
    SPECS_TEST_FOOTER();
}

// A value woken up at the wrong time is an internal error: the port must stop
// the simulation without emitting that value (was: emitted after sc_stop()).
TEST(OpticalOutputPort, DesyncStopsWithoutEmitting)
{
    SPECS_TEST_HEADER()
    {
        OOPHarness h("h");
        sc_start(SC_ZERO_TIME);

        h.enqueue(sc_time(5, SC_NS), OpticalSignal(OpticalSignal::field_type(1, 0), 1.55e-6));
        h.m_out_writer.notify_pending(sc_time(1, SC_NS));
        sc_start(10, SC_NS);

        EXPECT_EQ(sc_get_status(), SC_STOPPED);
        EXPECT_EQ(h.out.read().m_field, OpticalSignal::field_type(0, 0));
    }
    SPECS_TEST_FOOTER();
}

// A value queued before the port thread first runs must not be dropped
// (was: the thread cleared its queue on start, then hit the empty-queue error).
TEST(OpticalOutputPort, KeepsValueQueuedBeforeThreadStart)
{
    SPECS_TEST_HEADER()
    {
        OOPHarness h("h");

        h.enqueue(sc_time(1, SC_NS), OpticalSignal(OpticalSignal::field_type(1, 0), 1.55e-6));
        h.m_out_writer.notify_pending(sc_time(1, SC_NS));
        sc_start(10, SC_NS);

        EXPECT_NE(sc_get_status(), SC_STOPPED);
        EXPECT_EQ(h.out.read().m_field, OpticalSignal::field_type(1, 0));
    }
    SPECS_TEST_FOOTER();
}

// A frequency-domain write must merge with the queued value of the same
// (time, wavelength) even when another wavelength is on top of the queue
// (was: a second entry was pushed and the stale one could pop last, B-001).
TEST(OpticalOutputPort, FrequencyDomainWriteMergesBehindQueueTop)
{
    SPECS_TEST_HEADER()
    {
        OOPHarness h("h");
        auto &oop = h.m_out_writer;

        const OpticalSignal a_old(OpticalSignal::field_type(0.1, 0), 1.55e-6);
        const OpticalSignal b(OpticalSignal::field_type(2, 0), 1.56e-6);
        const OpticalSignal a_new(OpticalSignal::field_type(0.5, 0), 1.55e-6);

        oop.immediateWriteFrequencyDomain(a_old);
        oop.immediateWriteFrequencyDomain(b);
        // equal-time entries pop larger |E| first: b is now on top
        ASSERT_EQ(oop.m_queue.top().second.m_wavelength_id, b.m_wavelength_id);
        oop.immediateWriteFrequencyDomain(a_new);

        ASSERT_EQ(oop.m_queue.size(), 2u);
        int n_a = 0;
        for (auto it = oop.m_queue.begin(); it != oop.m_queue.end(); ++it)
        {
            if (it->second.m_wavelength_id != a_new.m_wavelength_id)
                continue;
            ++n_a;
            EXPECT_EQ(it->second.m_field, a_new.m_field);
        }
        EXPECT_EQ(n_a, 1);
    }
    SPECS_TEST_FOOTER();
}

// Sampled-time mode is not implemented (delayedWriteSampledTime() drops every
// write): a port configured with it must be rejected when the configuration is
// applied, with a clean error and exit 1, not per event (B-002).
TEST(OpticalOutputPortDeathTest, ApplyConfigRejectsSampledTime)
{
    SPECS_TEST_HEADER()
    {
        OOPHarness h("h");
        auto config = make_shared<OpticalOutputPortConfig>();
        config->m_mode = OpticalOutputPortMode::SAMPLED_TIME;
        h.m_out_writer.setConfig(config);

        EXPECT_EXIT(h.m_out_writer.applyConfig(), testing::ExitedWithCode(1), "sampled-time mode is not implemented");
    }
    SPECS_TEST_FOOTER();
}

// Emission gate (B-021): with nothing emitted yet (|last| = 0) the reltol test
// passes for any change and fails for none, as the former division did
// (x/0 = inf, 0/0 = NaN). Both tests must pass for a value to be emitted.
TEST(OpticalOutputPort, EmissionGateWithZeroLast)
{
    SPECS_TEST_HEADER()
    {
        OOPHarness h("h");
        auto &oop = h.m_out_writer;
        oop.applyConfig(); // default config: abstol 1e-8, reltol 1e-4
        typedef OpticalSignal::field_type F;

        // first emission (last = 0)
        EXPECT_TRUE(oop.check_emit_by_reltol(F(1e-3, 0), F(0, 0)));
        EXPECT_TRUE(oop.check_emit_by_reltol(F(0, 1e-300), F(0, 0)));
        EXPECT_TRUE(oop.check_emit_by_abstol(F(1e-3, 0), F(0, 0)));
        // zero to zero
        EXPECT_FALSE(oop.check_emit_by_reltol(F(0, 0), F(0, 0)));
        EXPECT_FALSE(oop.check_emit_by_abstol(F(0, 0), F(0, 0)));
        // below abstol, from 0: reltol passes, abstol does not
        EXPECT_FALSE(oop.check_emit_by_abstol(F(5e-9, 0), F(0, 0)));
        // |last| > 0: relative change against reltol = 1e-4
        EXPECT_FALSE(oop.check_emit_by_reltol(F(1 + 5e-5, 0), F(1, 0)));
        EXPECT_TRUE(oop.check_emit_by_reltol(F(1 + 2e-4, 0), F(1, 0)));
        EXPECT_TRUE(oop.check_emit_by_reltol(F(0, 0), F(1, 0)));
    }
    SPECS_TEST_FOOTER();
}

// Zero tolerances (B-153): with abstol = reltol = 0 the gate is off: any change is emitted
// (also one whose relative size underflows to 0) and an identical value is not. The snap
// (|E| < snap_factor * abstol) never applies.
TEST(OpticalOutputPort, EmissionGateWithZeroTolerances)
{
    SPECS_TEST_HEADER()
    {
        OOPHarness h("h");
        auto &oop = h.m_out_writer;
        typedef OpticalSignal::field_type F;
        auto config = make_shared<OpticalOutputPortConfig>();
        config->m_abstol = 0;
        config->m_reltol = 0; // default snap factor (10)
        oop.setConfig(config);
        oop.applyConfig();

        const double ulp1 = 2.220446049250313e-16; // 1 + ulp1 is the double after 1
        EXPECT_TRUE(oop.check_emit_by_abstol(F(1 + ulp1, 0), F(1, 0)));
        EXPECT_TRUE(oop.check_emit_by_reltol(F(1 + ulp1, 0), F(1, 0)));
        EXPECT_TRUE(oop.check_emit_by_abstol(F(0, 5e-324), F(0, 0)));
        EXPECT_TRUE(oop.check_emit_by_reltol(F(0, 5e-324), F(0, 0)));
        EXPECT_TRUE(oop.check_emit_by_reltol(F(0, 0), F(1, 0)));
        // |dE| / |last| = 1e-320 / 1e10 underflows to 0: still a change
        EXPECT_TRUE(oop.check_emit_by_abstol(F(1e10, 1e-320), F(1e10, 0)));
        EXPECT_TRUE(oop.check_emit_by_reltol(F(1e10, 1e-320), F(1e10, 0)));
        // identical values
        EXPECT_FALSE(oop.check_emit_by_abstol(F(0.5, -0.25), F(0.5, -0.25)));
        EXPECT_FALSE(oop.check_emit_by_reltol(F(0.5, -0.25), F(0.5, -0.25)));
        EXPECT_FALSE(oop.check_emit_by_abstol(F(0, 0), F(0, 0)));
        EXPECT_FALSE(oop.check_emit_by_reltol(F(0, 0), F(0, 0)));

        // Through the port
        auto write = [&](F field) {
            oop.delayedWrite(OpticalSignal(field, 1.55e-6), sc_time(1, SC_NS));
            sc_start(2, SC_NS);
            return h.out.read().m_field;
        };
        sc_start(SC_ZERO_TIME); // applies the config again (start_of_simulation)
        EXPECT_EQ(oop.m_abstol, 0);
        EXPECT_EQ(oop.m_reltol, 0);
        EXPECT_EQ(write(F(1e-9, 0)), F(1e-9, 0)) << "below the default abstol and snap (1e-7): emitted as is";
        EXPECT_EQ(write(F(1e-9, 1e-25)), F(1e-9, 1e-25)) << "relative change 1e-16: emitted";
        EXPECT_EQ(write(F(5e-324, 0)), F(5e-324, 0)) << "smallest subnormal: emitted, not snapped";
        EXPECT_EQ(oop.lastEmissionThreshold(), 0);
        const sc_dt::uint64 last_emission = oop.m_last_emission_delta;
        EXPECT_EQ(write(F(5e-324, 0)), F(5e-324, 0));
        EXPECT_EQ(oop.m_last_emission_delta, last_emission) << "identical value: not emitted";
        EXPECT_EQ(write(F(0, 0)), F(0, 0)) << "back to 0: emitted";
        EXPECT_GT(oop.m_last_emission_delta, last_emission);
    }
    SPECS_TEST_FOOTER();
}

// Snap to 0 (B-021): an emitted value with |E| < snap_factor * abstol is replaced
// by 0; the factor comes from the port configuration (default 10, 0 disables it).
TEST(OpticalOutputPort, SnapFactorFromConfig)
{
    SPECS_TEST_HEADER()
    {
        OOPHarness h("h");
        auto &oop = h.m_out_writer;
        typedef OpticalSignal::field_type F;
        auto write = [&](double field) {
            oop.delayedWrite(OpticalSignal(F(field, 0), 1.55e-6), sc_time(1, SC_NS));
            sc_start(2, SC_NS);
            return h.out.read().m_field;
        };
        auto set_snap_factor = [&](double snap_factor) {
            auto config = make_shared<OpticalOutputPortConfig>();
            config->m_snap_factor = snap_factor;
            oop.setConfig(config);
            oop.applyConfig();
        };

        sc_start(SC_ZERO_TIME); // applies the default config (abstol 1e-8, snap factor 10)
        EXPECT_EQ(oop.m_snap_factor, 10);
        EXPECT_EQ(write(5e-8), F(0, 0)) << "5e-8 < 10 * abstol: emitted as 0";
        EXPECT_EQ(write(2e-7), F(2e-7, 0)) << "2e-7 >= 10 * abstol: emitted as is";

        set_snap_factor(0);
        EXPECT_EQ(write(5e-8), F(5e-8, 0)) << "snap factor 0: no snap";
        EXPECT_EQ(write(2e-8), F(2e-8, 0));

        set_snap_factor(4);
        EXPECT_EQ(write(3.5e-8), F(0, 0)) << "3.5e-8 < 4 * abstol: emitted as 0";
        EXPECT_EQ(write(5e-8), F(5e-8, 0)) << "5e-8 >= 4 * abstol: emitted as is";
    }
    SPECS_TEST_FOOTER();
}

// B-014: classification of the rounding of a delay to the time step
TEST(DelayRounding, Classify)
{
    SPECS_TEST_HEADER()
    {
        const double thr = OpticalOutputPort::DELAY_ROUNDING_WARN_THRESHOLD;
        EXPECT_DOUBLE_EQ(thr, 0.1);

        // non-zero delay rounded to 0 time steps (backlog/repro/a1-zero-tick-delay-ring.cir: 13 fs at 100 fs)
        EXPECT_EQ(classifyDelayRounding(13.34e-15, 100e-15, thr), DELAY_ROUNDING_ERROR);
        EXPECT_EQ(classifyDelayRounding(49e-15, 100e-15, thr), DELAY_ROUNDING_ERROR);
        // rounded by more than the threshold: 53 fs -> 100 fs (+87 %), 2.27 ps -> 2 ps (-11.7 %)
        EXPECT_EQ(classifyDelayRounding(53.37e-15, 100e-15, thr), DELAY_ROUNDING_WARN);
        EXPECT_EQ(classifyDelayRounding(2.2653e-12, 1e-12, thr), DELAY_ROUNDING_WARN);
        EXPECT_EQ(classifyDelayRounding(88.46e-15, 100e-15, thr), DELAY_ROUNDING_WARN);
        // benign: the default 100 um waveguide at 100 fs (755 fs -> 800 fs, +5.95 %), exact, zero
        EXPECT_EQ(classifyDelayRounding(755.09e-15, 100e-15, thr), DELAY_ROUNDING_OK);
        EXPECT_EQ(classifyDelayRounding(2.5851e-12, 100e-15, thr), DELAY_ROUNDING_OK);
        EXPECT_EQ(classifyDelayRounding(300e-15, 100e-15, thr), DELAY_ROUNDING_OK);
        EXPECT_EQ(classifyDelayRounding(0, 100e-15, thr), DELAY_ROUNDING_OK);
        // the threshold is a parameter: 755 fs -> 800 fs is +5.95 %
        EXPECT_EQ(classifyDelayRounding(755.09e-15, 100e-15, 0.05), DELAY_ROUNDING_WARN);
        EXPECT_EQ(classifyDelayRounding(755.09e-15, 100e-15, 0.06), DELAY_ROUNDING_OK);
        // a delay of 5 time steps or more is never rounded by more than 10 %
        for (double ticks = 5; ticks < 50; ticks += 0.01)
            EXPECT_EQ(classifyDelayRounding(ticks * 100e-15, 100e-15, thr), DELAY_ROUNDING_OK) << ticks;

        EXPECT_DOUBLE_EQ(roundDelay(53.37e-15, 100e-15), 100e-15);
        EXPECT_DOUBLE_EQ(roundDelay(2.2653e-12, 1e-12), 2e-12);
        EXPECT_EQ(roundDelay(13.34e-15, 100e-15), 0);
    }
    SPECS_TEST_FOOTER();
}

// roundDelay() must round like sc_time(double, unit) (nearest time step, half up)
TEST(DelayRounding, MatchesSystemCRounding)
{
    SPECS_TEST_HEADER()
    {
        sc_set_time_resolution(100, SC_FS);
        const double resolution_s = sc_get_time_resolution().to_seconds();
        for (int k = 0; k < 10000; ++k)
        {
            const double delay_s = k * 0.37e-15;
            const double rounded_s = roundDelay(delay_s, resolution_s);
            EXPECT_EQ(std::llround(rounded_s / resolution_s), (long long)sc_time(delay_s, SC_SEC).value()) << delay_s;
            EXPECT_EQ(std::llround(rounded_s / resolution_s), (long long)sc_time(delay_s * 1e9, SC_NS).value()) << delay_s;
        }
    }
    SPECS_TEST_FOOTER();
}

// checkDelayRounding(): a message only where delays are honoured (event-driven
// ports in a time-domain analysis), and only above the threshold. A delay that
// rounds to 0 is an error for netlist waveguides (zero_is_error), a warning
// for device-internal ones
TEST(DelayRoundingDeathTest, CheckDelayRounding)
{
    SPECS_TEST_HEADER()
    {
        specsGlobalConfig.engine_timescale = SPECSConfig::HUNDRED_FS;
        specsGlobalConfig.applyEngineResolution();
        OOPHarness h("h");
        auto &oop = h.m_out_writer;
        auto config = make_shared<OpticalOutputPortConfig>();
        oop.setConfig(config);
        const auto ok = testing::ExitedWithCode(0);
        auto check = [&](double delay_s, bool zero_is_error = false) {
            oop.checkDelayRounding("dev", delay_s, zero_is_error);
            std::cerr << "end";
            exit(0);
        };

        specsGlobalConfig.analysis_type = SPECSConfig::TRAN;
        config->m_mode = OpticalOutputPortMode::EVENT_DRIVEN;
        EXPECT_EXIT(check(53.37e-15), ok,
                    "^Warning \\(dev\\): delay 5.337e-14 s rounded to 1e-13 s \\(\\+87.4 %\\) at the time resolution 1e-13 s; "
                    "use a finer time resolution, e.g. .options timescale=-14\nend$");
        EXPECT_EXIT(check(13.34e-15), ok,
                    "^Warning \\(dev\\): delay 1.334e-14 s rounds to 0 at the time resolution 1e-13 s [^\n]*timescale=-15\nend$");
        EXPECT_EXIT(check(13.34e-15, true), testing::ExitedWithCode(1),
                    "^Error \\(dev\\): delay 1.334e-14 s rounds to 0 at the time resolution 1e-13 s "
                    "\\(the delayed events would happen in the same time step\\); "
                    "use a finer time resolution, e.g. .options timescale=-15\n$");
        // only a rounding to 0 is an error: the other verdicts are the same for both origins
        EXPECT_EXIT(check(53.37e-15, true), ok, "^Warning \\(dev\\): delay 5.337e-14 s rounded to 1e-13 s [^\n]*\nend$");
        EXPECT_EXIT(check(755.09e-15), ok, "^end$");
        EXPECT_EXIT(check(755.09e-15, true), ok, "^end$");
        EXPECT_EXIT(check(0), ok, "^end$");
        EXPECT_EXIT(check(0, true), ok, "^end$");

        // with a resolution multiplier, events are on multiples of 10 ticks = 1 ps
        config->m_timestep_value = 10;
        EXPECT_EXIT(check(755.09e-15), ok,
                    "^Warning \\(dev\\): delay 7.5509e-13 s rounded to 1e-12 s \\(\\+32.4 %\\) at the time resolution 1e-12 s; "
                    "use a finer time resolution, e.g. .options timescale=-14, or a smaller .options resolution \\(10\\)\nend$");
        config->m_timestep_value = 1;

        // delays ignored: frequency-domain ports, .op, .dc
        config->m_mode = OpticalOutputPortMode::FREQUENCY_DOMAIN;
        EXPECT_EXIT(check(13.34e-15, true), ok, "^end$");
        config->m_mode = OpticalOutputPortMode::EVENT_DRIVEN;
        specsGlobalConfig.analysis_type = SPECSConfig::OP;
        EXPECT_EXIT(check(13.34e-15, true), ok, "^end$");
        specsGlobalConfig.analysis_type = SPECSConfig::DC;
        EXPECT_EXIT(check(13.34e-15, true), ok, "^end$");
    }
    SPECS_TEST_FOOTER();
}

// F-15: the gate decides on squared norms (std::norm) when the roundings cannot change the
// result, on the magnitudes (std::abs) otherwise. It must take the same decisions as the
// former gate, below, for every input: zero, NaN, infinities, subnormals, huge values and
// values one ulp around each threshold
namespace {
typedef OpticalSignal::field_type GateField;

bool formerAbstolGate(double abstol, const GateField &desired, const GateField &last)
{
    return abs(desired - last) > abstol;
}

bool formerReltolGate(double reltol, const GateField &desired, const GateField &last)
{
    double vector_distance = abs(desired - last);
    double last_size = abs(last);
    if (last_size == 0)
        return vector_distance > 0;
    if (reltol == 0)
        return vector_distance > 0;
    return (vector_distance/last_size) > reltol;
}

bool formerSnap(double snap_factor, double abstol, const GateField &desired)
{
    return abs(desired) < snap_factor*abstol;
}

// Magnitudes around the thresholds of the default and zero tolerances, and edge values
std::vector<double> gateTestMagnitudes()
{
    const double inf = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    std::vector<double> v = {0.0, -0.0, 5e-324, 1e-320, 2.2250738585072014e-308, 1e-300, 1e-200, 1e-160,
                             1e-150, 1e-20, 1e-12, 1e-4, 0.5, 1, 1.5, 1e100, 1e150, 1e160, 1e300,
                             1.7976931348623157e308, inf, -inf, nan};
    for (double t : {1e-8, 1e-7, 1e-4, 1e-12, 1e-16})
    {
        double x = t;
        for (int i = 0; i < 3; ++i)
            x = std::nextafter(x, 0.0);
        for (int i = 0; i < 7; ++i, x = std::nextafter(x, 1.0))
        {
            v.push_back(x);
            v.push_back(-x);
            v.push_back(x * 1e-4); // reltol: |dE| = 1e-4 * |E_last|
            v.push_back(1 + x);
        }
    }
    return v;
}
} // namespace

TEST(OpticalOutputPort, SquaredNormGateMatchesFormerGate)
{
    SPECS_TEST_HEADER()
    {
        OOPHarness h("h");
        auto &oop = h.m_out_writer;
        const std::vector<double> mags = gateTestMagnitudes();
        // a few complex values from those magnitudes (pure real, pure imaginary, mixed)
        std::vector<GateField> fields;
        for (double m : mags)
        {
            fields.emplace_back(m, 0);
            fields.emplace_back(0, m);
            fields.emplace_back(m * 0.6, m * 0.8);
        }
        // relative changes around reltol = 1e-4 of a few last values
        std::vector<GateField> lasts = {GateField(0, 0), GateField(1, 0), GateField(0.6, -0.8),
                                        GateField(1e-3, 2e-3), GateField(1e-8, 0), GateField(1e-300, 0),
                                        GateField(std::numeric_limits<double>::infinity(), 0),
                                        GateField(std::numeric_limits<double>::quiet_NaN(), 0)};

        size_t checked = 0;
        for (double abstol : {1e-8, 0.0, 1e-12})
        {
            for (double reltol : {1e-4, 0.0, 1e-9})
            {
                for (double snap_factor : {10.0, 0.0, 1.0})
                {
                    auto config = make_shared<OpticalOutputPortConfig>();
                    config->m_abstol = abstol;
                    config->m_reltol = reltol;
                    config->m_snap_factor = snap_factor;
                    oop.setConfig(config);
                    oop.applyConfig();
                    for (const GateField &last : lasts)
                    {
                        auto check = [&](const GateField &desired) {
                            ASSERT_EQ(oop.check_emit_by_abstol(desired, last), formerAbstolGate(abstol, desired, last))
                                << "abstol " << abstol << ", desired " << desired << ", last " << last;
                            ASSERT_EQ(oop.check_emit_by_reltol(desired, last), formerReltolGate(reltol, desired, last))
                                << "reltol " << reltol << ", desired " << desired << ", last " << last;
                            ASSERT_EQ(oop.check_snap_to_zero(desired), formerSnap(snap_factor, abstol, desired))
                                << "snap " << snap_factor << " x abstol " << abstol << ", desired " << desired;
                            ++checked;
                        };
                        for (const GateField &f : fields)
                        {
                            check(f);
                            check(last + f); // changes relative to last
                            check(last * (1 + f.real())); // relative changes along last
                        }
                        // exactly at and one ulp around the reltol threshold, along last
                        for (double r : {reltol, 1e-4, 1e-9})
                        {
                            double x = r;
                            for (int i = 0; i < 3; ++i)
                                x = std::nextafter(x, 0.0);
                            for (int i = 0; i < 7; ++i, x = std::nextafter(x, 1.0))
                                check(last * (1 + x));
                        }
                    }
                }
            }
        }
        EXPECT_GT(checked, 100000u);

        // the edge cases named by F-15, explicitly (default tolerances)
        auto default_config = make_shared<OpticalOutputPortConfig>();
        oop.setConfig(default_config);
        oop.applyConfig();
        const double inf = std::numeric_limits<double>::infinity();
        const double nan = std::numeric_limits<double>::quiet_NaN();
        // |last| = 0
        EXPECT_TRUE(oop.check_emit_by_reltol(GateField(1e-300, 0), GateField(0, 0)));
        EXPECT_TRUE(oop.check_emit_by_reltol(GateField(0, 5e-324), GateField(0, -0.0)));
        EXPECT_FALSE(oop.check_emit_by_abstol(GateField(0, 5e-324), GateField(0, 0)));
        // NaN: never emitted (comparisons with NaN are false)
        EXPECT_FALSE(oop.check_emit_by_abstol(GateField(nan, 0), GateField(1, 0)));
        EXPECT_FALSE(oop.check_emit_by_reltol(GateField(nan, 0), GateField(1, 0)));
        EXPECT_FALSE(oop.check_emit_by_reltol(GateField(1, 0), GateField(nan, 0)));
        EXPECT_FALSE(oop.check_snap_to_zero(GateField(nan, 0)));
        // infinities: hypot(inf, NaN) = inf, so inf - inf = NaN in one part still passes abstol
        EXPECT_TRUE(oop.check_emit_by_abstol(GateField(inf, 0), GateField(1, 0)));
        EXPECT_TRUE(oop.check_emit_by_abstol(GateField(inf, inf), GateField(inf, 0)));
        EXPECT_FALSE(oop.check_emit_by_reltol(GateField(1, 0), GateField(inf, 0)));
        EXPECT_FALSE(oop.check_snap_to_zero(GateField(inf, 0)));
    }
    SPECS_TEST_FOOTER();
}

// F-15: the notifications of the pending entries (were made by an sc_event_queue). Records every
// value the port writes, with time and delta cycle
class PendingHarness : public sc_module {
public:
    struct Write {
        sc_time t;
        sc_dt::uint64 delta;
        OpticalSignal::field_type field;
        uint32_t wavelength_id;
    };

    spx::oa_port_out_type p_out;
    OpticalOutputPort m_out_writer;
    spx::oa_signal_type out{"out"};
    std::vector<Write> seen;

    PendingHarness(sc_module_name name)
        : sc_module(name)
        , m_out_writer("out_writer", p_out)
    {
        p_out(out);
        auto config = make_shared<OpticalOutputPortConfig>();
        m_out_writer.setConfig(config);
        SC_METHOD(on_out_changed);
        sensitive << out;
        dont_initialize();
    }

    void on_out_changed()
    {
        seen.push_back({sc_time_stamp(), sc_delta_count(), out.read().m_field, out.read().m_wavelength_id});
    }

    void write(double field, double wavelength, double delay_ns)
    {
        m_out_writer.delayedWrite(OpticalSignal(OpticalSignal::field_type(field, 0), wavelength),
                                  sc_time(delay_ns, SC_NS));
    }
};

// Equal times: one entry per delta cycle, larger |E| first (the order of the coalescing
// queue); the last one taken is the value left on the net
TEST(OpticalOutputPort, PendingEqualTimesOnePerDeltaCycle)
{
    SPECS_TEST_HEADER()
    {
        PendingHarness h("h");
        sc_start(SC_ZERO_TIME);
        h.write(1, 1.55e-6, 1);
        h.write(3, 1.56e-6, 1);
        h.write(2, 1.57e-6, 1);
        EXPECT_EQ(h.m_out_writer.pending(), 3u);
        sc_start(2, SC_NS);

        ASSERT_EQ(h.seen.size(), 3u);
        const double wl[] = {1.56e-6, 1.57e-6, 1.55e-6};
        for (size_t i = 0; i < 3; ++i)
        {
            EXPECT_EQ(h.seen[i].t, sc_time(1, SC_NS));
            EXPECT_EQ(OpticalSignal::getWavelength(h.seen[i].wavelength_id), wl[i]);
            if (i > 0)
            {
                EXPECT_EQ(h.seen[i].delta, h.seen[i - 1].delta + 1);
            }
        }
        EXPECT_EQ(h.out.read().m_field, OpticalSignal::field_type(1, 0));
        EXPECT_TRUE(h.m_out_writer.isempty());
    }
    SPECS_TEST_FOOTER();
}

// A value for an earlier time than the pending ones moves the notification earlier
TEST(OpticalOutputPort, PendingEarlierWriteIsTakenFirst)
{
    SPECS_TEST_HEADER()
    {
        PendingHarness h("h");
        sc_start(SC_ZERO_TIME);
        h.write(5, 1.55e-6, 5);
        h.write(2, 1.55e-6, 2);
        h.write(3, 1.55e-6, 3);
        sc_start(1, SC_NS);
        h.write(4, 1.55e-6, 3); // at 4 ns
        h.write(1.5, 1.55e-6, 0.5); // at 1.5 ns, before all of them
        sc_start(10, SC_NS);

        ASSERT_EQ(h.seen.size(), 5u);
        const double t_ns[] = {1.5, 2, 3, 4, 5};
        for (size_t i = 0; i < 5; ++i)
        {
            EXPECT_EQ(h.seen[i].t, sc_time(t_ns[i], SC_NS));
            EXPECT_EQ(h.seen[i].field, OpticalSignal::field_type(t_ns[i], 0));
        }
    }
    SPECS_TEST_FOOTER();
}

// A value for a (time, wavelength) already pending replaces it; another time or wavelength is a
// new entry
TEST(OpticalOutputPort, PendingSameTimeAndWavelengthMerges)
{
    SPECS_TEST_HEADER()
    {
        PendingHarness h("h");
        auto &oop = h.m_out_writer;
        sc_start(SC_ZERO_TIME);
        h.write(1, 1.55e-6, 3);
        h.write(2, 1.55e-6, 5);
        h.write(3, 1.56e-6, 3);
        h.write(4, 1.55e-6, 3); // replaces the first one
        EXPECT_EQ(oop.pending(), 3u);
        h.write(5, 1.56e-6, 5); // new
        EXPECT_EQ(oop.pending(), 4u);
        sc_start(10, SC_NS);

        // (the order of equal times is not tested: a merged entry keeps its place in the heap)
        ASSERT_EQ(h.seen.size(), 4u);
        std::set<std::pair<double, double>> at3, at5; // (field, wavelength)
        for (const auto &w : h.seen)
        {
            ASSERT_TRUE(w.t == sc_time(3, SC_NS) || w.t == sc_time(5, SC_NS));
            (w.t == sc_time(3, SC_NS) ? at3 : at5).insert({w.field.real(), OpticalSignal::getWavelength(w.wavelength_id)});
        }
        EXPECT_EQ(at3, (std::set<std::pair<double, double>>{{4, 1.55e-6}, {3, 1.56e-6}}));
        EXPECT_EQ(at5, (std::set<std::pair<double, double>>{{2, 1.55e-6}, {5, 1.56e-6}}));
    }
    SPECS_TEST_FOOTER();
}

// reset() (between the points of a .dc sweep) drops the pending entries and their notification
TEST(OpticalOutputPort, PendingResetCancelsNotification)
{
    SPECS_TEST_HEADER()
    {
        PendingHarness h("h");
        auto &oop = h.m_out_writer;
        sc_start(SC_ZERO_TIME);
        h.write(1, 1.55e-6, 5);
        h.write(2, 1.56e-6, 5);
        sc_start(1, SC_NS);
        oop.reset();
        EXPECT_TRUE(oop.isempty());
        h.write(3, 1.55e-6, 6); // at 7 ns
        sc_start(10, SC_NS);

        EXPECT_NE(sc_get_status(), SC_STOPPED) << "no notification left without an entry";
        ASSERT_EQ(h.seen.size(), 1u);
        EXPECT_EQ(h.seen[0].t, sc_time(7, SC_NS));
        EXPECT_EQ(h.seen[0].field, OpticalSignal::field_type(3, 0));
    }
    SPECS_TEST_FOOTER();
}

// A .dc point (B-003, B-185): after reset(), a port with m_emit_each_wavelength writes the first
// value of each wavelength with an event, also when it is below abstol (written as 0, snapped) or
// equal to the value its net holds; the later values of the point go through the gate. Without the
// flag (.op, .tran), the gate compares the first value with 0 and an unchanged value wakes nothing
TEST(OpticalOutputPort, DcPointWritesFirstValueOfEachWavelengthWithAnEvent)
{
    for (const bool each_wavelength : {false, true})
    {
        SCOPED_TRACE(each_wavelength ? "m_emit_each_wavelength" : "default");
        SPECS_TEST_HEADER()
        {
            PendingHarness h("h");
            auto &oop = h.m_out_writer;
            sc_start(SC_ZERO_TIME);
            oop.m_mode = OpticalOutputPortMode::NO_DELAY;
            oop.m_emit_each_wavelength = each_wavelength;
            auto write = [&oop](double field, double wavelength)
            {
                oop.delayedWrite(OpticalSignal(OpticalSignal::field_type(field, 0), wavelength), SC_ZERO_TIME);
            };

            // first point: two wavelengths (1.56 um, the larger |E|, is taken first)
            oop.reset();
            write(1, 1.55e-6);
            write(2, 1.56e-6);
            sc_start(1, SC_NS);
            ASSERT_EQ(h.seen.size(), 2u);
            EXPECT_EQ(h.out.read(), OpticalSignal(OpticalSignal::field_type(1, 0), 1.55e-6));

            // next point: the same values (the net holds the first one), and one below abstol (1e-8)
            h.seen.clear();
            oop.reset();
            write(1, 1.55e-6);
            write(2, 1.56e-6);
            write(1e-9, 1.57e-6);
            sc_start(1, SC_NS);
            std::set<std::pair<double, double>> seen; // (field, wavelength)
            for (const auto &w : h.seen)
                seen.insert({w.field.real(), OpticalSignal::getWavelength(w.wavelength_id)});
            if (each_wavelength)
            {
                EXPECT_EQ(h.seen.size(), 3u);
                EXPECT_EQ(seen, (std::set<std::pair<double, double>>{{1, 1.55e-6}, {2, 1.56e-6}, {0, 1.57e-6}}));
            }
            else
            {
                // 1.56 um changes the net, then 1.55 um changes it back; 1.57 um is gated
                EXPECT_EQ(h.seen.size(), 2u);
                EXPECT_EQ(seen, (std::set<std::pair<double, double>>{{1, 1.55e-6}, {2, 1.56e-6}}));
            }

            // in the same point, an unchanged value of a wavelength already written is gated
            h.seen.clear();
            write(2, 1.56e-6);
            sc_start(1, SC_NS);
            EXPECT_TRUE(h.seen.empty());
        }
        SPECS_TEST_FOOTER();
    }
}

// OpticalNet::writeWithEvent(): a value-change event also when the value does not change; a
// plain write of the current value notifies nothing (sc_signal)
class NetEventHarness : public sc_module {
public:
    spx::oa_signal_type net{"net"};
    int events = 0;

    NetEventHarness(sc_module_name name)
        : sc_module(name)
    {
        SC_METHOD(on_net_changed);
        sensitive << net;
        dont_initialize();
    }

    void on_net_changed()
    {
        ++events;
    }
};

TEST(OpticalNet, WriteWithEventNotifiesAnUnchangedValue)
{
    SPECS_TEST_HEADER()
    {
        NetEventHarness h("h");
        const OpticalSignal s(OpticalSignal::field_type(1, 0), 1.55e-6);
        sc_start(SC_ZERO_TIME);
        h.net.write(s);
        sc_start(1, SC_NS);
        EXPECT_EQ(h.events, 1);
        h.net.write(s);
        sc_start(1, SC_NS);
        EXPECT_EQ(h.events, 1);
        h.net.writeWithEvent(s);
        sc_start(1, SC_NS);
        EXPECT_EQ(h.events, 2);
        EXPECT_TRUE(h.net.read() == s);
        // the next plain write of the same value notifies nothing again
        h.net.write(s);
        sc_start(1, SC_NS);
        EXPECT_EQ(h.events, 2);
    }
    SPECS_TEST_FOOTER();
}
