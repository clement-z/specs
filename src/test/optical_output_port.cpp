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
