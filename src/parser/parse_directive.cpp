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
        {
            specsGlobalConfig.trace_all_optical_nets = p.second.as_boolean();
            specsGlobalConfig.trace_all_optical_nets_explicit = true;
        }
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
        else if (kw == "SEED")
        {
            // F-01: master seed of the random streams. Netlist numbers are doubles: only the
            // integers up to 2^53 are exact (specs --seed accepts any 64-bit value)
            if (p.second.type == Variable::INTEGER && p.second.as_integer() == std::numeric_limits<int>::max())
            {
                // the lexer reads a digit string beyond the int range as INT_MAX, silently: such
                // seeds would all give the same run
                cerr << "Error: .options: " << kw << "=" << p.second.as_integer()
                     << ": integer literals from 2147483647 up are read as 2147483647;"
                     << " write the seed in exponent form (e.g. 3e9) or use specs --seed" << endl;
                exit(1);
            }
            const double v = p.second.as_double();
            if (!(v >= 0 && v <= 9007199254740992.0 && v == std::trunc(v)))
            {
                cerr << "Error: .options: " << kw << " must be an integer between 0 and 2^53 (got " << v << ")" << endl;
                exit(1);
            }
            // specs --seed takes priority over the netlist
            if (!specsGlobalConfig.seed_explicit)
                specsGlobalConfig.seed = static_cast<uint64_t>(v);
        }
        else if (kw == "MAXITER")
        {
            // F-28: delta cycles allowed per .op/.dc fixed point (0: no limit). Netlist numbers
            // are doubles: only the integers up to 2^53 are exact
            const double v = p.second.as_double();
            if (!(v >= 0 && v <= 9007199254740992.0 && v == std::trunc(v)))
            {
                cerr << "Error: .options: " << kw << " must be an integer between 0 and 2^53 (got " << v << ")" << endl;
                exit(1);
            }
            // specs --maxiter takes priority over the netlist
            if (!specsGlobalConfig.max_iterations_explicit)
                specsGlobalConfig.max_iterations = static_cast<uint64_t>(v);
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
