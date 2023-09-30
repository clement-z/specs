#pragma once

#include "optical_signal.h"
#include "specs.h"

#include <systemc.h>
#include <string>

using std::string;
using namespace std::string_literals;

class spx_module : public sc_module {
public:
    typedef spx_module this_type;
    typedef spx::oa_value_type sigval_type;
    typedef spx::oa_signal_type sig_type;
    typedef spx::oa_if_in_type if_in_type;
    typedef spx::oa_if_out_type if_out_type;
    typedef spx::oa_port_in_type port_in_type;
    typedef spx::oa_port_out_type port_out_type;

    enum ModuleFlags {
        NON_LINEAR = 0b0001,
        TIME_VARIANT = 0b0010,
        FREQUENCY_DEPENDENT = 0b0100,
    };

    ModuleFlags flags = FREQUENCY_DEPENDENT;

    virtual void init() {}
    virtual string describe() const { return ""s; }

    spx_module(sc_module_name name)
    : sc_module(name)
    {}
};