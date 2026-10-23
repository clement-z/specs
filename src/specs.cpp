#include "devices/bitstream_source.h"
#include "optical_signal.h"
#include "utils/sysc_utils.h"
#include "specs.h"
#include "devices/spx_module.h"
#include "devices/value_list_source.h"
#include "devices/electrical_value_list_source.h"

#include <systemc.h>
#include "optical_output_port.h"
#include "devices/cw_source.h"
#include "devices/probe.h"
#include "devices/detector.h"
#include "devices/power_meter.h"
#include "run_manifest.h"
#include "run_progress.h"
#include "run_stats.h"
#include "trace_table.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>

using std::string;

SPECSConfig specsGlobalConfig { "SPX" };

SPECSConfig::SPECSConfig(sc_module_name name)
    : sc_module(name)
{
    drop_all_events = false;
    // Simulation options
    engine_timescale = HUNDRED_FS;
    simulation_mode = OpticalOutputPortMode::DEFAULT;
    analysis_type = AnalysisType::TIME_DOMAIN;
    seed = DEFAULT_SEED;

    // Port options
    default_abstol = 1e-8;
    default_reltol = 1e-4;
    default_snap_factor = 10;
    default_resolution_multiplier = 1;

    // Trace options
    trace_filename = "traces/delete_me";
    default_trace_file = nullptr;
    trace_all_optical_nets = 1;
}

SPECSConfig::~SPECSConfig() {}

void SPECSConfig::runAnalysis()
{
    run_start = std::chrono::steady_clock::now();
    verifyConfig();
    applyEngineResolution();
    prepareSimulation();

    // F-15 --progress: passive, from the first sc_start() to the end of the analysis
    std::unique_ptr<RunProgress> progress;
    if (progress_period_s > 0)
    {
        progress = std::make_unique<RunProgress>(progress_period_s,
                                                 analysis_type == TIME_DOMAIN ? tran_duration
                                                                              : std::numeric_limits<double>::infinity());
        progress->start();
    }

    switch (analysis_type) {
        case CW_OPERATING_POINT:
            runOPAnalysis();
            break;
        case CW_SWEEP:
            runDCAnalysis();
            break;
        case TIME_DOMAIN:
            runTRANAnalysis();
            break;
        default:
            cerr << "Undefined Analysis type";
            sc_stop();
    }
    if (progress)
        progress->stop();

    // B-062: the wavelengths each MLPROBE ignored, one warning per probe, in name order
    for (auto mlprobe : sc_get_all_module_by_type<MLambdaProbe>())
        mlprobe->reportIgnoredWavelengths();

    // F-05: write the .npz table, close the CSV
    if (default_trace_table && !default_trace_table->close())
        exit(1);

    writeRunStats();
}

// F-15 event cap reached during an .op/.dc fixed point (also the operating point of a .tran): the
// fixed point was cut short, so its values are not a result. Error (exit 1), after writing the
// run statistics
void SPECSConfig::checkEventCap(const string &what) const
{
    if (!eventCapReached())
        return;
    cerr << "Error: " << what << " stopped at the event cap: " << event_cap->processed
         << " pending entries processed (.options maxevents=" << max_events << ")" << endl;
    writeRunStats();
    exit(1);
}

