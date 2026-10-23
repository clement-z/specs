// F-15 (D3): the stateless device reactions and the output port are SC_METHODs, not threads (no
// context switch per event). The only thread these devices create is the one that prints their
// parameters at initialization with --vci (specsGlobalConfig.verbose_component_initialization),
// so that the devices print in the order their threads used to.
#include <gtest/gtest.h>
#include <systemc.h>

#include <algorithm>
#include <string>
#include <vector>

#include "devices/crossing.h"
#include "devices/directional_coupler.h"
#include "devices/merger.h"
#include "devices/phaseshifter.h"
#include "devices/splitter.h"
#include "devices/waveguide.h"
#include "specs.h"

#include "test/utils.h"

namespace {

// Process names (relative to the device, sorted) of the given kind in the device and its children
// (the output ports)
void collect_processes(const sc_object *obj, const std::string &prefix, const std::string &kind,
                       std::vector<std::string> &names)
{
    for (const sc_object *child : obj->get_child_objects())
    {
        if (std::string(child->kind()) == kind)
            names.push_back(std::string(child->name()).substr(prefix.size() + 1));
        collect_processes(child, prefix, kind, names);
    }
}

std::vector<std::string> processes(const sc_module &device, const std::string &kind)
{
    std::vector<std::string> names;
    collect_processes(&device, device.name(), kind, names);
    std::sort(names.begin(), names.end());
    return names;
}

// One device of each converted type
struct Devices {
    WaveguideUni wg{"wg"};
    WaveguideBi wg_bi{"wg_bi"};
    DirectionalCouplerUni dc{"dc"};
    DirectionalCouplerBi dc_bi{"dc_bi"};
    Splitter splitter{"splitter"};
    Merger merger{"merger"};
    CrossingUni crossing{"crossing"};
    CrossingBi crossing_bi{"crossing_bi"};
    PhaseShifterUni ps{"ps"};
    PhaseShifterBi ps_bi{"ps_bi"};

    std::vector<const sc_module *> all() const
    {
        return {&wg, &wg_bi, &dc, &dc_bi, &splitter, &merger, &crossing, &crossing_bi, &ps, &ps_bi};
    }
};

} // namespace

TEST(ProcessStyle, ReactionsAndOutputPortsAreMethods)
{
    SPECS_TEST_HEADER()
    {
        specsGlobalConfig.verbose_component_initialization = false;
        Devices d;

        for (const sc_module *device : d.all())
        {
            SCOPED_TRACE(device->name());
            EXPECT_TRUE(processes(*device, "sc_thread_process").empty());
            EXPECT_TRUE(processes(*device, "sc_cthread_process").empty());
        }

        // The reactions keep the on_<port>_changed() names (F-15 Q3); each output port has one
        // method, on_data_ready()
        EXPECT_EQ(processes(d.wg, "sc_method_process"),
                  (std::vector<std::string>{"on_port_in_changed", "out_delayed_writer.on_data_ready"}));
        EXPECT_EQ(processes(d.merger, "sc_method_process"),
                  (std::vector<std::string>{"on_port_in1_changed", "on_port_in2_changed",
                                            "out_delayed_writer.on_data_ready"}));
        EXPECT_EQ(processes(d.ps_bi, "sc_method_process"),
                  (std::vector<std::string>{"on_p0_in_changed", "on_p1_in_changed", "on_port_vin_changed",
                                            "p0_out_delayed_writer.on_data_ready",
                                            "p1_out_delayed_writer.on_data_ready"}));
        EXPECT_EQ(processes(d.dc_bi, "sc_method_process").size(), 4u + 4u);
        EXPECT_EQ(processes(d.crossing_bi, "sc_method_process").size(), 4u + 4u);
    }
    SPECS_TEST_FOOTER();
}

TEST(ProcessStyle, VerboseInitializationAddsOnePrintThreadPerDevice)
{
    SPECS_TEST_HEADER()
    {
        specsGlobalConfig.verbose_component_initialization = true;
        Devices d;

        for (const sc_module *device : d.all())
        {
            SCOPED_TRACE(device->name());
            EXPECT_EQ(processes(*device, "sc_thread_process"),
                      (std::vector<std::string>{"print_initialization"}));
        }
    }
    SPECS_TEST_FOOTER();
}
