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

// Value of an .options keyword stored as an unsigned 64-bit integer (seed, maxiter). An integer
// literal is exact over its whole range, 0 to 2^63 - 1 (B-180); other netlist numbers are doubles,
// whose integers are exact up to 2^53 only.
static uint64_t unsigned_integer_option(const string &kw, const Variable &val)
{
    if (val.type == Variable::INTEGER)
    {
        const int64_t i = val.as_integer();
        if (i < 0)
        {
            cerr << "Error: .options: " << kw << " must be an integer between 0 and "
                 << std::numeric_limits<int64_t>::max() << " (got " << i << ")" << endl;
            exit(1);
        }
        return static_cast<uint64_t>(i);
    }
    const double v = val.as_double();
    if (!(v >= 0 && v <= 9007199254740992.0 && v == std::trunc(v)))
    {
        cerr << "Error: .options: " << kw << " must be an integer between 0 and 2^53 (got " << v << ")" << endl;
        exit(1);
    }
    return static_cast<uint64_t>(v);
}

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
        {
            specsGlobalConfig.trace_all_optical_nets = p.second.as_boolean();
            specsGlobalConfig.trace_all_optical_nets_explicit = true;
        }
        else if (kw == "ABSTOL")
        {
            // 0 is allowed (B-153, decision 2026-10-21; with reltol = 0 too: no emission gate)
            const double v = p.second.as_double();
            if (!(v >= 0))
            {
                cerr << "Error: .options: " << kw << " must be >= 0 (got " << v << ")" << endl;
                exit(1);
            }
            specsGlobalConfig.default_abstol = v;
        }
        else if (kw == "RELTOL")
        {
            // 0 is allowed, as for ABSTOL
            const double v = p.second.as_double();
            if (!(v >= 0))
            {
                cerr << "Error: .options: " << kw << " must be >= 0 (got " << v << ")" << endl;
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
        else if (kw == "SEED")
        {
            // F-01: master seed of the random streams (specs --seed accepts any 64-bit value)
            const uint64_t seed = unsigned_integer_option(kw, p.second);
            // specs --seed takes priority over the netlist
            if (!specsGlobalConfig.seed_explicit)
                specsGlobalConfig.seed = seed;
        }
        else if (kw == "MAXITER")
        {
            // F-28: delta cycles allowed per .op/.dc fixed point (0: no limit)
            const uint64_t max_iterations = unsigned_integer_option(kw, p.second);
            // specs --maxiter takes priority over the netlist
            if (!specsGlobalConfig.max_iterations_explicit)
                specsGlobalConfig.max_iterations = max_iterations;
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

SAVEDirective::Quantity SAVEDirective::quantity(string name)
{
    strutils::toupper(name);
    if (name == "POWER" || name == "POW" || name == "P")
        return POWER;
    if (name == "MAGNITUDE" || name == "MAG" || name == "MODULUS" || name == "MOD" || name == "E")
        return MODULUS;
    if (name == "PHASE" || name == "PHI")
        return PHASE;
    if (name == "WAVELENGTH" || name == "WL")
        return WAVELENGTH;
    return UNKNOWN;
}

void SAVEDirective::add_target(const pair<string, string> &target, const string &prefix)
{
    Target t;
    t.net = prefix + target.second;
    t.net_written = target.second;
    t.written = target.first.empty() ? target.second : target.first + "(" + target.second + ")";
    t.quantity = target.first.empty() ? ALL : quantity(target.first);
    targets.push_back(t);
    // also as positional arguments, for print() and the JSON export
    args.emplace_back(Variable(t.written));
}