// F-15 --stats: the counters of every optical output port, summed (src/run_stats.h)
void SPECSConfig::writeRunStats() const
{
    if (!stats_sections)
        return;
    RunStats stats;
    stats.sections = stats_sections;
    switch (analysis_type) {
        case CW_OPERATING_POINT:
            stats.analysis = "OP";
            break;
        case CW_SWEEP:
            stats.analysis = "DC";
            break;
        case TIME_DOMAIN:
            stats.analysis = "TRAN";
            break;
        default:
            stats.analysis = "UNDEFINED";
    }
    stats.abstol = default_abstol;
    stats.reltol = default_reltol;
    stats.snap_factor = default_snap_factor;
    stats.max_events = max_events;
    stats.event_cap_reached = eventCapReached();
    stats.end_time_s = sc_time_stamp().to_seconds();
    stats.collect(sc_get_all_module_by_type<OpticalOutputPort>());
    stats.wall_time_s = std::chrono::duration<double>(std::chrono::steady_clock::now() - run_start).count();
    const string json = stats.to_json();

    // an explicit file, else next to the default trace file, else (no trace file) on stdout
    const string filename = !stats_filename.empty() ? stats_filename
                          : !trace_filename.empty() ? trace_filename + ".stats.json" : "";
    if (!filename.empty())
    {
        std::ofstream outfile(filename, std::ios::out | std::ios::trunc);
        if (outfile)
            outfile << json;
        outfile.close();
        if (!outfile)
        {
            cerr << "Error: cannot write run statistics file \"" << filename << "\"" << endl;
            exit(1);
        }
        cout << "Run statistics written to " << filename << endl;
    }
    if (filename.empty() || print_stats)
        cout << json << flush;
}

namespace {

void setProbeLatchMode(Probe::LatchMode mode)
{
    for (auto probe : sc_get_all_module_by_type<Probe>())
        probe->m_latch_mode = mode;
}

// PD5: a plain probe traces one wavelength of its net. Once per run, in name order: one warning per
// probe that ignored another wavelength, one line for all the automatic probes (PROBE{<net>}:
// traceall, .save)
void reportProbeWavelengthLatches()
{
    vector<const Probe *> automatic;
    for (const Probe *probe : sc_get_all_module_by_type<Probe>())
    {
        if (isnan(probe->m_ignored_wavelength_m))
            continue;
        if (probe->m_automatic)
        {
            automatic.push_back(probe);
            continue;
        }
        cerr << "Warning: " << probe->name() << " traces one wavelength of its net "
             << port_channel_name(probe->p_in) << " (" << probe->m_traced_when_ignored_m * 1e9
             << " nm) and ignores the others (" << probe->m_ignored_wavelength_m * 1e9
             << " nm): use MLPROBE for per-wavelength values" << endl;
    }
    if (!automatic.empty())
    {
        const Probe *first = automatic.front();
        cerr << "Warning: " << automatic.size() << " automatic probe(s) PROBE{<net>} trace one wavelength of "
             << "their net and ignore the others (e.g. " << first->name() << " traces "
             << first->m_traced_when_ignored_m * 1e9 << " nm and ignores " << first->m_ignored_wavelength_m * 1e9
             << " nm): use MLPROBE for per-wavelength values" << endl;
    }
}

}

void SPECSConfig::runOPAnalysis()
{
    auto all_probes = sc_get_all_module_by_type<Probe>();
    auto all_mlprobes = sc_get_all_module_by_type<MLambdaProbe>();
    auto all_photodetectors = sc_get_all_module_by_type<Detector>();
    auto all_oop = sc_get_all_module_by_type<OpticalOutputPort>();
    auto all_cws = sc_get_all_module_by_type<CWSource>();

    // .op: one analysis point (the probes of a .tran latch once, set by runTRANAnalysis(), PD5)
    if (analysis_type != TIME_DOMAIN)
        setProbeLatchMode(Probe::LATCH_PER_TIME_STEP);

    // Disable photodetectors before the first sc_start(): they are enabled by default
    // (testbenches, C++ users) but must not write their readout during the OP
    for (auto pdet: all_photodetectors) {
        pdet->enable = sc_logic(0);
    }

    // Run to initialize all threads and register first values
    sc_start();
    checkEventCap(analysis_type == TIME_DOMAIN ? ".tran: the operating point before the transient"
                                               : ".op: the operating point");

    // Set values of signals according to NODESET directive
    for (auto &nodeset_order : nodeset_orders)
    {
        nodeset_order.second.first->write(nodeset_order.second.second);
    }

    // Disable probes for OP point
    for (auto probe: all_probes) {
        probe->enable = sc_logic(0);
    }

    // Disable ML probes for OP point
    for (auto mlprobe: all_mlprobes) {
        mlprobe->enable = sc_logic(0);
    }

    // Set all ports mode to NO_DELAY
    for (auto oop: all_oop) {
        oop->m_mode = OpticalOutputPortMode::NO_DELAY;
    }

    // Activate all CW sources
    for (auto cws: all_cws)
        cws->enable = sc_logic(1);

    // Run operating point simulation: the delta-cycle fixed point, at most max_iterations
    // delta cycles (F-28), then what is left at later times, as sc_start() did (nothing in FD)
    const sc_dt::uint64 start_delta = sc_delta_count();
    const bool idle = !sc_pending_activity_at_current_time();
    const string what = analysis_type == TIME_DOMAIN ? ".tran: the operating point before the transient"
                                                     : ".op: the operating point";
    if (!runFixedPoint())
        reportNonConvergence(what, start_delta);
    checkEventCap(what);
    // (an idle circuit keeps the SystemC warning W571, no activity, that this sc_start() gave)
    if (idle || sc_pending_activity())
        sc_start();
    checkEventCap(what);

    // Print results on the command line
    printOPAnalysisResult();
}

