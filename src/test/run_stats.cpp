#include <gtest/gtest.h>
#include <systemc.h>

#include <sstream>

#include "optical_output_port.h"
#include "run_progress.h"
#include "run_stats.h"
#include "specs.h"

#include "test/utils.h"

// An OpticalOutputPort bound to a signal, with its own configuration (F-15), and the
// values the signal took
class StatsHarness : public sc_module {
public:
    spx::oa_port_out_type p_out;
    OpticalOutputPort m_out_writer;
    spx::oa_signal_type out{"out"};
    shared_ptr<OpticalOutputPortConfig> config;
    std::vector<OpticalSignal::field_type> seen;

    StatsHarness(sc_module_name name)
        : sc_module(name)
        , m_out_writer("out_writer", p_out)
        , config(make_shared<OpticalOutputPortConfig>())
    {
        SC_HAS_PROCESS(StatsHarness);
        p_out(out);
        config->m_collect_stats = true;
        m_out_writer.setConfig(config);
        SC_METHOD(on_out_changed);
        sensitive << out;
        dont_initialize();
    }

    void on_out_changed()
    {
        seen.push_back(out.read().m_field);
    }
};

typedef OpticalSignal::field_type F;

// --stats counters (F-15): every value written to the port is scheduled (and possibly
// coalesced), every pending entry taken is counted once as emitted or suppressed
TEST(RunStats, PortCountersClassifyEveryEntry)
{
    SPECS_TEST_HEADER()
    {
        StatsHarness h("h");
        auto &oop = h.m_out_writer;
        const OpticalOutputPortStats &s = oop.getStats();
        sc_start(SC_ZERO_TIME); // default tolerances: abstol 1e-8, reltol 1e-4, snap factor 10

        auto write = [&](double field, double delay_ns = 1) {
            oop.delayedWrite(OpticalSignal(F(field, 0), 1.55e-6), sc_time(delay_ns, SC_NS));
        };

        write(1);
        write(0.5); // same (time, wavelength): coalesced, replaces 1
        EXPECT_EQ(oop.pending(), 1u);
        sc_start(2, SC_NS);
        EXPECT_EQ(h.out.read().m_field, F(0.5, 0));
        EXPECT_EQ(s.scheduled, 2u);
        EXPECT_EQ(s.coalesced, 1u);
        EXPECT_EQ(s.processed, 1u);
        EXPECT_EQ(s.emitted, 1u);
        EXPECT_EQ(s.pending_max, 1u);
        EXPECT_EQ(s.pending_sum, 2u);
        EXPECT_TRUE(s.has_emitted);
        EXPECT_EQ(s.last_emission, sc_time(1, SC_NS));

        write(0.5 + 5e-9); // below abstol
        sc_start(2, SC_NS);
        EXPECT_EQ(s.suppressed_abstol, 1u);
        write(0.5 + 2e-5); // above abstol, below reltol * 0.5
        sc_start(2, SC_NS);
        EXPECT_EQ(s.suppressed_reltol, 1u);
        EXPECT_EQ(s.emitted, 1u);
        EXPECT_EQ(s.last_emission, sc_time(1, SC_NS));

        write(0); // to 0: emitted
        sc_start(2, SC_NS);
        EXPECT_EQ(s.emitted, 2u);
        write(5e-8); // passes the gate from 0, snapped to 0 = the value emitted last
        sc_start(2, SC_NS);
        EXPECT_EQ(h.out.read().m_field, F(0, 0));
        EXPECT_EQ(s.suppressed_snap, 1u);
        EXPECT_EQ(s.emitted, 2u);

        // two entries pending at once (two times)
        write(1, 1);
        write(2, 2);
        EXPECT_EQ(s.pending_max, 2u);
        sc_start(3, SC_NS);
        EXPECT_EQ(h.out.read().m_field, F(2, 0));

        EXPECT_EQ(s.scheduled, 8u);
        EXPECT_EQ(s.processed, 7u);
        EXPECT_EQ(s.processed, s.emitted + s.suppressed_abstol + s.suppressed_reltol + s.suppressed_snap);
        EXPECT_EQ(s.scheduled, s.coalesced + s.processed + oop.pending());
        EXPECT_EQ(s.wavelength_ids.size(), 1u);

        // reset() (each .dc point) keeps the counters
        oop.reset();
        EXPECT_EQ(s.scheduled, 8u);
    }
    SPECS_TEST_FOOTER();
}

