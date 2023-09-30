#include "parse_directive.h"
#include "devices/alldevices.h"
#include "directional_coupler.h"
#include "optical_signal.h"
#include "parse_tree.h"
#include "specs.h"

using spx::oa_value_type;
using spx::ea_value_type;
using spx::ed_value_type;

using spx::ed_bus_type;
using spx::oa_signal_type;
using spx::ea_signal_type;
using spx::ed_signal_type;
using spx::ed_bus_type;

void OPTIONSDirective::create() const
{
    // Verify number of nodes
    assert(args.size() == 0);

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "TRACEALL")
            specsGlobalConfig.trace_all_optical_nets = p.second.as_boolean();
        else if (kw == "ABSTOL")
            specsGlobalConfig.default_abstol = p.second.as_double();
        else if (kw == "RELTOL")
            specsGlobalConfig.default_reltol = p.second.as_double();
        else if (kw == "TS" || kw == "TIMESCALE")
            specsGlobalConfig.engine_timescale = (SPECSConfig::EngineTimescale)p.second.as_integer();
        else if (kw == "RESOLUTION")
            specsGlobalConfig.default_resolution_multiplier = p.second.as_double();
        else if (kw == "TRACEALL")
            specsGlobalConfig.trace_all_optical_nets = p.second.as_double();
        else if (kw == "TEST_VARIABLE")
            cout << kw << "(" << p.second.kind() << "): " << p.second.get_str() << endl;
        else {
            cerr << "Unknown keyword: " << p.first;
            cerr << " (value: " << p.second.get_str() << " (" << p.second.kind() << "))" <<endl;
            exit(1);
        }
    }
}

void NODESETDirective::print() const
{
    ParseDirective::print();
    cout << "{";
    for (const auto &x : net_assignments)
    {
        for (const auto &ass : x.second)
        cout << ass.first << "(" << x.first << ") = "
             << ass.second.get_str() << " (" << ass.second.kind() << ") , ";
    }
    if ( !net_assignments.empty() )
        cout << "\b\b";
    cout << "}" << endl;
}

void NODESETDirective::create() const
{
    cerr << "NODESET is disabled pending bugfixes" << endl;
    exit(1);
#if 0
    map<string, pair<spx::oa_signal_type *, oa_value_type>> ic_map;
    auto &signals = parent->circuit_nets;
    for (const auto &x : net_assignments)
    {
        const auto &key = x.first;
        if (signals.count(key) == 0)
        {
            cerr << "Net not found " << key << endl;
            exit(1);
        }
        if (ic_map.count(key) == 0)
        {
            //FIXME:
            auto sig = dynamic_cast<oa_signal_type *>(signals[key].get());
            auto initial_val = sig->read();
            auto sig_address = dynamic_cast<oa_signal_type *>(signals[key].get());
            pair<oa_signal_type *, oa_value_type> assignment{sig_address, initial_val};
            ic_map.emplace(key, assignment);
        }
        oa_value_type signal_val = ic_map[key].second;
        for (const auto &assignment: x.second)
        {
            string attribute_name = assignment.first;
            strutils::toupper(attribute_name);
            const Variable &value = assignment.second;

            // cout << attribute_name << endl;
            if (attribute_name == "P" || attribute_name == "POW" || attribute_name == "POWER")
            {
                signal_val.m_field = polar(sqrt(value.as_double()), arg(signal_val.m_field));
                // cout << "################" << endl;
                // cout << signal_val << endl;
            }
            else if (attribute_name == "PHASE" || attribute_name == "PHI")
            {
                signal_val.m_field = polar(abs(signal_val.m_field), value.as_double());
                // cout << "################" << endl;
                // cout << signal_val << endl;
            }
            else if (attribute_name == "WL" || attribute_name == "LAMBDA" || attribute_name == "WAVELENGTH")
            {
                signal_val.setWavelength(value.as_double());
                // cout << "################" << endl;
                // cout << signal_val << endl;
            }
            else
            {
                cerr << "Unknown attribute" << endl;
                exit(1);
            }
        }
        specsGlobalConfig.nodeset_orders[key] = pair<oa_signal_type *, oa_value_type>(
            dynamic_cast<oa_signal_type *>(signals[key].get())
            , signal_val);
    }
#endif
}

void ICDirective::create() const
{
    cerr << "IC directive is not implemented" << endl;
    exit(1);
}