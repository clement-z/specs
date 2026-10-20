#pragma once

#include "optical_signal.h"
#include "specs.h"

#include <systemc.h>
#include <string>

using std::string;
using namespace std::string_literals;

inline std::string spx_child_name(const sc_core::sc_object *o, const std::string &sep_suffix, size_t idx)
{
    return std::string(o->name()) + sep_suffix + std::to_string(idx);
}

class spx_module : public sc_module {
public:
    typedef spx_module this_type;
    typedef spx::oa_value_type sigval_type;
    typedef spx::oa_signal_type sig_type;
    typedef spx::oa_if_in_type if_in_type;
    typedef spx::oa_if_out_type if_out_type;
    typedef spx::oa_port_in_type port_in_type;
    typedef spx::oa_port_out_type port_out_type;

    virtual void init() {}
    virtual string describe() const { return ""s; }

    spx_module(sc_module_name name)
    : sc_module(name)
    {}
};