// Stats off (the default): nothing is counted
TEST(RunStats, NoCountersWhenOff)
{
    SPECS_TEST_HEADER()
    {
        StatsHarness h("h");
        h.config->m_collect_stats = false;
        auto &oop = h.m_out_writer;
        sc_start(SC_ZERO_TIME);
        oop.delayedWrite(OpticalSignal(F(1, 0), 1.55e-6), sc_time(1, SC_NS));
        sc_start(2, SC_NS);
        EXPECT_EQ(h.out.read().m_field, F(1, 0));
        EXPECT_EQ(oop.getStats().scheduled, 0u);
        EXPECT_EQ(oop.getStats().processed, 0u);
    }
    SPECS_TEST_FOOTER();
}

// No coalescing (F-15 expert option): every write is its own entry, entries of the same time
// are taken in write order (FIFO), so the last write of a time step is the one left on the net
TEST(RunStats, NoCoalescingKeepsEveryWriteInOrder)
{
    SPECS_TEST_HEADER()
    {
        StatsHarness h("h");
        h.config->m_coalesce = false;
        auto &oop = h.m_out_writer;
        const OpticalOutputPortStats &s = oop.getStats();
        sc_start(SC_ZERO_TIME);

        // |E| decreasing then increasing: the coalescing queue orders equal times by |E|, this one by write order
        oop.delayedWrite(OpticalSignal(F(1, 0), 1.55e-6), sc_time(1, SC_NS));
        oop.delayedWrite(OpticalSignal(F(3, 0), 1.55e-6), sc_time(1, SC_NS));
        oop.delayedWrite(OpticalSignal(F(2, 0), 1.55e-6), sc_time(1, SC_NS));
        EXPECT_EQ(oop.pending(), 3u);
        EXPECT_TRUE(oop.m_queue.empty());

        sc_start(2, SC_NS);
        EXPECT_EQ(h.seen, (std::vector<F>{1, 3, 2}));
        EXPECT_EQ(s.scheduled, 3u);
        EXPECT_EQ(s.coalesced, 0u);
        EXPECT_EQ(s.processed, 3u);
        EXPECT_EQ(s.emitted, 3u);
        EXPECT_EQ(s.pending_max, 3u);
        EXPECT_TRUE(oop.isempty());

        // frequency-domain writes too
        oop.immediateWriteFrequencyDomain(OpticalSignal(F(4, 0), 1.55e-6));
        oop.immediateWriteFrequencyDomain(OpticalSignal(F(5, 0), 1.55e-6));
        EXPECT_EQ(oop.pending(), 2u);
        sc_start(1, SC_NS);
        EXPECT_EQ(h.seen, (std::vector<F>{1, 3, 2, 4, 5}));
        EXPECT_EQ(s.processed, 5u);

        // drop_queue() empties it
        oop.delayedWrite(OpticalSignal(F(1, 0), 1.55e-6), sc_time(1, SC_NS));
        EXPECT_FALSE(oop.isempty());
        oop.drop_queue();
        EXPECT_TRUE(oop.isempty());
    }
    SPECS_TEST_FOOTER();
}

