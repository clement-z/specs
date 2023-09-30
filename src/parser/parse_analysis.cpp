#include "parse_analysis.h"
#include "cw_source.h"
#include "devices/alldevices.h"
#include "directional_coupler.h"
#include "optical_output_port.h"
#include "parse_tree.h"
#include "specs.h"
#include "strutils.h"
#include "sysc_utils.h"
#include "general_utils.h"
#include <functional>

using spx::oa_value_type;
using spx::ea_value_type;
using spx::ed_value_type;

using spx::ed_bus_type;
using spx::oa_signal_type;
using spx::ea_signal_type;
using spx::ed_signal_type;
using spx::ed_bus_type;

void OPAnalysis::create() const
{
    specsGlobalConfig.analysis_type = SPECSConfig::OP;
    specsGlobalConfig.simulation_mode = OpticalOutputPortMode::FREQUENCY_DOMAIN;
}

void DCAnalysis::create() const
{
    specsGlobalConfig.analysis_type = SPECSConfig::DC;
    specsGlobalConfig.simulation_mode = OpticalOutputPortMode::FREQUENCY_DOMAIN;

    assert(sweep_orders.size() > 0);

    set<CWSource *>  all_cws = sc_get_all_object_by_type<CWSource>();
    vector<CWSource *>  all_cws_vec(all_cws.begin(), all_cws.end());

    int i = 0;
    for (const auto &sweep_order: sweep_orders)
    {
        string element_name = parent->name_prefix() + sweep_order.first.first;
        string attribute_name = sweep_order.first.second;

        auto is_desired_element = [&element_name](const auto &x){ return x->name() == element_name; };
        auto it = std::find_if(all_cws_vec.begin(), all_cws_vec.end(), is_desired_element);
        if (it == all_cws_vec.end())
        {
            cerr << "Element not found: " << element_name << endl;
            exit(1);
        }
        CWSource *elem = *it;
        // cout << elem << endl;
        // cout << &CWSource::setWavelength << endl;

        function<void(double)> f;
        if (attribute_name == "WL" || attribute_name == "WAVELENGTH" || attribute_name == "LAMBDA")
        {
            f = std::bind(&CWSource::setWavelength, elem, placeholders::_1);
        }
        else if (attribute_name == "P" || attribute_name == "POW" || attribute_name == "POWER")
        {
            f = std::bind(static_cast<void (CWSource::*)(const double&)>(&CWSource::setPower), elem, placeholders::_1);
        }
        else if (attribute_name == "F" || attribute_name == "FREQ" || attribute_name == "FREQUENCY")
        {
            f = std::bind((&CWSource::setFrequency), elem, placeholders::_1);
        }
        else
        {
            cerr << "Unknown attribute for " << sweep_order.first.first << ": " << attribute_name << endl;
            exit(1);
        }
        auto values = range(sweep_order.second[0], sweep_order.second[1], sweep_order.second[2]);
        auto order = pair<function<void(double)>, vector<double>>(f, values);
        auto order_name = attribute_name + "(" + element_name + ")";
        specsGlobalConfig.cw_sweep_orders.emplace(order_name, order);
        ++i;
    }
}

void TRANAnalysis::create() const
{
    specsGlobalConfig.analysis_type = SPECSConfig::TRAN;
    specsGlobalConfig.simulation_mode = OpticalOutputPortMode::TIME_DOMAIN;

    assert(args.size() <= 1);

    if (args.size() >= 1)
    {
        double val = args[0].as_double();
        // cout << val << endl;
        if (val > 0)
            specsGlobalConfig.tran_duration = args[0].as_double();
        else
        {
            cerr << "Negative simulation duration is not accepted" << endl;
            exit(1);
        }
    }

    for (const auto &p : kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        cerr << "Unknown keyword " << kw << endl;
        exit(1);
    }
}
