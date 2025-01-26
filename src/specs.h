#pragma once

#include "utils/sysc_utils.h"
#include "optical_signal.h"
#include "optical_output_port.h"

#include <systemc.h>
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
    map<string, pair<sc_signal<OpticalSignal, SC_MANY_WRITERS> *, OpticalSignal>> ic_orders;
    map<string, pair<sc_signal<OpticalSignal, SC_MANY_WRITERS> *, OpticalSignal>> nodeset_orders;

    // Simulation options
    EngineTimescale engine_timescale = ONE_FS;
    OpticalOutputPortMode simulation_mode;
    AnalysisType analysis_type;
    map<string, pair<function<void(double)>, vector<double>>> cw_sweep_orders;
    double tran_duration = std::numeric_limits<double>::infinity();

    // Port options
    double default_abstol = 1e-8;
    double default_reltol = 1e-4;
    sc_time::value_type default_resolution_multiplier = 1;

    // For multi-wavelength support
    vector<double> wavelengths_vector;

    // Trace options
    string trace_filename = "";
    sc_trace_file *default_trace_file = nullptr;
    bool trace_all_optical_nets = 1;

    // other
    sc_signal<bool, SC_MANY_WRITERS> drop_all_events;
    bool verbose_component_initialization = false;

    SPECSConfig(sc_module_name name);

    ~SPECSConfig() {}

    virtual void before_end_of_elaboration()
    {}

    void runAnalysis();
    void runOPAnalysis();
    void runDCAnalysis();
    void runTRANAnalysis();

    void applyEngineResolution() {
        // set engine time resolution
        sc_set_time_resolution(std::pow(10, 15+engine_timescale), SC_FS);
    }
    void applyDefaultOpticalOutputPortConfig();
    void traceSettings();
    void applyDefaultTraceFileToAllSignals();
    void verifyConfig()
    {
        assert(default_abstol > 0);
        assert(default_reltol > 0);
        assert(default_resolution_multiplier >= 1);
        assert(ONE_FS <= engine_timescale && engine_timescale <= ONE_SEC);
        assert(PORT_MODE_MINVAL < simulation_mode && simulation_mode < PORT_MODE_MAXVAL);
        assert(ANALYSIS_TYPE_MINVAL < analysis_type && analysis_type < ANALYSIS_TYPE_MAXVAL);
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
