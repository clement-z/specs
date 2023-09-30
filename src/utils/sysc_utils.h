#pragma once

#include <systemc.h>

#include <vector>
#include <set>

using std::vector;
using std::set;

bool sc_object_is_module(const sc_object &obj);

// Return vector containing all children objects (expanded) of obj
set<sc_object *> sc_collect_children_object(sc_object* obj);

// Return vector containing all children objects (expanded) of obj
set<sc_module *> sc_collect_children_module(sc_module* obj);

// Return vector containing all sc_module registered with engine
set<sc_module *> sc_get_all_module();

// Return vector containing all sc_object registered with engine
set<sc_object *> sc_get_all_object();

// Return vector containing all sc_module of a certain type
template<typename T>
set<T *> sc_get_all_module_by_type();

// Return vector containing all sc_object of a certain type
template<typename T>
set<T *> sc_get_all_object_by_type();

////////////////////////////////////////
// Definition of template functions
////////////////////////////////////////


template<typename T>
set<T *> sc_get_all_module_by_type() {
    set<sc_module *> all_modules = sc_get_all_module();
    set<T *> all_requested;
    for (auto mod : all_modules)
    {
        // cout << mod->name() << endl;
        auto requested = dynamic_cast<T *>(mod);
        if (requested)
            all_requested.insert(requested);
    }
    return all_requested;
}

template<typename T>
set<T *> sc_get_all_object_by_type() {
    set<sc_object *> all_objects = sc_get_all_object();
    set<T *> all_requested;

    for (auto obj : all_objects)
    {
        //cout << obj->name() << endl;
        auto requested = dynamic_cast<T *>(obj);
        if (requested) {
            //cout << "\\__( WE WANT IT )" << endl;
            all_requested.insert(requested);
        }
    }
    return all_requested;
}