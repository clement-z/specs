#pragma once

#include "parser/parse_tree.h"

#include <initializer_list>
#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iostream>

using std::pair;
using std::make_pair;
using std::string;
using std::vector;
using std::map;
using std::cout;
using std::cerr;
using std::endl;
using std::pair;
using std::unique_ptr;

// Value of an .options keyword stored as an unsigned 64-bit integer (seed, maxiter, maxevents): false if
// val is not one (OPTIONSDirective::create() reports it)
bool options_unsigned_integer(const Variable &val, uint64_t &result);

/* OPTIONS: specify simulator options */
struct OPTIONSDirective : public ParseDirective {
    /* Import constructor from ParseDirective */
    using ParseDirective::ParseDirective;

    virtual ParseDirective* clone() const
    { return new OPTIONSDirective(*this); }
    virtual void create() const;
    virtual string kind() const
    { return "OPTIONS"; }
};

/* NODESET: specify initial guesses  */
struct NODESETDirective : public ParseDirective {
    typedef string net_name;
    typedef string property_name;
    typedef Variable property_val;
    typedef pair<property_name, property_val> assignment;

    map<net_name, vector<assignment>> net_assignments;

    /* Import constructor from ParseDirective */
    using ParseDirective::ParseDirective;

    virtual void print() const;

    virtual ParseDirective* clone() const
    { return new NODESETDirective(*this); }
    virtual void create() const;
    virtual string kind() const
    { return "NODESET"; }
};

/* SAVE: trace the listed optical nets (F-05). ParseTree::build_circuit() creates the probes, one per
   signal of each net (two on a bidirectional net), named and traced like the automatic probes
   (PROBE{<signal>}); the targets on one net share its probe */
struct SAVEDirective : public ParseDirective {
    enum Quantity { ALL, POWER, MODULUS, PHASE, WAVELENGTH, UNKNOWN };

    struct Target {
        string net;         // full net name (with the prefix of the parse tree)
        string net_written; // net name as written in the netlist
        string written;     // target as written: net or QTY(net)
        Quantity quantity;
    };
    vector<Target> targets;

    /* Import constructor from ParseDirective */
    using ParseDirective::ParseDirective;

    // Quantity of a QTY(net) target: the keywords of PROBE (case-insensitive), UNKNOWN otherwise
    static Quantity quantity(string name);
    // (QTY, net) as parsed (QTY empty for a net alone); prefix: name prefix of the parse tree
    void add_target(const pair<string, string> &target, const string &prefix);

    virtual ParseDirective* clone() const
    { return new SAVEDirective(*this); }
    // Nothing to do here: the probes need the elaborated nets (ParseTree::build_circuit())
    virtual void create() const {}
    virtual string kind() const
    { return "SAVE"; }
};
