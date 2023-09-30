#include <sysc_utils.h>

#include <string>


inline bool sc_object_is_module(const sc_object &obj) {
    return strcmp(obj.kind(), "sc_module") == 0;
}

// Return vector containing all children objects (expanded) of obj
set<sc_object *> sc_collect_children_object(sc_object* obj)
{
    auto v = obj->get_child_objects();
    auto children = set<sc_object *>(v.begin(), v.end());
    auto all_children = children;
    for (auto child : children)
        if ( child )
        {
            auto children_children = sc_collect_children_object(child);
            for (auto child_child : children_children)
                all_children.insert(child_child);
        }
    return all_children;
}

// Return vector containing all children objects (expanded) of obj
set<sc_module *> sc_collect_children_module(sc_module* obj)
{
    auto v = obj->get_child_objects();
    auto children = set<sc_object *>(v.begin(), v.end());
    set<sc_module *> all_children;
    for (auto obj: children)
    {
        auto mod = dynamic_cast<sc_module *>(obj);
        if(mod) {
            all_children.insert(mod);
            auto children2 = sc_collect_children_module(mod);
            all_children.insert(children2.begin(), children2.end());
        }
    }
    return all_children;
}

set<sc_module *> sc_get_all_module() {
    set<sc_module *> all_modules;

    for ( auto obj : sc_get_top_level_objects() ) {
        auto mod = dynamic_cast<sc_module *>(obj);
        if (mod)
        {
            all_modules.insert(mod);
            auto all_children = sc_collect_children_module(mod);
            for (auto child : all_children)
                all_modules.insert(child);
        }
    }
    return all_modules;
}

set<sc_object *> sc_get_all_object() {
    set<sc_object *> all_objects;

    for ( auto obj : sc_get_top_level_objects() ) {
        all_objects.insert(obj);
        auto all_children = sc_collect_children_object(obj);
        for (auto child : all_children)
            all_objects.insert(child);
    }
    return all_objects;
}