// Event cap (F-15 expert option): the simulation stops at the end of the delta cycle in which
// the ports sharing the cap have processed max_events entries in total
TEST(RunStats, EventCapStopsTheSimulation)
{
    SPECS_TEST_HEADER()
    {
        StatsHarness a("a");
        StatsHarness b("b");
        auto cap = make_shared<OpticalOutputPortEventCap>(3);
        a.config->m_event_cap = cap;
        b.config->m_event_cap = cap;
        sc_start(SC_ZERO_TIME);

        for (int k = 1; k <= 3; ++k)
        {
            a.m_out_writer.delayedWrite(OpticalSignal(F(k, 0), 1.55e-6), sc_time(k, SC_NS));
            b.m_out_writer.delayedWrite(OpticalSignal(F(k, 0), 1.55e-6), sc_time(k, SC_NS));
        }
        sc_start(10, SC_NS);
        EXPECT_EQ(sc_get_status(), SC_STOPPED);
        EXPECT_TRUE(cap->reached);
        // 2 at 1 ns, the third at 2 ns stops it; the other entry of that delta cycle is processed too
        EXPECT_EQ(cap->processed, 4u);
        EXPECT_EQ(sc_time_stamp(), sc_time(2, SC_NS));
        EXPECT_EQ(a.out.read().m_field, F(2, 0));
        EXPECT_EQ(b.out.read().m_field, F(2, 0));
    }
    SPECS_TEST_FOOTER();
}

// RunStats: sums, rows, sections and the JSON layout
TEST(RunStats, CollectAndJson)
{
    SPECS_TEST_HEADER()
    {
        StatsHarness a("a");
        StatsHarness b("b");
        sc_start(SC_ZERO_TIME);
        a.m_out_writer.delayedWrite(OpticalSignal(F(1, 0), 1.55e-6), sc_time(1, SC_NS));
        a.m_out_writer.delayedWrite(OpticalSignal(F(1 + 1e-9, 0), 1.55e-6), sc_time(2, SC_NS));
        b.m_out_writer.delayedWrite(OpticalSignal(F(1, 0), 1.31e-6), sc_time(1, SC_NS));
        b.m_out_writer.delayedWrite(OpticalSignal(F(2, 0), 1.31e-6), sc_time(5, SC_NS));
        sc_start(3, SC_NS);

        RunStats stats;
        stats.sections = RunStats::SUMMARY | RunStats::PORTS | RunStats::DEVICES;
        stats.analysis = "TRAN";
        stats.abstol = 1e-8;
        stats.reltol = 1e-4;
        stats.snap_factor = 10;
        stats.end_time_s = 3e-9;
        stats.wall_time_s = 0.25;
        stats.collect({&a.m_out_writer, &b.m_out_writer});

        EXPECT_EQ(stats.totals.output_ports, 2u);
        EXPECT_EQ(stats.totals.scheduled, 4u);
        EXPECT_EQ(stats.totals.processed, 3u);
        EXPECT_EQ(stats.totals.emitted, 2u);
        EXPECT_EQ(stats.totals.suppressed_abstol, 1u);
        EXPECT_EQ(stats.totals.pending_end, 1u);
        ASSERT_EQ(stats.ports.size(), 2u);
        EXPECT_EQ(stats.ports[0].port, "a.out_writer");
        EXPECT_EQ(stats.ports[0].net, "a.out");
        EXPECT_EQ(stats.ports[0].writer, "a");
        ASSERT_EQ(stats.devices.size(), 1u);
        EXPECT_EQ(stats.devices[0].type, "StatsHarness");
        EXPECT_EQ(stats.devices[0].instances, 2u);

        const std::string expected =
            "{\n"
            "  \"format\": \"specs-run-stats\",\n"
            "  \"format_version\": 1,\n"
            "  \"analysis\": \"TRAN\",\n"
            "  \"sections\": [\"summary\", \"ports\", \"devices\"],\n"
            "  \"options\": {\"abstol\": 1e-08, \"reltol\": 1e-04, \"snapfactor\": 10, \"coalesce\": true, \"maxevents\": 0},\n"
            "  \"event_cap_reached\": false,\n"
            "  \"end_time_s\": 3e-09,\n"
            "  \"wall_time_s\": 0.25,\n"
            "  \"totals\": {\"output_ports\": 2, \"scheduled\": 4, \"coalesced\": 0, \"processed\": 3, \"emitted\": 2, "
            "\"suppressed\": {\"abstol\": 1, \"reltol\": 0, \"snap\": 0}, \"pending_max\": 2, \"pending_mean\": 1.5, "
            "\"pending_end\": 1, \"last_emission_s\": 1e-09},\n"
            "  \"ports\": [\n"
            "    {\"port\": \"a.out_writer\", \"net\": \"a.out\", \"writer\": \"a\", \"wavelengths_m\": [1.55e-06], "
            "\"scheduled\": 2, \"coalesced\": 0, \"processed\": 2, \"emitted\": 1, "
            "\"suppressed\": {\"abstol\": 1, \"reltol\": 0, \"snap\": 0}, \"pending_max\": 2, \"pending_mean\": 1.5, "
            "\"pending_end\": 0, \"last_emission_s\": 1e-09},\n"
            "    {\"port\": \"b.out_writer\", \"net\": \"b.out\", \"writer\": \"b\", \"wavelengths_m\": [1.31e-06], "
            "\"scheduled\": 2, \"coalesced\": 0, \"processed\": 1, \"emitted\": 1, "
            "\"suppressed\": {\"abstol\": 0, \"reltol\": 0, \"snap\": 0}, \"pending_max\": 2, \"pending_mean\": 1.5, "
            "\"pending_end\": 1, \"last_emission_s\": 1e-09}\n"
            "  ],\n"
            "  \"devices\": [\n"
            "    {\"type\": \"StatsHarness\", \"instances\": 2, \"output_ports\": 2, \"scheduled\": 4, \"coalesced\": 0, "
            "\"processed\": 3, \"emitted\": 2, \"suppressed\": {\"abstol\": 1, \"reltol\": 0, \"snap\": 0}, "
            "\"pending_max\": 2, \"pending_mean\": 1.5, \"pending_end\": 1, \"last_emission_s\": 1e-09}\n"
            "  ]\n"
            "}\n";
        EXPECT_EQ(stats.to_json(), expected);

        // summary only; nothing emitted: null
        RunStats empty;
        empty.analysis = "OP";
        const std::string json = empty.to_json();
        EXPECT_EQ(json.find("\"ports\""), std::string::npos);
        EXPECT_EQ(json.find("\"devices\""), std::string::npos);
        EXPECT_NE(json.find("\"pending_mean\": 0, \"pending_end\": 0, \"last_emission_s\": null}\n}\n"), std::string::npos)
            << json;
    }
    SPECS_TEST_FOOTER();
}

