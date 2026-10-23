#include "parser/parse_functions.h"

#include <cmath>
#include <limits>
#include <sstream>

#include "utils/rng.h"

namespace {

typedef double (*math_function_type)(double);

const map<string, math_function_type> &math_functions()
{
    static const map<string, math_function_type> functions = {
        { "sqrt",  [](double x) { return std::sqrt(x); } },
        { "exp",   [](double x) { return std::exp(x); } },
        { "log",   [](double x) { return std::log(x); } },
        { "log10", [](double x) { return std::log10(x); } },
        { "sin",   [](double x) { return std::sin(x); } },
        { "cos",   [](double x) { return std::cos(x); } },
        { "tan",   [](double x) { return std::tan(x); } },
        { "asin",  [](double x) { return std::asin(x); } },
        { "acos",  [](double x) { return std::acos(x); } },
        { "atan",  [](double x) { return std::atan(x); } },
        { "abs",   [](double x) { return std::fabs(x); } },
        { "floor", [](double x) { return std::floor(x); } },
        { "ceil",  [](double x) { return std::ceil(x); } },
    };
    return functions;
}

string call_str(const string &name, const FunctionArgs &fargs)
{
    std::stringstream ss;
    ss << name << "(";
    bool first = true;
    for (const auto &a : fargs.args)
    {
        ss << (first ? "" : ", ") << a.get_str();
        first = false;
    }
    for (const auto &kw : fargs.kwargs)
    {
        ss << (first ? "" : ", ") << kw.first << "=" << kw.second.get_str();
        first = false;
    }
    ss << ")";
    return ss.str();
}

// The numbers of fargs.args, false if one is not a number
bool number_args(const FunctionArgs &fargs, vector<double> &values)
{
    values.clear();
    for (const auto &a : fargs.args)
    {
        if (!a.is_number())
            return false;
        values.push_back(a.as_double());
    }
    return true;
}

}

bool is_random_function(const string &name)
{
    return name == "gauss" || name == "uniform" || name == "lognormal" || name == "choice";
}

bool call_function(ParseTree *pt, const string &name, const FunctionArgs &fargs, const string &where,
                   Variable &result, string &error)
{
    string fname = name;
    strutils::lowercase(fname);
    const string call = call_str(fname, fargs);

    const auto it_math = math_functions().find(fname);
    const bool is_min_max = fname == "min" || fname == "max";
    if (it_math != math_functions().end() || is_min_max)
    {
        vector<double> x;
        if (!fargs.kwargs.empty())
        {
            error = fname + "() has no keyword arguments: " + call;
            return false;
        }
        if (!number_args(fargs, x))
        {
            error = fname + "() takes numbers: " + call;
            return false;
        }
        double value = 0;
        if (is_min_max)
        {
            if (x.empty())
            {
                error = fname + "() takes at least one argument: " + call;
                return false;
            }
            value = x[0];
            for (double v : x)
                value = fname == "min" ? std::min(value, v) : std::max(value, v);
        }
        else
        {
            if (x.size() != 1)
            {
                error = fname + "() takes one argument: " + call;
                return false;
            }
            value = it_math->second(x[0]);
        }
        if (!std::isfinite(value))
        {
            error = call + " is not a finite number";
            return false;
        }
        result = Variable(value);
        return true;
    }

    if (!is_random_function(fname))
    {
        error = "unknown function " + name + "()";
        return false;
    }

    // shared=1 / shared="name"
    int shared = 0;
    string shared_name;
    for (const auto &kw : fargs.kwargs)
    {
        string key = kw.first;
        strutils::lowercase(key);
        if (key != "shared")
        {
            error = fname + "(): unknown keyword " + kw.first + " (expected shared): " + call;
            return false;
        }
        if (kw.second.type == Variable::STRING)
        {
            shared_name = kw.second.str;
            if (shared_name.empty())
            {
                error = fname + "(): shared=\"\" (expected a name, or 0/1): " + call;
                return false;
            }
            shared = 2;
        }
        else if (kw.second.is_number())
            shared = kw.second.as_boolean() ? 1 : 0;
        else
        {
            error = fname + "(): shared must be 0, 1 or a quoted name: " + call;
            return false;
        }
    }
    if (pt->draws_forbidden)
    {
        error = call + ": random functions cannot be used in the parameter defaults of a .subckt line "
                "(they would be drawn once for every instance); draw in the body (.param) or on the "
                "instance line";
        return false;
    }

    // the choices of choice(): its arguments, or the elements of a single list argument
    vector<Variable> choices = fargs.args;
    vector<double> x;
    if (fname == "choice")
    {
        if (choices.size() == 1 && choices[0].is_list())
            choices = vector<Variable>(choices[0].vec);
        if (choices.empty())
        {
            error = "choice() takes at least one value: " + call;
            return false;
        }
        for (const auto &c : choices)
        {
            if (c.is_none() || c.is_list())
            {
                error = "choice() takes values (numbers or strings): " + call;
                return false;
            }
        }
    }
    else
    {
        if (fargs.args.size() != 2 || !number_args(fargs, x) || !std::isfinite(x[0]) || !std::isfinite(x[1]))
        {
            error = fname + "() takes two finite numbers: " + call;
            return false;
        }
        if (fname == "uniform" && !(x[0] <= x[1]))
        {
            error = "uniform(a, b) needs a <= b: " + call;
            return false;
        }
        if (fname != "uniform" && !(x[1] >= 0))
        {
            error = fname + "(mu, sigma) needs sigma >= 0: " + call;
            return false;
        }
    }

    const string stream = pt->next_draw_stream(shared, shared_name);
    const uint64_t seed = specsGlobalConfig.seed;
    spx::Rng rng = spx::rng_for(seed, stream);
    if (fname == "gauss")
        result = Variable(x[0] + x[1] * rng.normal());
    else if (fname == "lognormal")
        result = Variable(std::exp(x[0] + x[1] * rng.normal()));
    else if (fname == "uniform")
        result = Variable(x[0] + (x[1] - x[0]) * rng.uniform());
    else
    {
        // uniform() < 1, so the index is < size (for any size below 2^53)
        const size_t i = static_cast<size_t>(rng.uniform() * static_cast<double>(choices.size()));
        result = choices[i];
    }
    if (result.is_number() && !std::isfinite(result.as_double()))
    {
        error = call + " drew a value that is not a finite number";
        return false;
    }
    pt->record_draw(stream, result.is_number() ? result.as_double() : std::numeric_limits<double>::quiet_NaN(),
                    seed, where);
    return true;
}
