#include "utils/sysc_utils.h"

#include <string>


// Return vector containing all children objects (expanded) of obj, in hierarchy order
vector<sc_object *> sc_collect_children_object(sc_object* obj)
{
    vector<sc_object *> all_children;
    for (auto child : obj->get_child_objects())
        if ( child )
        {
            all_children.push_back(child);
            auto children_children = sc_collect_children_object(child);
            all_children.insert(all_children.end(), children_children.begin(), children_children.end());
        }
    return all_children;
}

// Return vector containing all children modules (expanded) of obj, in hierarchy order
vector<sc_module *> sc_collect_children_module(sc_module* obj)
{
    vector<sc_module *> all_children;
    for (auto child: obj->get_child_objects())
    {
        auto mod = dynamic_cast<sc_module *>(child);
        if(mod) {
            all_children.push_back(mod);
            auto children2 = sc_collect_children_module(mod);
            all_children.insert(all_children.end(), children2.begin(), children2.end());
        }
    }
    return all_children;
}

vector<sc_module *> sc_get_all_module() {
    vector<sc_module *> all_modules;

    for ( auto obj : sc_get_top_level_objects() ) {
        auto mod = dynamic_cast<sc_module *>(obj);
        if (mod)
        {
            all_modules.push_back(mod);
            auto all_children = sc_collect_children_module(mod);
            all_modules.insert(all_modules.end(), all_children.begin(), all_children.end());
        }
    }
    std::stable_sort(all_modules.begin(), all_modules.end(), sc_object_name_less);
    return all_modules;
}

vector<sc_object *> sc_get_all_object() {
    vector<sc_object *> all_objects;

    for ( auto obj : sc_get_top_level_objects() ) {
        all_objects.push_back(obj);
        auto all_children = sc_collect_children_object(obj);
        all_objects.insert(all_objects.end(), all_children.begin(), all_children.end());
    }
    std::stable_sort(all_objects.begin(), all_objects.end(), sc_object_name_less);
    return all_objects;
}
