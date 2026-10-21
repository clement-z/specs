#include "parser/parse_directive.h"
#include "devices/alldevices.h"
#include "devices/directional_coupler.h"
#include "optical_signal.h"
#include "parser/parse_tree.h"
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
    if (args.size() > 0)
    {
        cerr << "Error: .options: too many positional arguments (" << args.size() << ", expected at most 0)" << endl;
        exit(1);
    }

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "TRACEALL")
            specsGlobalConfig.trace_all_optical_nets = p.second.as_boolean();
        else if (kw == "ABSTOL")
        {
            const double v = p.second.as_double();
            if (!(v > 0))
            {
                cerr << "Error: .options: " << kw << " must be > 0 (got " << v << ")" << endl;
                exit(1);
            }
            specsGlobalConfig.default_abstol = v;
        }
        else if (kw == "RELTOL")
        {
            const double v = p.second.as_double();
            if (!(v > 0))
            {
                cerr << "Error: .options: " << kw << " must be > 0 (got " << v << ")" << endl;
                exit(1);
            }
            specsGlobalConfig.default_reltol = v;
        }
        else if (kw == "SNAPFACTOR")
        {
            // B-021: emitted values with |E| < snapfactor * abstol become 0 (0 disables it)
            const double v = p.second.as_double();
            if (!(v >= 0))
            {
                cerr << "Error: .options: " << kw << " must be >= 0 (got " << v << ")" << endl;
                exit(1);
            }
            specsGlobalConfig.default_snap_factor = v;
        }
        else if (kw == "TS" || kw == "TIMESCALE")
        {
            // as_integer() truncates toward zero: check the truncated value
            // as a double first (an out-of-range double-to-int cast is UB)
            const double v = std::trunc(p.second.as_double());
            if (!(v >= static_cast<int>(SPECSConfig::ONE_FS) && v <= static_cast<int>(SPECSConfig::ONE_SEC)))
            {
                cerr << "Error: .options: " << kw << " must be between " << SPECSConfig::ONE_FS
                     << " and " << SPECSConfig::ONE_SEC << " (got " << p.second.as_double() << ")" << endl;
                exit(1);
            }
            specsGlobalConfig.engine_timescale = (SPECSConfig::EngineTimescale)p.second.as_integer();
        }
        else if (kw == "RESOLUTION")
        {
            // checked before the conversion to an unsigned 64-bit integer
            const double v = p.second.as_double();
            if (!(v >= 1 && v < 18446744073709551616.0))
            {
                cerr << "Error: .options: " << kw << " must be >= 1 and < 2^64 (got " << v << ")" << endl;
                exit(1);
            }
            specsGlobalConfig.default_resolution_multiplier = v;
        }
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
}
