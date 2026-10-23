// B-048 maintainer condition: at TRAN start, the probes (PROBE, MLPROBE and the traceall auto-probes)
// trace the state of their net after the operating point (.nodeset included) and after the
// initial conditions, from t = 0. runTRANAnalysis() runs (a) runOPAnalysis(), (b)
// applyInitialConditions(), (c) the probe enables, in that order.
//
// The parser rejects .ic for now (B-161), so the initial conditions are given through
// SPECSConfig::ic_orders here. When .ic is implemented, it must fill ic_orders (or keep this order:
// IC before the probe enables) and this test stays valid.
#include <gtest/gtest.h>
#include <systemc.h>

#include <cmath>
#include <complex>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "devices/cw_source.h"
#include "devices/probe.h"
#include "devices/waveguide.h"
#include "optical_output_port.h"
#include "specs.h"
#include "test/utils.h"

namespace {

// Records every value written to a trace signal of a probe, with its time stamp
class TraceRecorder : public sc_module {
public:
    const sc_signal<double> &m_sig;
    std::vector<std::pair<sc_time, double>> m_history;

    TraceRecorder(sc_module_name name, const sc_signal<double> &sig)
        : sc_module(name)
        , m_sig(sig)
    {
        SC_METHOD(on_sig_changed);
        sensitive << m_sig;
        dont_initialize();
    }

    void on_sig_changed()
    {
        m_history.emplace_back(sc_time_stamp(), m_sig.read());
    }
};

OpticalSignal makeSignal(double power, double phase)
{
    return OpticalSignal(std::polar(std::sqrt(power), phase), 1.55e-6);
}

}