// A port whose signal was destroyed before the statistics are collected (the -t testbenches
// destroy theirs when their function returns): no net, and its dangling interface pointer is
// never dereferenced (ASan builds check it)
class ExternalSignalHarness : public sc_module {
public:
    spx::oa_port_out_type p_out;
    OpticalOutputPort m_out_writer;

    ExternalSignalHarness(sc_module_name name, spx::oa_signal_type &sig)
        : sc_module(name)
        , m_out_writer("out_writer", p_out)
    {
        p_out(sig);
        auto config = make_shared<OpticalOutputPortConfig>();
        config->m_collect_stats = true;
        m_out_writer.setConfig(config);
    }
};

TEST(RunStats, PortOfADestroyedSignalHasNoNet)
{
    SPECS_TEST_HEADER()
    {
        auto *sig = new spx::oa_signal_type("gone");
        ExternalSignalHarness h("h", *sig);
        sc_start(SC_ZERO_TIME);
        h.m_out_writer.delayedWrite(OpticalSignal(F(1, 0), 1.55e-6), sc_time(1, SC_NS));
        sc_start(2, SC_NS);
        EXPECT_EQ(sig->read().m_field, F(1, 0));

        RunStats stats;
        stats.sections = RunStats::SUMMARY | RunStats::PORTS;
        stats.collect({&h.m_out_writer});
        ASSERT_EQ(stats.ports.size(), 1u);
        EXPECT_EQ(stats.ports[0].net, "gone");

        delete sig;
        stats.collect({&h.m_out_writer});
        ASSERT_EQ(stats.ports.size(), 1u);
        EXPECT_EQ(stats.ports[0].net, "");
        EXPECT_EQ(stats.ports[0].writer, "h");
        EXPECT_EQ(stats.totals.emitted, 1u);
        EXPECT_NE(stats.to_json().find("{\"port\": \"h.out_writer\", \"net\": null, \"writer\": \"h\""), std::string::npos);
    }
    SPECS_TEST_FOOTER();
}

