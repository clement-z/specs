#pragma once

#include "utils/sysc_utils.h"
#include "optical_signal.h"
#include "optical_output_port.h"

#include <systemc.h>
#include <cstdint>
#include <vector>
#include <map>
#include <utility>
#include <functional>

using std::vector;
using std::map;
using std::pair;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::function;

namespace spx {
    // Value types
    typedef OpticalSignal oa_value_type;
    typedef double ea_value_type;
    typedef sc_logic ed_value_type;

    // Signal types
    typedef sc_signal<oa_value_type, SC_MANY_WRITERS> oa_signal_type;
    typedef sc_signal<ea_value_type, SC_MANY_WRITERS> ea_signal_type;
    typedef sc_signal<ed_value_type, SC_MANY_WRITERS> ed_signal_type;
    typedef sc_signal<sc_lv_base, SC_MANY_WRITERS> ed_bus_type;

    // Optical analog IF type
    typedef sc_signal_in_if<oa_value_type> oa_if_in_type;
    typedef sc_signal_out_if<oa_value_type> oa_if_out_type;
    typedef sc_signal_inout_if<oa_value_type> oa_if_inout_type;

    // Electrical analog IF type
    typedef sc_signal_in_if<ea_value_type> ea_if_in_type;
    typedef sc_signal_out_if<ea_value_type> ea_if_out_type;
    typedef sc_signal_inout_if<ea_value_type> ea_if_inout_type;

    // Optical analog ports
    typedef sc_port<oa_if_in_type, 0, SC_ZERO_OR_MORE_BOUND> oa_port_in_type;
    typedef sc_port<oa_if_out_type> oa_port_out_type;
    typedef sc_port<oa_if_inout_type> oa_port_inout_type;

    // Electrical analog ports
    typedef sc_port<ea_if_in_type, 0, SC_ZERO_OR_MORE_BOUND> ea_port_in_type;
    typedef sc_port<ea_if_out_type> ea_port_out_type;
    typedef sc_port<ea_if_inout_type> ea_port_inout_type;
};

class SPECSConfig : public sc_module {
public:
    enum EngineTimescale {
        ONE_SEC     = 0,
        HUNDRED_MS  = -1,
        TEN_MS      = -2,
        ONE_MS      = -3,
        HUNDRED_US  = -4,
        TEN_US      = -5,
        ONE_US      = -6,
        HUNDRED_NS  = -7,
        TEN_NS      = -8,
        ONE_NS      = -9,
        HUNDRED_PS  = -10,
        TEN_PS      = -11,
        ONE_PS      = -12,
        HUNDRED_FS  = -13,
        TEN_FS      = -14,
        ONE_FS      = -15,
    };

    enum AnalysisType {
        ANALYSIS_TYPE_MINVAL = -1,
        CW_OPERATING_POINT = 0,
        CW_SWEEP = 1,
        TIME_DOMAIN = 2,
        ANALYSIS_TYPE_MAXVAL,

        // aliases
        DEFAULT   = TIME_DOMAIN,
        OP        = CW_OPERATING_POINT,
        DC        = CW_SWEEP,
        TRAN      = TIME_DOMAIN,
        UNDEFINED = ANALYSIS_TYPE_MAXVAL,
    };

    // Hold simulation objects
    vector<shared_ptr<sc_object>> additional_objects;
    map<string, pair<sc_signal<OpticalSignal, SC_MANY_WRITERS> *, OpticalSignal>> nodeset_orders;
    // Initial conditions of a .tran, written after the OP (applyInitialConditions). Only filled
    // from C++ for now: the parser rejects .ic (B-161)
    map<string, pair<sc_signal<OpticalSignal, SC_MANY_WRITERS> *, OpticalSignal>> ic_orders;