void SPECSConfig::runDCAnalysis()
{
    auto all_probes = sc_get_all_module_by_type<Probe>();
    auto all_mlprobes = sc_get_all_module_by_type<MLambdaProbe>();
    auto all_photodetectors = sc_get_all_module_by_type<Detector>();
    auto all_oop = sc_get_all_module_by_type<OpticalOutputPort>();
    auto all_sig = sc_get_all_object_by_type<sc_signal<OpticalSignal, SC_MANY_WRITERS>>();
    auto all_cws = sc_get_all_module_by_type<CWSource>();

    // Each sweep point (one time step) latches its own wavelength (PD5)
    setProbeLatchMode(Probe::LATCH_PER_TIME_STEP);

    // Disable photodetectors before the first sc_start() (they are enabled by default)
    for (auto pdet: all_photodetectors) {
        // Remark: Photodetectors could in fact be left enabled ?
        // Would that serve any purpose though...?
        pdet->enable = sc_logic(0);
    }

    // Run to initialize all threads and register first values
    sc_start();
    checkEventCap(".dc: the initialisation");

    // Enable normal probes
    for (auto probe: all_probes) {
        probe->enable = sc_logic(1);
    }

    // Disable ML probes
    for (auto mlprobe: all_mlprobes) {
        mlprobe->enable = sc_logic(0);
    }

    // Set all ports mode to NO_DELAY
    for (auto oop: all_oop) {
        oop->m_mode = OpticalOutputPortMode::NO_DELAY;
    }

    // Activate CW sources
    for (auto cws: all_cws) {
        cws->enable = sc_logic(1);
    }

    if (cw_sweep_orders.empty())
    {
        cerr << "Error: .dc analysis requires a sweep order (e.g. .dc /cwsrc1/WL start stop step)" << endl;
        exit(1);
    }
    const auto &order = *cw_sweep_orders.begin();
    wavelengths_vector.reserve(order.second.values.size());
    cout << "Starting sweep on " << order.first;
    cout << " (" << order.second.values.size() << " points)" << endl;
    size_t point = 0;
    for (const auto &val : order.second.values)
    {
        ++point;
        // Apply sweep param
        order.second.apply(val);

        // A wavelength (or frequency) sweep: the probes latch the swept wavelength of this point
        // when their net carries it (PD5)
        const double swept_wavelength_m = order.second.sweeps_wavelength
                                              ? order.second.source->m_signal_on.getWavelength()
                                              : std::numeric_limits<double>::quiet_NaN();
        for (auto probe: all_probes)
            probe->setSweptWavelength(swept_wavelength_m);

        //cout << order.first << " = " << val << endl;

        // Reset OOP and set internal signals to new wavelength
        for (auto oop: all_oop) {
            oop->reset();
            // TODO: also reset photodetectors?
        }

        // run the fixed point (at most max_iterations delta cycles, F-28) and advance one tick
        const sc_dt::uint64 start_delta = sc_delta_count();
        stringstream what;
        what << ".dc: sweep point " << point << " of " << order.second.values.size()
             << " (" << order.first << " = " << val << ")";
        if (!runFixedPoint())
            reportNonConvergence(what.str(), start_delta);
        checkEventCap(what.str());
        sc_start(sc_time::from_value(1));
        checkEventCap(what.str());

        // Reset CW sources
        for (auto cws: all_cws)
        {
            cws->reset.write(sc_logic(1));
        }

        //printOPAnalysisResult();
    }
    reportProbeWavelengthLatches();
}

