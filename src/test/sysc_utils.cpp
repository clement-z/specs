#include <gtest/gtest.h>
#include <systemc.h>

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "utils/sysc_utils.h"

#include "test/utils.h"

using std::string;
using std::vector;

namespace {
struct LeafModule : public sc_module {
    sc_signal<int> sig;
    LeafModule(sc_module_name name) : sc_module(name), sig("sig") {}
};

struct ParentModule : public sc_module {
    // created in the order z, a: the hierarchy (creation) order is not the name order
    LeafModule z;
    LeafModule a;
    ParentModule(sc_module_name name) : sc_module(name), z("z"), a("a") {}
};

template <typename T>
vector<string> names_of(const vector<T *> &objects)
{
    vector<string> names;
    for (const auto *obj : objects)
        names.push_back(obj->name());
    return names;
}
} // namespace

// B-016: the collections are in name order, independent of creation order and heap addresses
// (they were std::set<T *>, i.e. ordered by pointer)
TEST(SysCUtils, CollectionsAreNameOrdered)
{
    SPECS_TEST_HEADER()
    {
        // created in the order c, p, b; the heap usually gives increasing addresses, i.e. the
        // opposite of the name order for c and b
        vector<std::unique_ptr<sc_module>> modules;
        modules.emplace_back(new LeafModule("c"));
        modules.emplace_back(new ParentModule("p"));
        modules.emplace_back(new LeafModule("b"));

        const vector<string> expected_leaves = {"b", "c", "p.a", "p.z"};
        EXPECT_EQ(names_of(sc_get_all_module_by_type<LeafModule>()), expected_leaves);
        EXPECT_EQ(names_of(sc_get_all_object_by_type<LeafModule>()), expected_leaves);

        const vector<string> expected_signals = {"b.sig", "c.sig", "p.a.sig", "p.z.sig"};
        EXPECT_EQ(names_of(sc_get_all_object_by_type<sc_signal<int>>()), expected_signals);

        // every object exactly once, sorted by name in byte order (strcmp: a parent before its
        // children, upper case before lower case); SPX is specsGlobalConfig
        const vector<string> expected_modules = {"SPX", "b", "c", "p", "p.a", "p.z"};
        EXPECT_EQ(names_of(sc_get_all_module()), expected_modules);
        const vector<string> expected_objects = {"SPX", "SPX.signal_0", "b", "b.sig", "c", "c.sig",
                                                 "p", "p.a", "p.a.sig", "p.z", "p.z.sig"};
        EXPECT_EQ(names_of(sc_get_all_object()), expected_objects);

        // the hierarchy helpers keep the creation order
        EXPECT_EQ(names_of(sc_collect_children_module(modules[1].get())), (vector<string>{"p.z", "p.a"}));
        EXPECT_EQ(names_of(sc_collect_children_object(modules[1].get())),
                  (vector<string>{"p.z", "p.z.sig", "p.a", "p.a.sig"}));
    }
    SPECS_TEST_FOOTER();
}
