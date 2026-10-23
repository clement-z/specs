#pragma once

#include "parser/parse_tree.h"

// Functions of netlist expressions (F-02): name(arg, ..., kw=value, ...), names case-insensitive.
//   math:   sqrt exp log log10 sin cos tan asin acos atan abs floor ceil (one argument), min max (one or
//           more); the result must be finite
//   random: gauss(mu, sigma), uniform(a, b), lognormal(mu, sigma) (exp of gauss(mu, sigma)),
//           choice(v1, v2, ...) or choice([v1, v2, ...]), with an optional keyword shared=1 (one draw
//           for every instance of the subcircuit) or shared="name" (one draw for every use of the name)
// Expressions are evaluated when their line is parsed, so a random function draws once per
// evaluation: once per instance in a subcircuit body (each instance's body is parsed on its own).
// Each draw takes its own stream, spx::rng_for(specsGlobalConfig.seed, <stream name>), stream names as
// in ParseTree::next_draw_stream(), and is recorded in the root tree (ParseTree::draws).

struct FunctionArgs {
    vector<Variable> args;
    map<string, Variable> kwargs;
};

// Value of the call name(fargs) in the tree pt (random functions draw from pt's streams), at `where`
// (file:line, for the records of the draws). Returns false with a message in `error` if the call is
// invalid.
bool call_function(ParseTree *pt, const string &name, const FunctionArgs &fargs, const string &where,
                   Variable &result, string &error);

// The random functions (lower case)
bool is_random_function(const string &name);