void SPECSConfig::runTRANAnalysis()
{
    auto all_probes = sc_get_all_module_by_type<Probe>();
    auto all_mlprobes = sc_get_all_module_by_type<MLambdaProbe>();
    auto all_photodetectors = sc_get_all_module_by_type<Detector>();
    auto all_oop = sc_get_all_module_by_type<OpticalOutputPort>();
    auto all_vl_src = sc_get_all_module_by_type<VLSource>();
    auto all_evl_src = sc_get_all_module_by_type<EVLSource>();

    // TRAN start sequence (B-048):
    // (a) operating point, .nodeset values included
    // (b) initial conditions
    // (c) enable the probes: each one samples the current value of its net at that point
    //     (and the photodetectors, which write their readout)
    // (b) must stay before (c), so that the probes trace the post-OP, post-IC state at t = 0
    // (B-048 maintainer condition, see TranStart.ProbesSampleAfterOperatingPointAndInitialConditions)

    // The probes latch one wavelength for the whole transient, when they are enabled (PD5)
    setProbeLatchMode(Probe::LATCH_ONCE);

    // (a) Run OP analysis
    runOPAnalysis();

    // (b) Apply initial conditions
    applyInitialConditions();

    // (c) Enable probes
    for (auto probe: all_probes) {
        probe->enable = sc_logic(1);
    }

    // (c) Enable ML probes
    for (auto mlprobe: all_mlprobes) {
        mlprobe->enable = sc_logic(1);
    }

    // (c) Enable photodetectors: each one writes the readout of the fields it stored (B-049)
    for (auto pdet: all_photodetectors) {
        pdet->enable = sc_logic(1);
    }

    // Re-apply configuration of all ports
    for (auto oop: all_oop) {
        oop->applyConfig();
    }

    // Activate TRAN sources
    for (auto src: all_vl_src)
        src->enable = sc_logic(1);
    for (auto src: all_evl_src)
        src->enable = sc_logic(1);

    // Strart TRAN simulation
    if (isfinite(tran_duration))
        sc_start(tran_duration, SC_SEC);
    else
        sc_start();

    // F-15 event cap: the transient ends there (its traces stop at that time)
    if (eventCapReached())
        cerr << "Warning: .tran stopped at the event cap at " << sc_time_stamp() << ": " << event_cap->processed
             << " pending entries processed (.options maxevents=" << max_events << ")" << endl;

    reportProbeWavelengthLatches();
    cout << "Simulated " << sc_time_stamp() << endl;
}

// The .op/.dc fixed point (F-28): runs the delta cycles of the current time step one at a time,
// as sc_start() would, but at most max_iterations of them (0: no limit). Returns false if
// activity is still pending at the current time after that
bool SPECSConfig::runFixedPoint()
{
    uint64_t iterations = 0;
    while (sc_pending_activity_at_current_time() && sc_get_status() != SC_STOPPED)
    {
        if (max_iterations && iterations >= max_iterations)
            return false;
        sc_start(SC_ZERO_TIME);
        ++iterations;
    }
    return true;
}

