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
