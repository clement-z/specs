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
#include "devices/generic_transmission_device.h"
#include "run_manifest.h"

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

void SPECSConfig::runAnalysis()
{
    verifyConfig();
    applyEngineResolution();
    prepareSimulation();

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
}

void SPECSConfig::runOPAnalysis()
{
    auto all_probes = sc_get_all_module_by_type<Probe>();
    auto all_mlprobes = sc_get_all_module_by_type<MLambdaProbe>();
    auto all_photodetectors = sc_get_all_module_by_type<Detector>();
    auto all_oop = sc_get_all_module_by_type<OpticalOutputPort>();
    auto all_cws = sc_get_all_module_by_type<CWSource>();

    // Disable photodetectors before the first sc_start(): they are enabled by default
    // (testbenches, C++ users) but must not write their readout during the OP
    for (auto pdet: all_photodetectors) {
        pdet->enable = sc_logic(0);
    }

    // Run to initialize all threads and register first values
    sc_start();

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

    // Run operating point simulation
    sc_start();

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

    // Disable photodetectors before the first sc_start() (they are enabled by default)
    for (auto pdet: all_photodetectors) {
        // Remark: Photodetectors could in fact be left enabled ?
        // Would that serve any purpose though...?
        pdet->enable = sc_logic(0);
    }

    // Run to initialize all threads and register first values
    sc_start();

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
    wavelengths_vector.reserve(order.second.second.size());
    cout << "Starting sweep on " << order.first;
    cout << " (" << order.second.second.size() << " points)" << endl;
    for (const auto &val : order.second.second)
    {
        // Apply sweep param
        order.second.first(val);

        //cout << order.first << " = " << val << endl;

        // Reset OOP and set internal signals to new wavelength
        for (auto oop: all_oop) {
            oop->reset();
            // TODO: also reset photodetectors?
        }

        // run simulation and advance one tick
        sc_start(sc_time::from_value(1));

        // Reset CW sources
        for (auto cws: all_cws)
        {
            cws->reset.write(sc_logic(1));
        }

        //printOPAnalysisResult();
    }
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

    cout << "Simulated " << sc_time_stamp() << endl;
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

    // apply default config to all optical output ports which don't have one
    auto all_oop = sc_get_all_module_by_type<OpticalOutputPort>();
    for (auto oop: all_oop) {
        if(!oop->getConfig().get())
            oop->setConfig(oop_default_config);
        // FIXME: only works for first call to sc_start !!
    }
}

void SPECSConfig::applyDefaultTraceFileToAllSignals() {
    if (!default_trace_file)
        return;

    auto all_probes = sc_get_all_object_by_type<Probe>();
    for (auto p: all_probes) {
        p->setTraceFile(default_trace_file);
    }

    auto all_mlambda_probes = sc_get_all_object_by_type<MLambdaProbe>();
    for (auto p: all_mlambda_probes) {
        p->setTraceFile(default_trace_file);
    }

    if (trace_all_optical_nets)
    {
        auto all_optical_sigs = sc_get_all_object_by_type<sc_signal<OpticalSignal, SC_MANY_WRITERS>>();
        for (auto &sig: all_optical_sigs) {
            string signame = sig->name();

            // Check if the signal is explicitly marked as uninteresting
            stringstream ss (signame);
            string base_signame;
            while (getline (ss, base_signame, '/')) {}
            if (base_signame.empty() || base_signame[0] == '_')
                continue;
            //cout << signame << endl;

            auto p = make_shared<Probe>(("PROBE{" + signame + "}").c_str(), true, true, true, true);
            p->setTraceFile(default_trace_file);
            p->p_in(*sig);
            additional_objects.push_back(p);
        }
    }
    // TODO: refactor ↓
    auto all_pdets = sc_get_all_object_by_type<Detector>();
    for (auto &pdet: all_pdets) {
        pdet->trace(default_trace_file);
    }

    auto all_pwr_meters = sc_get_all_object_by_type<PowerMeter>();
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
        if (!default_trace_file && trace_filename.size())
        {
            default_trace_file = sc_create_vcd_trace_file(trace_filename.c_str());
            opened_default_trace_file = true;
        }

        if (default_trace_file)
            default_trace_file->set_time_unit(std::pow(10, 15 + engine_timescale), SC_FS);

        // Run manifest (F-01): written next to the default trace file opened above, and printed
        // once with --print-manifest (with "trace_file": null if SPECS writes no default trace)
        if (opened_default_trace_file || print_run_manifest)
        {
            // sc_create_vcd_trace_file() appends ".vcd"
            const string manifest_trace_file = opened_default_trace_file ? trace_filename + ".vcd" : "";
            const string manifest_json = RunManifest(*this, manifest_trace_file).to_json();
            if (opened_default_trace_file)
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