// Non-convergence of runFixedPoint() (F-28): error message and residual, then exit(1).
// The residual lists the output ports that still emitted in the last tenth of the iterations
// (at least the last 100 delta cycles), with their last change and its emission threshold
void SPECSConfig::reportNonConvergence(const string &what, sc_dt::uint64 start_delta) const
{
    const sc_dt::uint64 end_delta = sc_delta_count();
    const sc_dt::uint64 window = std::max<sc_dt::uint64>(100, max_iterations / 10);
    const sc_dt::uint64 window_start = std::max(start_delta, end_delta > window ? end_delta - window : 0);

    vector<OpticalOutputPort *> changing;
    for (auto oop : sc_get_all_module_by_type<OpticalOutputPort>())
    {
        if (oop->m_has_emitted && oop->m_last_emission_delta >= window_start)
            changing.push_back(oop);
    }
    auto ratio = [](const OpticalOutputPort *oop)
    {
        // zero threshold (abstol = 0, and reltol = 0 or |E_last| = 0, B-153): any change is
        // infinitely above it (said explicitly rather than divided by 0)
        if (oop->lastEmissionThreshold() == 0)
            return std::numeric_limits<double>::infinity();
        return oop->lastEmissionChange() / oop->lastEmissionThreshold();
    };
    // largest change relative to its threshold first, by order of magnitude, then in name order:
    // the ratios of a loop's ports differ only in the last bits, which differ between compilers
    auto decade = [&ratio](const OpticalOutputPort *oop) { return std::floor(std::log10(ratio(oop))); };
    std::stable_sort(changing.begin(), changing.end(),
                     [&decade](const OpticalOutputPort *a, const OpticalOutputPort *b) { return decade(a) > decade(b); });

    cerr << "Error: " << what << " did not converge within " << max_iterations
         << " iterations (delta cycles, .options maxiter)" << endl;
    cerr << "Residual: " << changing.size() << " optical output port(s) changed in the last "
         << end_delta - window_start << " delta cycles. Last change |dE| and emission threshold "
         << "max(abstol, reltol*|E_last|), in sqrt(W):" << endl;
    const size_t max_listed = 10;
    for (size_t i = 0; i < changing.size() && i < max_listed; ++i)
    {
        OpticalOutputPort *oop = changing[i];
        const sc_object *net = dynamic_cast<const sc_object *>(oop->m_port.get_interface());
        const sc_object *device = oop->get_parent_object();
        cerr << "  " << (net ? net->name() : oop->name());
        if (device)
            cerr << " (" << device->name() << ")";
        cerr << " @ " << OpticalSignal::getWavelength(oop->m_last_emission_wlid) * 1e9 << " nm: |dE| = "
             << oop->lastEmissionChange() << " > " << oop->lastEmissionThreshold()
             << " (" << ratio(oop) << " x)" << endl;
    }
    if (changing.size() > max_listed)
        cerr << "  ... and " << changing.size() - max_listed << " more" << endl;
    cerr << "A feedback loop with a round-trip transmission close to 1 (lossless or weakly coupled) converges "
         << "slowly or never: raise .options maxiter (0: no limit), loosen reltol/abstol, or add loss" << endl;
    writeRunStats();
    exit(1);
}

void SPECSConfig::applyInitialConditions()
{
    // Set values of signals according to the initial conditions (same mechanism as
    // nodeset_orders, but applied after the OP). The writes take effect at the next
    // sc_start(), in the same update phase as the probe enables of runTRANAnalysis().
    // ic_orders is only filled from C++ for now: the parser rejects .ic (B-161). A future
    // .ic directive must fill ic_orders, or keep this order (B-048).
    for (auto &ic_order : ic_orders)
    {
        ic_order.second.first->write(ic_order.second.second);
    }
}

void SPECSConfig::applyDefaultOpticalOutputPortConfig() {
    //assert(!oop_configs.empty());
    auto oop_default_config = make_shared<OpticalOutputPortConfig>();
    oop_default_config->m_mode = simulation_mode;
    oop_default_config->m_abstol = default_abstol;
    oop_default_config->m_reltol = default_reltol;
    oop_default_config->m_snap_factor = default_snap_factor;
    oop_default_config->m_timestep_value = default_resolution_multiplier; // relative to systemc timestep
    // F-15: event cap and --stats
    oop_default_config->m_collect_stats = stats_sections != 0;
    if (max_events && !event_cap)
        event_cap = make_shared<OpticalOutputPortEventCap>(max_events);
    oop_default_config->m_event_cap = event_cap;

    // apply default config to all optical output ports which don't have one
    auto all_oop = sc_get_all_module_by_type<OpticalOutputPort>();
    for (auto oop: all_oop) {
        if(!oop->getConfig().get())
            oop->setConfig(oop_default_config);
        // FIXME: only works for first call to sc_start !!
    }
}