TEST(TranStart, ProbesSampleAfterOperatingPointAndInitialConditions)
{
    SPECS_TEST_HEADER()
    {
        SPECSConfig &c = specsGlobalConfig;
        c.analysis_type = SPECSConfig::TRAN;
        c.trace_all_optical_nets = true; // auto-probes PROBE{<net>} too
        c.trace_filename = testing::TempDir() + "specs_test_" + std::to_string(getpid());
        c.engine_timescale = SPECSConfig::ONE_FS;
        c.tran_duration = 1e-12;
        c.applyEngineResolution();

        // CW source -> waveguide -> `out`: its value comes from the OP
        spx::oa_signal_type in{"in"};
        spx::oa_signal_type out{"out"};
        CWSource cw("cw");
        cw.setWavelength(1.55e-6);
        cw.setPower(1e-3);
        cw.p_out(in);
        WaveguideUni wg("wg", 1e-2, 0, 2.2, 4.2); // 100 um, lossless
        wg.p_in(in);
        wg.p_out(out);

        // Nets that no device writes: their value only comes from the nodeset (an OP-phase event,
        // seen while the probes are disabled) and/or from the initial conditions (written after the
        // OP). `ns_ic` has both: the IC wins, and the probes must never trace the nodeset value.
        spx::oa_signal_type ns{"ns"};
        spx::oa_signal_type ic{"ic"};
        spx::oa_signal_type ns_ic{"ns_ic"};
        const OpticalSignal ns_value = makeSignal(2e-3, 0.25);
        const OpticalSignal ic_value = makeSignal(4e-3, -0.5);
        const OpticalSignal ns_ic_nodeset = makeSignal(3e-3, 1.0);
        const OpticalSignal ns_ic_value = makeSignal(5e-3, -1.5);
        c.nodeset_orders["ns"] = {&ns, ns_value};
        c.nodeset_orders["ns_ic"] = {&ns_ic, ns_ic_nodeset};
        c.ic_orders["ic"] = {&ic, ic_value};
        c.ic_orders["ns_ic"] = {&ns_ic, ns_ic_value};

        Probe probe_out("probe_out");
        probe_out.p_in(out);
        Probe probe_ns("probe_ns");
        probe_ns.p_in(ns);
        Probe probe_ic("probe_ic");
        probe_ic.p_in(ic);
        Probe probe_ns_ic("probe_ns_ic");
        probe_ns_ic.p_in(ns_ic);
        MLambdaProbe mlprobe_out("mlprobe_out", {1.55e-6});
        mlprobe_out.p_in(out);
        MLambdaProbe mlprobe_ns_ic("mlprobe_ns_ic", {1.55e-6});
        mlprobe_ns_ic.p_in(ns_ic);

        // The steps of runAnalysis() (applyEngineResolution() was called above, it can only run once),
        // split so that the recorders of the auto-probes can be created once prepareSimulation() has
        // created the auto-probes
        c.verifyConfig();
        c.prepareSimulation();

        // (probe, its net): each probe is checked against the value of its net after the run, which is
        // the post-OP / post-IC value (the TRAN changes none of these nets, checked below)
        std::vector<std::pair<const Probe *, const spx::oa_signal_type *>> probes;
        auto auto_probe = [](const char *net) {
            auto p = dynamic_cast<const Probe *>(sc_find_object((std::string("PROBE{") + net + "}").c_str()));
            EXPECT_NE(p, nullptr) << "no auto-probe on " << net;
            return p;
        };
        for (const Probe *p : {(const Probe *)&probe_out, auto_probe("out")})
            probes.emplace_back(p, &out);
        for (const Probe *p : {(const Probe *)&probe_ns, auto_probe("ns")})
            probes.emplace_back(p, &ns);
        for (const Probe *p : {(const Probe *)&probe_ic, auto_probe("ic")})
            probes.emplace_back(p, &ic);
        for (const Probe *p : {(const Probe *)&probe_ns_ic, auto_probe("ns_ic")})
            probes.emplace_back(p, &ns_ic);
        for (const auto &x : probes)
            ASSERT_NE(x.first, nullptr);

        std::vector<std::unique_ptr<TraceRecorder>> recorders;
        int i = 0;
        for (const auto &x : probes)
        {
            for (const sc_signal<double> *sig : {&x.first->m_trace_sig_power, &x.first->m_trace_sig_modulus,
                                                 &x.first->m_trace_sig_phase, &x.first->m_trace_sig_wavelength})
                recorders.push_back(std::make_unique<TraceRecorder>(("rec" + std::to_string(i++)).c_str(), *sig));
        }
        const size_t n_probe_recorders = recorders.size();
        for (const MLambdaProbe *p : {&mlprobe_out, &mlprobe_ns_ic})
        {
            for (const auto &sig : p->m_lambda_signals.at(1.55e-6))
                recorders.push_back(std::make_unique<TraceRecorder>(("rec" + std::to_string(i++)).c_str(), *sig));
        }

        c.runTRANAnalysis();
        EXPECT_EQ(sc_time_stamp(), sc_time(1, SC_PS));

        // Post-OP / post-IC values of the nets (the TRAN changes none of them)
        EXPECT_DOUBLE_EQ(out.read().power(), 1e-3);
        EXPECT_EQ(ns.read(), ns_value);
        EXPECT_EQ(ic.read(), ic_value);
        EXPECT_EQ(ns_ic.read(), ns_ic_value);

        // Every trace signal changed exactly once, at t = 0, to the value of its net: the first
        // sample is the post-OP / post-IC value (no 0 for the whole TRAN, as before B-048, and no
        // pre-IC value first, e.g. the nodeset value of `ns_ic`)
        auto expect_single_sample = [](const TraceRecorder &rec, double expected, const std::string &what) {
            ASSERT_EQ(rec.m_history.size(), 1u) << what;
            EXPECT_EQ(rec.m_history[0].first, SC_ZERO_TIME) << what;
            EXPECT_DOUBLE_EQ(rec.m_history[0].second, expected) << what;
        };
        for (size_t k = 0; k < probes.size(); ++k)
        {
            const Probe &p = *probes[k].first;
            const OpticalSignal s = probes[k].second->read();
            expect_single_sample(*recorders[4 * k + 0], s.power(), std::string(p.name()) + ".power");
            expect_single_sample(*recorders[4 * k + 1], s.modulus(), std::string(p.name()) + ".abs");
            expect_single_sample(*recorders[4 * k + 2], s.phase(), std::string(p.name()) + ".phase");
            expect_single_sample(*recorders[4 * k + 3], 1.55e-6, std::string(p.name()) + ".wavelength");
        }
        const OpticalSignal ml_expected[] = {out.read(), ns_ic_value};
        for (size_t k = 0; k < 2; ++k)
        {
            const OpticalSignal &s = ml_expected[k];
            expect_single_sample(*recorders[n_probe_recorders + 3 * k + 0], s.power(), "mlprobe power");
            expect_single_sample(*recorders[n_probe_recorders + 3 * k + 1], s.modulus(), "mlprobe abs");
            expect_single_sample(*recorders[n_probe_recorders + 3 * k + 2], s.phase(), "mlprobe phase");
        }
        EXPECT_DOUBLE_EQ(probe_ns_ic.m_trace_sig_power.read(), 5e-3);
        EXPECT_DOUBLE_EQ(probe_ns.m_trace_sig_power.read(), 2e-3);
        EXPECT_DOUBLE_EQ(probe_ic.m_trace_sig_power.read(), 4e-3);
        EXPECT_DOUBLE_EQ(probe_out.m_trace_sig_power.read(), 1e-3);
    }
    SPECS_TEST_FOOTER();
}
