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

    // Port options
    default_abstol = 1e-8;
    default_reltol = 1e-4;
    default_resolution_multiplier = 1;

    // Trace options
    trace_filename = "traces/delete_me";
    default_trace_file = nullptr;
    trace_all_optical_nets = 1;
}

void SPECSConfig::runAnalysis()
{
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

    // Disable photodetectors
    for (auto pdet: all_photodetectors) {
        pdet->enable = sc_logic(0);
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

    // Enable photodetectors
    for (auto pdet: all_photodetectors) {
        // Remark: Photodetectors could in fact be left enabled ?
        // Would that serve any purpose though...?
        pdet->enable = sc_logic(0);
    }

    // Set all ports mode to NO_DELAY
    for (auto oop: all_oop) {
        oop->m_mode = OpticalOutputPortMode::NO_DELAY;
    }

    // Activate CW sources
    for (auto cws: all_cws) {
        cws->enable = sc_logic(1);
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
            #if 1 // reset OOP
            oop->reset();
            // TODO: also reset photodetectors?
            #elif 0 // remove previous wavelengths from OOP but keep value
            if (wavelengths_vector.size() >= 2)
            {
                oop->swap_wavelengths(wavelengths_vector.size()-1, wavelengths_vector.size()-2);
                oop->delete_wavelength(wavelengths_vector.size()-2);
                oop->m_skip_next_convergence_check = true;
            }
            #else
            // do nothing
            #endif
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

    // Run OP analysis
    runOPAnalysis();

    // Set values of signals according to IC directive
    for (auto &ic_order : ic_orders)
    {
        //ic_order.second.first->write(ic_order.second.second);
    }

    // Enable probes
    for (auto probe: all_probes) {
        probe->enable = sc_logic(1);
    }

    // Enable ML probes
    for (auto mlprobe: all_mlprobes) {
        mlprobe->enable = sc_logic(1);
    }

    // Enable photodetectors
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

void SPECSConfig::applyDefaultOpticalOutputPortConfig() {
    //assert(!oop_configs.empty());
    auto oop_default_config = make_shared<OpticalOutputPortConfig>();
    oop_default_config->m_mode = simulation_mode;
    oop_default_config->m_abstol = default_abstol;
    oop_default_config->m_reltol = default_reltol;
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

    // auto all_mlprobes = sc_get_all_object_by_type<MLambdaProbe>();
    // for (auto p: all_mlprobes) {
    //     p->m_Tf = default_trace_file;
    //     p->prepare();
    // }

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
        string detname = pdet->name();
        cout << detname << endl;
        pdet->trace(default_trace_file);
    }

    auto all_pwr_meters = sc_get_all_object_by_type<PowerMeter>();
    for (auto &pwr_meter: all_pwr_meters) {
        string pwr_meter_name = pwr_meter->name();
        cout << pwr_meter_name << endl;
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
        if (!default_trace_file && trace_filename.size())
            default_trace_file = sc_create_vcd_trace_file(trace_filename.c_str());

        default_trace_file->set_time_unit(std::pow(10, 15 + engine_timescale), SC_FS);
    }

    applyDefaultOpticalOutputPortConfig();
    applyDefaultTraceFileToAllSignals();
    if (default_trace_file)
        sc_trace(default_trace_file, *this, "");
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