void SPECSConfig::applyDefaultTraceFileToAllSignals() {
    TraceTable *table = default_trace_table.get();
    if (!default_trace_file && !table)
        return;

    auto all_probes = sc_get_all_object_by_type<Probe>();
    for (auto p: all_probes) {
        p->setTraceFile(default_trace_file);
        p->setTraceTable(table);
    }

    auto all_mlambda_probes = sc_get_all_object_by_type<MLambdaProbe>();
    for (auto p: all_mlambda_probes) {
        p->setTraceFile(default_trace_file);
        p->setTraceTable(table);
    }

    if (trace_all_optical_nets)
    {
        // nets that already have their automatic probe (.save, F-05) keep it
        set<string> probe_names;
        for (auto p: all_probes)
            probe_names.insert(p->name());

        auto all_optical_sigs = sc_get_all_object_by_type<sc_signal<OpticalSignal, SC_MANY_WRITERS>>();
        for (auto &sig: all_optical_sigs) {
            string signame = sig->name();
            if (probe_names.count("PROBE{" + signame + "}"))
                continue;

            // Check if the signal is explicitly marked as uninteresting
            stringstream ss (signame);
            string base_signame;
            while (getline (ss, base_signame, '/')) {}
            if (base_signame.empty() || base_signame[0] == '_')
                continue;
            //cout << signame << endl;

            auto p = make_shared<Probe>(("PROBE{" + signame + "}").c_str(), true, true, true, true);
            p->m_automatic = true;
            p->setTraceFile(default_trace_file);
            p->setTraceTable(table);
            p->p_in(*sig);
            additional_objects.push_back(p);
        }
    }
    // TODO: refactor ↓
    auto all_pdets = sc_get_all_object_by_type<Detector>();
    auto all_pwr_meters = sc_get_all_object_by_type<PowerMeter>();
    if (!default_trace_file)
    {
        // the tables only hold the probes (F-05)
        if (!all_pdets.empty() || !all_pwr_meters.empty())
            cerr << "Warning: photodetector and power meter readouts are only traced in the VCD "
                 << "(add --format vcd)" << endl;
        return;
    }
    for (auto &pdet: all_pdets) {
        pdet->trace(default_trace_file);
    }

    for (auto &pwr_meter: all_pwr_meters) {
        pwr_meter->trace(default_trace_file);
    }
}

string SPECSConfig::analysisTypeDesc() const
{
    switch (analysis_type) {
        case CW_OPERATING_POINT:
            return "CW OPERATING POINT";
        case CW_SWEEP:
            return "CW SWEEP";
        case TIME_DOMAIN:
            return "TIME DOMAIN";
        default:
            return "UNDEFINED";
    }
}

void SPECSConfig::printConfig() const
{
    cout << "Current SPECS config: " << endl;
    cout << "- simulation mode: " << oopPortMode2str(simulation_mode) << endl;
    cout << "- analysis type: " << analysisTypeDesc() << endl;
    cout << "- engine timescale (log10 of s): " << engine_timescale << endl;
    cout << "- abstol: " << default_abstol << endl;
    cout << "- default reltol: " << default_reltol << endl;
    cout << "- resolution multiplier: " << default_resolution_multiplier << endl;
    cout << "- trace all optical nets: " << trace_all_optical_nets << endl;
}

// Zero tolerances are allowed (B-153, decision 2026-10-21), e.g. to compare with a fixed-step
// discrete model. With both at 0 nothing is gated: say so once per run (verifyConfig() is called
// from runAnalysis() and from prepareSimulation())
void SPECSConfig::warnZeroTolerances()
{
    if (zero_tolerances_warned || default_abstol != 0 || default_reltol != 0)
        return;
    zero_tolerances_warned = true;
    cerr << "Warning: abstol = 0 and reltol = 0: the emission gate is off, every change of a value is emitted;"
         << " a circuit with feedback may never quiesce in .tran (it stops only when its values repeat exactly),"
         << " and .op/.dc may stop at .options maxiter with a non-convergence error" << endl;
}

