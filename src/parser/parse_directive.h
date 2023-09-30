#pragma once

#include "parse_tree.h"

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

/* IC: specify initial conditions options */
// Inherit from NODESETDirective as options are the same.
struct ICDirective : public NODESETDirective {
    /* Import constructor from ParseDirective */
    using NODESETDirective::NODESETDirective;

    virtual ParseDirective* clone() const
    { return new ICDirective(*this); }
    virtual void create() const;
    virtual string kind() const
    { return "IC"; }
};