#pragma once

#include <systemc.h>

#include <algorithm>
#include <cstring>
#include <vector>
#include <set>

using std::vector;
using std::set;

// Name order: everything that iterates these collections (init() calls, the OP listing,
// auto-probe creation, VCD declaration order, ...) follows the hierarchical names, not heap
// addresses (B-016). SystemC names are unique; equal names would keep the hierarchy order.
inline bool sc_object_name_less(const sc_object *a, const sc_object *b)
{
    return std::strcmp(a->name(), b->name()) < 0;
}

// Return vector containing all children objects (expanded) of obj, in hierarchy order
// (each object followed by its children, in creation order)
vector<sc_object *> sc_collect_children_object(sc_object* obj);

// Return vector containing all children modules (expanded) of obj, in hierarchy order
vector<sc_module *> sc_collect_children_module(sc_module* obj);

// Return vector containing all sc_module registered with engine, in name order
vector<sc_module *> sc_get_all_module();

// Return vector containing all sc_object registered with engine, in name order
vector<sc_object *> sc_get_all_object();

// Return vector containing all sc_module of a certain type, in name order
template<typename T>
vector<T *> sc_get_all_module_by_type();

// Return vector containing all sc_object of a certain type, in name order
template<typename T>
vector<T *> sc_get_all_object_by_type();

////////////////////////////////////////
// Definition of template functions
////////////////////////////////////////


template<typename T>
vector<T *> sc_get_all_module_by_type() {
    vector<sc_module *> all_modules = sc_get_all_module();
    vector<T *> all_requested;
    for (auto mod : all_modules)
    {
        // cout << mod->name() << endl;
        auto requested = dynamic_cast<T *>(mod);
        if (requested)
            all_requested.push_back(requested);
    }
    return all_requested;
}

template<typename T>
vector<T *> sc_get_all_object_by_type() {
    vector<sc_object *> all_objects = sc_get_all_object();
    vector<T *> all_requested;

    for (auto obj : all_objects)
    {
        //cout << obj->name() << endl;
        auto requested = dynamic_cast<T *>(obj);
        if (requested) {
            //cout << "\\__( WE WANT IT )" << endl;
            all_requested.push_back(requested);
        }
    }
    return all_requested;
}

// Name of the channel bound to a port, or "<unbound>" (--vci printing).
// The channel need not be an spx::oa/ea_signal_type (e.g. sc_signal<OpticalSignal> in CROW).
template<typename P>
inline const char *port_channel_name(P &port)
{
    auto *obj = dynamic_cast<const sc_object *>(port.get_interface());
    return obj ? obj->name() : "<unbound>";
}