void SPECSConfig::printOPAnalysisResult() const
{
    cout << "╔═══════════════════╗" << endl;
    cout << "║  OPERATING POINT  ║" << endl;
    cout << "╠═══════════════════╣" << endl;
    cout << "║                   ╨───┄┄" << endl;
    auto all_optical_sigs = sc_get_all_object_by_type<sc_signal<OpticalSignal, SC_MANY_WRITERS>>();
    for (auto &sig: all_optical_sigs) {
        string signame = sig->name();
        cout << "║ ";
        cout << "- " << signame << " (OPTICAL)" << " = " << sig->read() << endl;
    }
    cout << "║                   ╥───┄┄" << endl;
    cout << "╚═══════════════════╝" << endl;
}

void SPECSConfig::prepareSimulation() {
    auto all_spx_mod = sc_get_all_object_by_type<spx_module>();
    for (auto &mod : all_spx_mod)
        mod->init();
    printConfig();
    verifyConfig();

    { // open default trace_file
        bool opened_default_trace_file = false;
        if (!default_trace_file && trace_filename.size() && trace_vcd)
        {
            default_trace_file = sc_create_vcd_trace_file(trace_filename.c_str());
            opened_default_trace_file = true;
        }

        if (default_trace_file)
            default_trace_file->set_time_unit(std::pow(10, 15 + engine_timescale), SC_FS);

        // tables of the probes (F-05, --format csv|npz)
        bool opened_default_trace_table = false;
        if (!default_trace_table && trace_filename.size() && (trace_csv || trace_npz))
        {
            default_trace_table = std::make_unique<TraceTable>(trace_csv ? trace_filename + ".csv" : "",
                                                               trace_npz ? trace_filename + ".npz" : "");
            opened_default_trace_table = true;
        }

        // Run manifest (F-01): written next to the default trace file opened above, and printed
        // once with --print-manifest (with "trace_file": null if SPECS writes no default trace).
        // Its trace file is the VCD, or the first table (CSV, then .npz) when there is no VCD
        if (opened_default_trace_file || opened_default_trace_table || print_run_manifest)
        {
            // sc_create_vcd_trace_file() appends ".vcd"
            string manifest_trace_file;
            if (opened_default_trace_file)
                manifest_trace_file = trace_filename + ".vcd";
            else if (opened_default_trace_table)
                manifest_trace_file = trace_filename + (trace_csv ? ".csv" : ".npz");
            const string manifest_json = RunManifest(*this, manifest_trace_file).to_json();
            if (opened_default_trace_file || opened_default_trace_table)
                RunManifest::write_file(trace_filename + ".manifest.json", manifest_json);
            if (print_run_manifest)
            {
                cout << manifest_json << flush;
                print_run_manifest = false;
            }
        }
    }

    applyDefaultOpticalOutputPortConfig();
    applyDefaultTraceFileToAllSignals();
    if (default_trace_file)
        sc_trace(default_trace_file, *this, "");

    // sc_start(SC_ZERO_TIME); // ← May have to add this here to handle initial events directly written to device ports (rather than sources)
}

inline void sc_trace(sc_trace_file *tf, const SPECSConfig &s, string parent_tree)
{
    parent_tree += (parent_tree.size() ? "." : "");
    sc_trace(tf, s.default_abstol, parent_tree + "abstol");
    sc_trace(tf, s.default_reltol, parent_tree + "reltol");
    sc_trace(tf, (int&)s.default_resolution_multiplier, parent_tree + "resolution_multiplier");
    sc_trace(tf, (int&)s.engine_timescale, parent_tree + "engine_timescale");
    sc_trace(tf, (int&)s.simulation_mode, parent_tree + "simulation_mode");
    sc_trace(tf, (int&)s.analysis_type, parent_tree + "analysis_type");
    sc_trace(tf, s.trace_all_optical_nets, parent_tree + "trace_all_optical_nets");
    sc_trace(tf, s.verbose_component_initialization, parent_tree + "verbose_component_initialization");
}