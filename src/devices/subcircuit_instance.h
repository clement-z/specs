#pragma once

#include <memory>
#include <vector>

#include "specs.h"
#include "devices/spx_module.h"

using std::vector;
using std::string;
using std::shared_ptr;

class SubcircuitInstance: public spx_module {
public:
    // vector<shared_ptr<spx::oa_port_inout_type>> ports;
    vector<shared_ptr<sc_object>> signals;
    vector<shared_ptr<sc_object>> modules;

    SubcircuitInstance(sc_module_name name)
    : spx_module(name)
    {}

    ~SubcircuitInstance()
    {}
};