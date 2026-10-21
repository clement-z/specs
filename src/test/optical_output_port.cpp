#include <gtest/gtest.h>
#include <systemc.h>

#include "optical_output_port.h"
#include "specs.h"

#include "test/utils.h"

// Owns an OpticalOutputPort bound to a signal, so that the thread
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

        h.m_out_writer.m_event_queue.notify(sc_time(1, SC_NS));
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
        h.m_out_writer.m_event_queue.notify(sc_time(1, SC_NS));
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
        h.m_out_writer.m_event_queue.notify(sc_time(1, SC_NS));
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