    // Simulation options
    EngineTimescale engine_timescale = HUNDRED_FS;
    OpticalOutputPortMode simulation_mode;
    // Set when simulation_mode was chosen explicitly (specs -m): the analyses (.op/.dc/.tran)
    // then keep it instead of applying their default mode (B-004)
    bool simulation_mode_explicit = false;
    AnalysisType analysis_type;
    map<string, pair<function<void(double)>, vector<double>>> cw_sweep_orders;
    double tran_duration = std::numeric_limits<double>::infinity();
    // Master seed of the random streams (F-01): each noisy device draws from
    // spx::rng_for(seed, <its hierarchical name>) (utils/rng.h). Default 0: runs are reproducible
    static constexpr uint64_t DEFAULT_SEED = 0;
    uint64_t seed = DEFAULT_SEED;
    // Set when the seed was chosen explicitly (specs --seed): .options seed is then ignored
    bool seed_explicit = false;

    // Port options
    double default_abstol = 1e-8;
    double default_reltol = 1e-4;
    // values with |E| < default_snap_factor * default_abstol are emitted as 0 (.options snapfactor, B-021)
    double default_snap_factor = 10;
    sc_time::value_type default_resolution_multiplier = 1;

    // For multi-wavelength support
    vector<double> wavelengths_vector;

    // Trace options
    string trace_filename = "";
    sc_trace_file *default_trace_file = nullptr;
    bool trace_all_optical_nets = 1;

    // Run manifest (F-01, src/run_manifest.h), written next to the default trace file
    vector<string> command_line; // argv as received by sc_main
    vector<string> netlist_files; // files opened by the parser (-f, .include), in first-open order
    bool print_run_manifest = false; // also print the run manifest to stdout, once (--print-manifest)

    // other
    // Unused since ws-cleanup C-01; kept because removing it shifts the printed
    // OpticalSignal IDs (heap layout, B-016): delete it together with B-016.
    sc_signal<bool, SC_MANY_WRITERS> drop_all_events;
    bool verbose_component_initialization = false;

    SPECSConfig(sc_module_name name);

    ~SPECSConfig() {}

    void runAnalysis();
    void runOPAnalysis();
    void runDCAnalysis();
    void runTRANAnalysis();
    void applyInitialConditions();

    void applyEngineResolution() {
        // set engine time resolution
        sc_set_time_resolution(std::pow(10, 15+engine_timescale), SC_FS);
    }
    void applyDefaultOpticalOutputPortConfig();
    void applyDefaultTraceFileToAllSignals();
    void verifyConfig()
    {
        // Not asserts: the values come from user input and must also be checked with NDEBUG
        auto invalid = [](const char *what, const auto &value)
        {
            std::cerr << "Error: invalid configuration: " << what << " (got " << value << ")" << std::endl;
            exit(1);
        };
        if (!(default_abstol > 0))
            invalid("abstol must be > 0", default_abstol);
        if (!(default_reltol > 0))
            invalid("reltol must be > 0", default_reltol);
        if (!(default_snap_factor >= 0))
            invalid("snap factor must be >= 0", default_snap_factor);
        if (!(default_resolution_multiplier >= 1))
            invalid("resolution multiplier must be >= 1", default_resolution_multiplier);
        if (!(ONE_FS <= engine_timescale && engine_timescale <= ONE_SEC))
            invalid("timescale must be between -15 (1 fs) and 0 (1 s)", engine_timescale);
        if (!(PORT_MODE_MINVAL < simulation_mode && simulation_mode < PORT_MODE_MAXVAL))
            invalid("unknown simulation mode", simulation_mode);
        // B-002: OpticalOutputPort::delayedWriteSampledTime() is a TODO that drops every write
        if (simulation_mode == SAMPLED_TIME)
            invalid("sampled-time simulation mode is not implemented", simulation_mode);
        if (!(ANALYSIS_TYPE_MINVAL < analysis_type && analysis_type < ANALYSIS_TYPE_MAXVAL))
            invalid("unknown analysis type", analysis_type);
    }
    string analysisTypeDesc() const;
    void printConfig() const;
    void printOPAnalysisResult() const;
    void prepareSimulation();
    inline void register_object(shared_ptr<sc_object> object) {
        additional_objects.push_back(object);
    }

    inline friend void
    sc_trace(sc_trace_file *tf, const SPECSConfig &s, string parent_tree);
};

extern SPECSConfig specsGlobalConfig;