TEST(RunStats, ParseSections)
{
    unsigned sections = 0;
    EXPECT_TRUE(RunStats::parse_sections("summary", sections));
    EXPECT_EQ(sections, unsigned(RunStats::SUMMARY));
    EXPECT_TRUE(RunStats::parse_sections("ports", sections));
    EXPECT_EQ(sections, unsigned(RunStats::SUMMARY | RunStats::PORTS));
    EXPECT_TRUE(RunStats::parse_sections("devices", sections));
    EXPECT_EQ(sections, unsigned(RunStats::SUMMARY | RunStats::DEVICES));
    EXPECT_TRUE(RunStats::parse_sections("ports,devices", sections));
    EXPECT_EQ(sections, unsigned(RunStats::SUMMARY | RunStats::PORTS | RunStats::DEVICES));
    EXPECT_FALSE(RunStats::parse_sections("port", sections));
    EXPECT_FALSE(RunStats::parse_sections("ports,,devices", sections));
    EXPECT_FALSE(RunStats::parse_sections("Ports", sections));
}

TEST(RunStats, JsonNumbers)
{
    EXPECT_EQ(RunStats::json_number(0), "0");
    EXPECT_EQ(RunStats::json_number(10), "10");
    EXPECT_EQ(RunStats::json_number(1e-8), "1e-08");
    EXPECT_EQ(RunStats::json_number(0.1), "0.1");
    EXPECT_EQ(RunStats::json_number(1.0 / 3), "0.3333333333333333");
    EXPECT_EQ(RunStats::json_number(std::numeric_limits<double>::infinity()), "null");
    EXPECT_EQ(RunStats::json_number(std::numeric_limits<double>::quiet_NaN()), "null");
}

// --progress (F-15): lines on the given stream while the simulation runs; the simulation is the
// same with and without it (time, values) and it never stops it
static void run_progress_case(bool with_progress)
{
    StatsHarness h("h");
    std::ostringstream os;
    RunProgress progress(1e-9, 10e-9, os); // a line after (almost) every delta cycle
    if (with_progress)
        progress.start();
    sc_start(SC_ZERO_TIME);
    for (int k = 1; k <= 5; ++k)
        h.m_out_writer.delayedWrite(OpticalSignal(F(k, 0), 1.55e-6), sc_time(k, SC_NS));
    sc_start(10, SC_NS);
    progress.stop();
    const uint64_t lines = progress.lines_printed();
    sc_start(1, SC_NS);
    EXPECT_EQ(progress.lines_printed(), lines) << "no line once stopped";

    EXPECT_NE(sc_get_status(), SC_STOPPED);
    EXPECT_EQ(sc_time_stamp(), sc_time(11, SC_NS));
    EXPECT_EQ(h.seen, (std::vector<F>{1, 2, 3, 4, 5}));
    if (with_progress)
    {
        EXPECT_GE(lines, 5u);
        const std::string text = os.str();
        EXPECT_EQ(text.rfind("Progress: ", 0), 0u) << text;
        EXPECT_NE(text.find(" % of 10 ns), "), std::string::npos) << text;
        EXPECT_NE(text.find(" delta cycles\n"), std::string::npos) << text;
        size_t n_lines = 0;
        for (char c : text)
            n_lines += c == '\n';
        EXPECT_EQ(n_lines, lines);
    }
    else
    {
        EXPECT_EQ(lines, 0u);
        EXPECT_EQ(os.str(), "");
    }
}

TEST(RunProgress, Off)
{
    SPECS_TEST_HEADER()
    {
        run_progress_case(false);
    }
    SPECS_TEST_FOOTER();
}

TEST(RunProgress, PrintsWithoutChangingTheSimulation)
{
    SPECS_TEST_HEADER()
    {
        run_progress_case(true);
    }
    SPECS_TEST_FOOTER();
}
