#pragma once

#include "parse_tree.h"
#include "specs.h"
#include "strutils.h"

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

/* OP: operating point analysis */
struct OPAnalysis : public ParseAnalysis {
    /* Import constructor from ParseAnalysis */
    using ParseAnalysis::ParseAnalysis;

    virtual ParseAnalysis* clone() const
    { return new OPAnalysis(*this); }
    virtual void create() const;
    virtual string kind() const
    { return "OP"; }
};

/* DC: stepped-source operating point analysis */
struct DCAnalysis : public ParseAnalysis {
    typedef pair<string, string> sweep_param_type;
    typedef vector<double> sweep_range_type;
    typedef pair<sweep_param_type, sweep_range_type> sweep_order_type;

    /* Import constructor from ParseAnalysis */
    using ParseAnalysis::ParseAnalysis;

    map<sweep_param_type, sweep_range_type> sweep_orders;

    void register_sweep_order(sweep_param_type param, sweep_range_type range)
    {
        strutils::toupper(param.first);
        strutils::toupper(param.second);
        if (sweep_orders.count(param) != 0)
        {
            cerr << "A sweep order for " << param.second << "(" << param.first << ")";
            cerr << " was already recoreded." << endl;
            exit(1);
        }
        auto n = (range[1] - range[0]) / range[2];
        n = max(0.0, n);
        n = floor(n);
        if (n <= 1.5)
        {
            cerr << "Sweep order must contain at least 2 point." << endl;
            exit(1);
        }
        cout << "Recorded sweep order for " << param.second << "(" << param.first << ")";
        cout << " (" << n << "points)" << endl;
        sweep_orders.emplace(param, range);
    }

    void register_sweep_order(sweep_order_type order)
    {
        register_sweep_order(order.first, order.second);
    }

    virtual ParseAnalysis* clone() const
    { return new DCAnalysis(*this); }
    virtual void create() const;
    virtual string kind() const
    { return "DC"; }
};

/* TRAN: transient analysis */
struct TRANAnalysis : public ParseAnalysis {
    /* Import constructor from ParseElement */
    using ParseAnalysis::ParseAnalysis;

    virtual ParseAnalysis* clone() const
    { return new TRANAnalysis(*this); }
    virtual void create() const;
    virtual string kind() const
    { return "TRAN"; }
};