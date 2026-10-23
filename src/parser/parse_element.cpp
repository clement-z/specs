#include "parser/parse_element.h"
#include "devices/alldevices.h"
#include "specs.h"
#include "parser/parser_state.h"
#include "utils/strutils.h"

#include <cstdint>
#include <limits>
#include <string>

/** ******************************************* **/
/**     Helper macros and defines               **/
/** ******************************************* **/
#define AS_READER (false)
#define AS_WRITER (!AS_READER)

#define INSTANTIATE_AND_CONNECT_UNI(ELEM_NAME, pt_helper) \
ELEM_NAME::element_type_base *ELEM_NAME::instantiate_and_connect_uni(ParseTreeCreationHelper &pt_helper) const

#define INSTANTIATE_AND_CONNECT_BI(ELEM_NAME, pt_helper) \
ELEM_NAME::element_type_base *ELEM_NAME::instantiate_and_connect_bi(ParseTreeCreationHelper &pt_helper) const

#define INSTANTIATE(pt_helper, bidirectional) \
\
if (bidirectional)\
    obj = (element_type_base *)instantiate_and_connect_bi(pt_helper);\
else\
    obj = (element_type_base *)instantiate_and_connect_uni(pt_helper);

// Integer parameter stored as an int: a value beyond the int range is an error, not a wrapped value
// (B-180). Values within it are checked by the device (min_valid is only for the message).
static int int_parameter(const Variable &v, const string &elem_name, const string &what, int min_valid)
{
    const int64_t i = v.as_integer();
    if (i < std::numeric_limits<int>::min() || i > std::numeric_limits<int>::max())
    {
        cerr << "Error (" << elem_name << "): " << what << " must be between " << min_valid << " and "
             << std::numeric_limits<int>::max() << " (got " << i << ")" << endl;
        exit(1);
    }
    return static_cast<int>(i);
}

/** ******************************************* **/
/**               Waveguide                     **/
/** ******************************************* **/
INSTANTIATE_AND_CONNECT_UNI(WGElement, pt_helper)
{
    // create object with default params
    element_type_uni *obj = new element_type_uni(name.c_str());

    // connect ports
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_in,  nets[0], AS_READER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_out, nets[1], AS_WRITER);

    // return the module
    return obj;
}

INSTANTIATE_AND_CONNECT_BI(WGElement, pt_helper)
{
    // create object with default params
    element_type_bi *obj = new element_type_bi(name.c_str());

    // Update unidirectional nets connected to the waveguide
    if (!pt_helper.nets->at(nets[0]).bidirectional())
        (*pt_helper.nets)[nets[0]].m_writers_count++;
    if (!pt_helper.nets->at(nets[1]).bidirectional())
        (*pt_helper.nets)[nets[1]].m_readers_count++;

    // Bidirectional waveguide needs all nets as bidir
    // (role of the device on each net: as in the uni variant)
    pt_helper.upgrade_signal(nets[0], AS_READER);
    pt_helper.upgrade_signal(nets[1], AS_WRITER);

    // connect ports
    pt_helper.connect_bi<spx::oa_signal_type>(obj->p0_in, obj->p0_out, nets[0]);
    pt_helper.connect_bi<spx::oa_signal_type>(obj->p1_in, obj->p1_out, nets[1]);

    return obj;
}

sc_module *WGElement::create(ParseTreeCreationHelper &pt_helper) const
{
    element_type_base *obj = nullptr;

    // Verify number of nets
    assert(nets.size() == n_nets);

    // Prevent connecting a waveguide to itself, it will hang the execution!!!
    if (nets[0] == nets[1])
    {
        cerr << "Error: A waveguide cannot be looped back upon himself" << endl;
        exit(1);
    }

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Check if device has to be bidirectional
    bool bidirectional = false;
    for (const auto &net : nets)
    {
        // for waveguide, if any net is bidir, module has to be bidir
        bidirectional |= pt_helper.nets->at(net).bidirectional();
    }

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, bidirectional);

    // B-014: written in the netlist, so a delay that rounds to 0 time steps is an error
    obj->m_from_netlist = true;

    // Parse positional arguments
    if(args.size() > 0)
        obj->m_length_cm = args[0].as_double() * 100;
    if(args.size() > 1)
        obj->m_neff = args[1].as_double();
    if(args.size() > 2)
        obj->m_ng = args[2].as_double();
    if(args.size() > 3)
        obj->m_attenuation_dB_cm = args[3].as_double();
    if(args.size() > 4)
        obj->m_D = args[4].as_double();

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "NEFF")
            obj->m_neff = p.second.as_double();
        else if (kw == "NG")
            obj->m_ng = p.second.as_double();
        else if (kw == "L" || kw == "LENGTH")
            obj->m_length_cm = p.second.as_double() * 100;
        else if (kw == "ATT")
            obj->m_attenuation_dB_cm = p.second.as_double();
        else if (kw == "D")
            obj->m_D = p.second.as_double();
        else {
            cerr << "Unknown keyword: " << p.first << endl;
            exit(1);
        }
    }

    return obj;
}

/** ******************************************* **/
/**               Merger                        **/
/** ******************************************* **/
INSTANTIATE_AND_CONNECT_UNI(MergerElement, pt_helper)
{
    // create object with default params
    element_type_uni *obj = new element_type_uni(name.c_str());

    // connect ports
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_in1, nets[0], AS_READER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_in2, nets[1], AS_READER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_out, nets[2], AS_WRITER);

    // return the module
    return obj;
}

sc_module *MergerElement::create(ParseTreeCreationHelper &pt_helper) const
{
    element_type_base *obj = nullptr;

    // Verify number of nets
    assert(nets.size() == n_nets);

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, false);

    // Parse positional arguments
    if(args.size() > 0)
        obj->m_attenuation_dB = args[0].as_double();

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "IL" || kw == "INSERTION_LOSS")
            obj->m_attenuation_dB = p.second.as_double();
        else {
            cerr << "Unknown keyword: " << p.first << endl;
            exit(1);
        }
    }
    return obj;
}

/** ******************************************* **/
/**               Splitter                      **/
/** ******************************************* **/
INSTANTIATE_AND_CONNECT_UNI(SplitterElement, pt_helper)
{
    // create object with default params
    element_type_uni *obj = new element_type_base(name.c_str());

    // connect ports
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_in,   nets[0], AS_READER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_out1, nets[1], AS_WRITER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_out2, nets[2], AS_WRITER);

    // return the module
    return obj;
}

sc_module *SplitterElement::create(ParseTreeCreationHelper &pt_helper) const
{
    element_type_base *obj = nullptr;

    // Verify number of nets
    assert(nets.size() == n_nets);

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, false);

    // Parse positional arguments
    if(args.size() > 0)
        obj->m_split_ratio = args[0].as_double();
    if(args.size() > 1)
        obj->m_attenuation_dB = args[1].as_double();

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "IL" || kw == "INSERTION_LOSS")
            obj->m_attenuation_dB = p.second.as_double();
        else if (kw == "SPLITTING_RATIO" || kw == "RATIO")
            obj->m_split_ratio = p.second.as_double();
        else {
            cerr << "Unknown keyword: " << p.first << endl;
            exit(1);
        }
    }
    if (!(obj->m_split_ratio >= 0 && obj->m_split_ratio <= 1))
    {
        cerr << "Error (" << name << "): splitter ratio " << obj->m_split_ratio << " is outside [0, 1]" << endl;
        exit(1);
    }
    return obj;
}

/** ******************************************* **/
/**            Directional coupler              **/
/** ******************************************* **/
INSTANTIATE_AND_CONNECT_UNI(DCElement, pt_helper)
{
    // create object with default params
    element_type_uni *obj = new element_type_uni(name.c_str());

    // connect p_in1
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_in1,  nets[0], AS_READER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_in2,  nets[1], AS_READER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_out1, nets[2], AS_WRITER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_out2, nets[3], AS_WRITER);

    // return the module
    return obj;
}

INSTANTIATE_AND_CONNECT_BI(DCElement, pt_helper)
{
    // create object with default params
    element_type_bi *obj = new element_type_bi(name.c_str());

    // Update unidirectional nets connected to the waveguide
    if (!pt_helper.nets->at(nets[0]).bidirectional())
        (*pt_helper.nets)[nets[0]].m_writers_count++;
    if (!pt_helper.nets->at(nets[1]).bidirectional())
        (*pt_helper.nets)[nets[1]].m_writers_count++;
    if (!pt_helper.nets->at(nets[2]).bidirectional())
        (*pt_helper.nets)[nets[2]].m_readers_count++;
    if (!pt_helper.nets->at(nets[3]).bidirectional())
        (*pt_helper.nets)[nets[3]].m_readers_count++;

    // Bidirectional DC needs all nets as bidir
    // (role of the device on each net: as in the uni variant)
    pt_helper.upgrade_signal(nets[0], AS_READER);
    pt_helper.upgrade_signal(nets[1], AS_READER);
    pt_helper.upgrade_signal(nets[2], AS_WRITER);
    pt_helper.upgrade_signal(nets[3], AS_WRITER);

    // connect ports
    pt_helper.connect_bi<spx::oa_signal_type>(obj->p0_in, obj->p0_out, nets[0]);
    pt_helper.connect_bi<spx::oa_signal_type>(obj->p1_in, obj->p1_out, nets[1]);
    pt_helper.connect_bi<spx::oa_signal_type>(obj->p2_in, obj->p2_out, nets[2]);
    pt_helper.connect_bi<spx::oa_signal_type>(obj->p3_in, obj->p3_out, nets[3]);

    // return the module
    return obj;
}

sc_module *DCElement::create(ParseTreeCreationHelper &pt_helper) const
{
    element_type_base *obj = nullptr;

    // Verify number of nets
    assert(nets.size() == n_nets);

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Check if device has to be bidirectional
    bool bidirectional = false;
    for (const auto &net : nets)
    {
        // for coupler, if any net is bidir, module has to be bidir
        bidirectional |= pt_helper.nets->at(net).bidirectional();
    }

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, bidirectional);

    // Parse positional arguments
    if(args.size() > 0)
        // k, cross-coupling field coef (P_cross = k^2): through power = 1 - k^2
        obj->m_dc_through_coupling_power = 1 - pow(args[0].as_double(), 2);
    if(args.size() > 1)
        obj->m_dc_loss = args[1].as_double(); // insertion loss in dB

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "K" || kw == "KF" || kw == "KFIELD")
            // k, cross-coupling field coef (P_cross = k^2): through power = 1 - k^2
            obj->m_dc_through_coupling_power = 1 - pow(p.second.as_double(), 2);
        else if (kw == "KP" || kw == "KPOW" || kw == "KPOWER")
            // kp, cross-coupling power ratio (P_cross = kp): through power = 1 - kp
            obj->m_dc_through_coupling_power = 1 - p.second.as_double();
        else if (kw == "T")
            // t, through field coef (P_through = t^2): through power = t^2
            obj->m_dc_through_coupling_power = pow(p.second.as_double(), 2);
        else if (kw == "LOSS")
            obj->m_dc_loss = p.second.as_double(); // insertion loss in dB
        else {
            cerr << "Unknown keyword: " << p.first << endl;
            exit(1);
        }
    }
    if (!(obj->m_dc_through_coupling_power >= 0 && obj->m_dc_through_coupling_power <= 1))
    {
        cerr << "Error (" << name << "): coupler through power " << obj->m_dc_through_coupling_power
             << " is outside [0, 1] (k and t must be in [-1, 1], kp in [0, 1])" << endl;
        exit(1);
    }
    return obj;
}

/** ******************************************* **/
/**               Phase-shifter                 **/
/** ******************************************* **/
INSTANTIATE_AND_CONNECT_UNI(PhaseShifterElement, pt_helper)
{
    // create object with default params
    element_type_uni *obj = new element_type_uni(name.c_str());

    // connect ports
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_in,  nets[0], AS_READER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_out, nets[1], AS_WRITER);
    pt_helper.connect_uni<spx::ea_signal_type>(obj->p_vin, nets[2], AS_READER);

    // return the module
    return obj;
}

INSTANTIATE_AND_CONNECT_BI(PhaseShifterElement, pt_helper)
{
    // create object with default params
    element_type_bi *obj = new element_type_bi(name.c_str());

    // Update unidirectional nets connected to the waveguide
    if (!pt_helper.nets->at(nets[0]).bidirectional())
        (*pt_helper.nets)[nets[0]].m_writers_count++;
    if (!pt_helper.nets->at(nets[1]).bidirectional())
        (*pt_helper.nets)[nets[1]].m_readers_count++;

    // Bidirectional phase-shifter needs only optical nets as bidir
    // (role of the device on each net: as in the uni variant)
    pt_helper.upgrade_signal(nets[0], AS_READER);
    pt_helper.upgrade_signal(nets[1], AS_WRITER);

    // connect ports
    pt_helper.connect_bi<spx::oa_signal_type>(obj->p0_in, obj->p0_out, nets[0]);
    pt_helper.connect_bi<spx::oa_signal_type>(obj->p1_in, obj->p1_out, nets[1]);
    pt_helper.connect_uni<spx::ea_signal_type>(obj->p_vin, nets[2], AS_READER);

    // return the module
    return obj;
}

sc_module *PhaseShifterElement::create(ParseTreeCreationHelper &pt_helper) const
{
    element_type_base *obj = nullptr;

    // Verify number of nodes
    assert(nets.size() == n_nets);

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Check if device has to be bidirectional
    // for PS, if any optical net is bidir, module has to be bidir
    bool bidirectional = false;
    bidirectional |= pt_helper.nets->at(nets[0]).bidirectional();
    bidirectional |= pt_helper.nets->at(nets[1]).bidirectional();

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, bidirectional);

    // Verify number of args
    if (args.size() > 2)
    {
        cerr << "Error (" << name << "): too many positional arguments (" << args.size() << ", expected at most 2)" << endl;
        exit(1);
    }

    // Parse positional arguments
    if(args.size() > 0)
        obj->m_sensitivity = args[0].as_double();
    if(args.size() > 1)
        obj->m_attenuation_dB = args[1].as_double();

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);

        if (kw == "ATTENUATION" || kw == "ATT")
            obj->m_attenuation_dB = p.second.as_double();
        else if (kw == "SENSITIVITY" || kw == "GAIN" || kw == "G")
            obj->m_sensitivity = p.second.as_double();
        else {
            cerr << "Unknown keyword: " << p.first << endl;
            exit(1);
        }
    }
    return obj;
}

/** ******************************************* **/
/**                    MZI                      **/
/** ******************************************* **/
INSTANTIATE_AND_CONNECT_UNI(MZIElement, pt_helper)
{
    // create object with default params
    element_type_uni *obj = new element_type_uni(name.c_str());

    // connect ports
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_in1,  nets[0], AS_READER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_in2,  nets[1], AS_READER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_out1, nets[2], AS_WRITER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_out2, nets[3], AS_WRITER);
    pt_helper.connect_uni<spx::ea_signal_type>(obj->p_vin,  nets[4], AS_READER);

    return obj;
}

INSTANTIATE_AND_CONNECT_BI(MZIElement, pt_helper)
{
    // create object with default params
    element_type_bi *obj = new element_type_bi(name.c_str());

    // Update unidirectional nets connected to the waveguide
    if (!pt_helper.nets->at(nets[0]).bidirectional())
        (*pt_helper.nets)[nets[0]].m_writers_count++;
    if (!pt_helper.nets->at(nets[1]).bidirectional())
        (*pt_helper.nets)[nets[1]].m_writers_count++;
    if (!pt_helper.nets->at(nets[2]).bidirectional())
        (*pt_helper.nets)[nets[2]].m_readers_count++;
    if (!pt_helper.nets->at(nets[3]).bidirectional())
        (*pt_helper.nets)[nets[3]].m_readers_count++;

    // Bidirectional phase-shifter needs only optical nets as bidir
    // (role of the device on each net: as in the uni variant)
    pt_helper.upgrade_signal(nets[0], AS_READER);
    pt_helper.upgrade_signal(nets[1], AS_READER);
    pt_helper.upgrade_signal(nets[2], AS_WRITER);
    pt_helper.upgrade_signal(nets[3], AS_WRITER);

    // connect ports
    pt_helper.connect_bi<spx::oa_signal_type>(obj->p0_in, obj->p0_out, nets[0]);
    pt_helper.connect_bi<spx::oa_signal_type>(obj->p1_in, obj->p1_out, nets[1]);
    pt_helper.connect_bi<spx::oa_signal_type>(obj->p2_in, obj->p2_out, nets[2]);
    pt_helper.connect_bi<spx::oa_signal_type>(obj->p3_in, obj->p3_out, nets[3]);
    pt_helper.connect_uni<spx::ea_signal_type>(obj->p_vin,             nets[4], AS_READER);

    return obj;
}

sc_module *MZIElement::create(ParseTreeCreationHelper &pt_helper) const
{
    element_type_base *obj = nullptr;

    // Verify number of nodes
    assert(nets.size() == n_nets);

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Check if device has to be bidirectional
    // for PS, if any optical net is bidir, module has to be bidir
    bool bidirectional = false;
    bidirectional |= pt_helper.nets->at(nets[0]).bidirectional();
    bidirectional |= pt_helper.nets->at(nets[1]).bidirectional();

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, bidirectional);

    if (args.size() > 1)
    {
        cerr << "Error (" << name << "): too many positional arguments (" << args.size() << ", expected at most 1)" << endl;
        exit(1);
    }
    // Parse positional arguments
    if(args.size() > 0)
        obj->m_ps_sens_rad_v = args[0].as_double();
    // TODO: other positional args

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "SENSITIVITY")
            obj->m_ps_sens_rad_v = p.second.as_double();
        else if (kw == "LENGTH")
            obj->m_length_cm = 100 * p.second.as_double();
        else if (kw == "LENGTH_REF")
            obj->m_length_ref_cm = 100 * p.second.as_double();
        else if (kw == "NEFF")
            obj->m_neff = p.second.as_double();
        else if (kw == "NG")
            obj->m_ng = p.second.as_double();
        else if (kw == "ATT")
            obj->m_attenuation_dB_cm = p.second.as_double();
        else if (kw == "IL_DC")
            obj->m_dc_loss_dB = p.second.as_double();
        else if (kw == "IL_PHASESHIFTER")
            obj->m_ps_loss_dB = p.second.as_double();
        else {
            cerr << "Unknown keyword: " << p.first << endl;
            exit(1);
        }
    }
    return obj;
}

/** ******************************************* **/
/**                 Crossing                    **/
/** ******************************************* **/
INSTANTIATE_AND_CONNECT_UNI(CrossingElement, pt_helper)
{
    element_type_uni *obj = new element_type_uni(name.c_str());

    // connect ports
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_in1,  nets[0], AS_READER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_in2,  nets[1], AS_READER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_out1, nets[2], AS_WRITER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_out2, nets[3], AS_WRITER);

    // return the module
    return obj;
}

INSTANTIATE_AND_CONNECT_BI(CrossingElement, pt_helper)
{
    element_type_bi *obj = new element_type_bi(name.c_str());

    // Update unidirectional nets connected to the crossing
    if (!pt_helper.nets->at(nets[0]).bidirectional())
        (*pt_helper.nets)[nets[0]].m_writers_count++;
    if (!pt_helper.nets->at(nets[1]).bidirectional())
        (*pt_helper.nets)[nets[1]].m_writers_count++;
    if (!pt_helper.nets->at(nets[2]).bidirectional())
        (*pt_helper.nets)[nets[2]].m_readers_count++;
    if (!pt_helper.nets->at(nets[3]).bidirectional())
        (*pt_helper.nets)[nets[3]].m_readers_count++;

    // Bidirectional crossing needs all optical nets as bidir
    // (role of the device on each net: as in the uni variant)
    pt_helper.upgrade_signal(nets[0], AS_READER);
    pt_helper.upgrade_signal(nets[1], AS_READER);
    pt_helper.upgrade_signal(nets[2], AS_WRITER);
    pt_helper.upgrade_signal(nets[3], AS_WRITER);

    // connect ports
    pt_helper.connect_bi<spx::oa_signal_type>(obj->p0_in, obj->p0_out, nets[0]);
    pt_helper.connect_bi<spx::oa_signal_type>(obj->p1_in, obj->p1_out, nets[1]);
    pt_helper.connect_bi<spx::oa_signal_type>(obj->p2_in, obj->p2_out, nets[2]);
    pt_helper.connect_bi<spx::oa_signal_type>(obj->p3_in, obj->p3_out, nets[3]);

    // return the module
    return obj;
}

sc_module *CrossingElement::create(ParseTreeCreationHelper &pt_helper) const
{
    element_type_base *obj = nullptr;

    // Verify number of nets
    assert(nets.size() == n_nets);

    // Verify number of positional args
    if (args.size() > 2)
    {
        cerr << "Error (" << name << "): too many positional arguments (" << args.size() << ", expected at most 2)" << endl;
        exit(1);
    }

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Check if device has to be bidirectional (B-159)
    bool bidirectional = false;
    for (const auto &net : nets)
    {
        // for crossing, if any net is bidir, module has to be bidir
        bidirectional |= pt_helper.nets->at(net).bidirectional();
    }

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, bidirectional);

    // Parse positional arguments
    // att: excess loss in dB, on top of the power lost to crosstalk
    // xtalk: cross power ratio in dB; its sign is ignored, a positive value is read as
    // an isolation (xtalk=20 and xtalk=-20 both give 1 % on the cross port, B-032)
    if(args.size() > 0)
        obj->m_attenuation_power_dB = args[0].as_double();
    if(args.size() > 1)
        obj->m_crosstalk_power_dB = args[1].as_double();

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "ATTENUATION" || kw == "ATT")
            obj->m_attenuation_power_dB = p.second.as_double();
        else if (kw == "CROSSTALK" || kw == "XTALK")
            // sign ignored: xtalk=20 (isolation) is the same as xtalk=-20 (B-032)
            obj->m_crosstalk_power_dB = p.second.as_double();
        else {
            cerr << "Unknown keyword: " << p.first << endl;
            exit(1);
        }
    }
    return obj;
}

/** ******************************************* **/
/**                  CW Source                  **/
/** ******************************************* **/
INSTANTIATE_AND_CONNECT_UNI(CWSourceElement, pt_helper)
{
    element_type_uni *obj = new element_type_uni(name.c_str());

    // connect ports
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_out, nets[0], AS_WRITER);

    // return the module
    return obj;
}

sc_module *CWSourceElement::create(ParseTreeCreationHelper &pt_helper) const
{
    CWSource *obj = nullptr;

    // Verify number of nets
    assert(nets.size() == 1);

    // Verify number of positional args
    if (args.size() > 3)
    {
        cerr << "Error (" << name << "): too many positional arguments (" << args.size() << ", expected at most 3)" << endl;
        exit(1);
    }

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, false);

    // Parse positional arguments (wl power phi)
    if(args.size() > 0)
        obj->setWavelength(args[0].as_double());
    if(args.size() > 1)
        obj->setPower(args[1].as_double());
    if(args.size() > 2)
        obj->setPhase(args[2].as_double());

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "WL" || kw == "WAVELENGTH")
            obj->setWavelength(p.second.as_double());
        else if (kw == "POWER")
            obj->setPower(p.second.as_double());
        else if (kw == "PHI")
            continue; // handle it after all power commands are executed
        else {
            cerr << "Unknown keyword: " << p.first << endl;
            exit(1);
        }
    }
    // TODO: make this cleaner (priority between setting kwargs...)
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "PHI")
            obj->setPhase(p.second.as_double());
    }
    return obj;
}

/** ******************************************* **/
/**                  VL Source                  **/
/** ******************************************* **/
INSTANTIATE_AND_CONNECT_UNI(VLSourceElement, pt_helper)
{
    element_type_uni *obj = new element_type_uni(name.c_str());

    // connect ports
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_out, nets[0], AS_WRITER);

    // return the module
    return obj;
}

sc_module *VLSourceElement::create(ParseTreeCreationHelper &pt_helper) const
{
    VLSource *obj = nullptr;

    // Verify number of nets
    assert(nets.size() == n_nets);

    // Verify number of positional args
    if (args.size() > 1)
    {
        cerr << "Error (" << name << "): too many positional arguments (" << args.size() << ", expected at most 1)" << endl;
        exit(1);
    }

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, false);

    // Parse positional arguments (file)
    if(args.size() > 0)
        obj->setValues(args[0].as_string());

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "FILE")
        {
            cerr << "not implemented" << endl;
            exit(1);
        }
        else if (kw == "SCALE")
        {
            cerr << "not implemented" << endl;
            exit(1);
        }
        else if (kw == "VALUES")
        {
            // cout << "here" << endl;
            vector<VLSource::time_value_pair_type> values;
            bool is_string = p.second.type == Variable::STRING;
            bool is_list = p.second.type == Variable::LIST;
            if (!is_list && !is_string)
            {
                // cout << "A" << endl;
                cerr << "Variable passed to keyword 'values' should be either:"<< endl;
                cerr << " - an Nx3 list [[t0, p0, wl0], ...] with t0, p0 and wl0 convertible to double"
                     << " (or None if no change from previous entry)" << endl;
                cerr << " - a filename containing the list" << endl;
                exit(1);
            }

            if (is_list)
            {
                for (const Variable &var : p.second.vec)
                {
                    if (var.type != Variable::LIST || var.vec.size() != 3)
                    {
                        // cout << "B" << endl;
                        cerr << "Variable passed to keyword 'values' should be a an Nx3 list [[t0, p0, wl0], ...]" << endl;
                        cerr << "with t0, p0 and wl0 convertible to double (or None if no change from previous entry)" << endl;
                        exit(1);
                    }
                    for (const Variable &ivar : var.vec)
                    {
                        // cout << "C" << ivar.get_str() << endl;

                        if (ivar.is_number() || ivar.is_none())
                            continue;

                        cerr << "Variable passed to keyword 'values' should be a an Nx3 list [[t0, p0, wl0], ...]" << endl;
                        cerr << "with t0, p0 and wl0 convertible to double (or None if no change from previous entry)" << endl;
                        exit(1);
                    }
                    double t, p, wl;
                    int none_cnt = 0;
                    if (var.vec[0].is_number())
                        t = var.vec[0].as_double();
                    else if (var.vec[0].is_none() && ! values.empty())
                    {
                        t = values.back().first;
                        ++none_cnt;
                    }
                    else
                    {
                        cerr << "First values list entry should be fully specified" << endl;
                        exit(1);
                    }

                    if (var.vec[1].is_number())
                        p = var.vec[1].as_double();
                    else if (var.vec[1].is_none() && ! values.empty())
                    {
                        p = values.back().second.power();
                        ++none_cnt;
                    }
                    else
                    {
                        cerr << "First values list entry should be fully specified" << endl;
                        exit(1);
                    }

                    if (var.vec[2].is_number())
                        wl = var.vec[2].as_double();
                    else if (var.vec[2].is_none() && ! values.empty())
                    {
                        wl = values.back().second.getWavelength();
                        ++none_cnt;
                    }
                    else
                    {
                        cerr << "First values list entry should be fully specified" << endl;
                        exit(1);
                    }

                    if (none_cnt < 3)
                    // OpticalSignal takes amplitude as value in the constructor
                        values.emplace_back(t, spx::oa_value_type(sqrt(p), wl));
                    else
                    {
                        cerr << "Values list entry cannot be all none" << endl;
                    }
                }
                obj->setValues(values);
            }

            if (is_string)
            {
                obj->setValues(p.second.as_string());
            }
        }
        else {
            cerr << "Unknown keyword: " << p.first << endl;
            exit(1);
        }

    }
    return obj;
}

// Value files of a VLSRC/EVLSRC line, as create() opens them: the positional argument, then
// values="<file>" (a path relative to the working directory)
static vector<string> value_list_files(const ParseElement &e)
{
    vector<string> files;
    if (!e.args.empty() && e.args[0].type == Variable::STRING)
        files.push_back(e.args[0].str);
    for (const auto &p : e.kwargs)
    {
        if (strutils::iequals(p.first, "VALUES") && p.second.type == Variable::STRING)
            files.push_back(p.second.str);
    }
    return files;
}

vector<string> VLSourceElement::data_files() const
{
    return value_list_files(*this);
}

/** ******************************************* **/
/**                 EVL Source                  **/
/** ******************************************* **/
INSTANTIATE_AND_CONNECT_UNI(EVLSourceElement, pt_helper)
{
    element_type_uni *obj = new element_type_uni(name.c_str());

    // connect ports
    pt_helper.connect_uni<spx::ea_signal_type>(obj->p_out, nets[0], AS_WRITER);

    // return the module
    return obj;
}

sc_module *EVLSourceElement::create(ParseTreeCreationHelper &pt_helper) const
{
    EVLSource *obj = nullptr;

    // Verify number of nets
    assert(nets.size() == n_nets);

    // Verify number of positional args
    if (args.size() > 1)
    {
        cerr << "Error (" << name << "): too many positional arguments (" << args.size() << ", expected at most 1)" << endl;
        exit(1);
    }

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, false);

    // Parse positional arguments (file)
    if(args.size() > 0)
        obj->setValues(args[0].as_string());

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "FILE")
        {
            cerr << "not implemented" << endl;
            exit(1);
        }
        else if (kw == "SCALE")
        {
            cerr << "not implemented" << endl;
            exit(1);
        }
        else if (kw == "VALUES")
        {
            // cout << "here" << endl;
            vector<EVLSource::time_value_pair_type> values;
            bool is_string = p.second.type == Variable::STRING;
            bool is_list = p.second.type == Variable::LIST;
            if (!is_list && !is_string)
            {
                cerr << "Variable passed to keyword 'values' should be either:"<< endl;
                cerr << " - an Nx2 list [[t0, V0], ...] with t0 and V0 convertible to double"
                     << " (or None if no change from previous entry)" << endl;
                cerr << " - a filename containing the list" << endl;
                exit(1);
            }

            if (is_list)
            {
                for (const Variable &var : p.second.vec)
                {
                    if (var.type != Variable::LIST || var.vec.size() != 2)
                    {
                        cerr << "Variable passed to keyword 'values' should be either:"<< endl;
                        cerr << " - an Nx2 list [[t0, V0], ...] with t0 and V0 convertible to double"
                            << " (or None if no change from previous entry)" << endl;
                        cerr << " - a filename containing the list" << endl;
                        exit(1);
                    }
                    for (const Variable &ivar : var.vec)
                    {
                        if (ivar.is_number() || ivar.is_none())
                            continue;

                        cerr << "Variable passed to keyword 'values' should be either:"<< endl;
                        cerr << " - an Nx2 list [[t0, V0], ...] with t0 and V0 convertible to double"
                            << " (or None if no change from previous entry)" << endl;
                        cerr << " - a filename containing the list" << endl;
                        exit(1);
                    }
                    double t, V;
                    int none_cnt = 0;
                    if (var.vec[0].is_number())
                        t = var.vec[0].as_double();
                    else if (var.vec[0].is_none() && ! values.empty())
                    {
                        t = values.back().first;
                        ++none_cnt;
                    }
                    else
                    {
                        cerr << "First values list entry should be fully specified" << endl;
                        exit(1);
                    }

                    if (var.vec[1].is_number())
                        V = var.vec[1].as_double();
                    else if (var.vec[1].is_none() && ! values.empty())
                    {
                        V = values.back().second;
                        ++none_cnt;
                    }
                    else
                    {
                        cerr << "First values list entry should be fully specified" << endl;
                        exit(1);
                    }

                    if (none_cnt < 2)
                        values.emplace_back(t, spx::ea_value_type(V));
                    else
                    {
                        cerr << "Values list entry cannot be all none" << endl;
                    }
                }
                obj->setValues(values);
            }

            if (is_string)
            {
                obj->setValues(p.second.as_string());
            }
        }
        else {
            cerr << "Unknown keyword: " << p.first << endl;
            exit(1);
        }

    }
    return obj;
}

vector<string> EVLSourceElement::data_files() const
{
    return value_list_files(*this);
}

/** ******************************************* **/
/**                   Probe                     **/
/** ******************************************* **/
INSTANTIATE_AND_CONNECT_UNI(ProbeElement, pt_helper)
{
    element_type_uni *obj = new element_type_uni(name.c_str());

    // connect p_in
    pt_helper.connect_uni<spx::oa_signal_type>(
        obj->p_in,
        nets[0],
        AS_READER
    );

    // return the module
    return obj;
}

sc_module *ProbeElement::create(ParseTreeCreationHelper &pt_helper) const
{
    Probe *obj = nullptr;

    // Verify number of nets
    assert(nets.size() == n_nets);

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, false);

    if (args.empty() && kwargs.empty())
    {
        // cout << "empty args" << endl;
        obj->m_trace_power = true;
        obj->m_trace_modulus = true;
        obj->m_trace_phase = true;
        obj->m_trace_wavelength = true;
    }

    // The wavelength is always traced (PD6): a false 4th argument or wl=0 is still accepted, and
    // ignored with a warning (build_circuit())
    bool trace_wavelength = true;

    // Parse positional args
    for (size_t i = 0; i < args.size(); ++i)
    {
        switch (i) {
            case 0:
                obj->m_trace_power = args[i].as_boolean();
                break;
            case 1:
                obj->m_trace_modulus = args[i].as_boolean();
                break;
            case 2:
                obj->m_trace_phase = args[i].as_boolean();
                break;
            case 3:
                trace_wavelength = args[i].as_boolean();
                break;
            default:
                cerr << "Too many input arguments for " << name << endl;
                exit(1);
        }
    }

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "POWER" || kw == "POW" || kw == "P")
            obj->m_trace_power = p.second.as_boolean();
        else if (kw == "MAGNITUDE" || kw == "MAG" || kw == "MODULUS" || kw == "MOD" || kw == "E")
            obj->m_trace_modulus = p.second.as_boolean();
        else if (kw == "PHASE" || kw == "PHI")
            obj->m_trace_phase = p.second.as_boolean();
        else if (kw == "WAVELENGTH" || kw == "WL")
            trace_wavelength = p.second.as_boolean();
        else {
            cerr << name << ": unknown keyword: " << p.first << endl;
            exit(1);
        }
    }
    obj->m_trace_wavelength = true;
    if (!trace_wavelength)
        pt_helper.probes_ignoring_wavelength_off.insert(name);
    return obj;
}

/** ******************************************* **/
/**            Multilambda Probe                **/
/** ******************************************* **/
INSTANTIATE_AND_CONNECT_UNI(MLProbeElement, pt_helper)
{
    element_type_uni *obj = new element_type_uni(name.c_str());

    // connect p_in
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_in, nets[0], AS_READER);

    // return the module
    return obj;
}

sc_module *MLProbeElement::create(ParseTreeCreationHelper &pt_helper) const
{
    element_type_base *obj = nullptr;

    // Verify number of nets
    assert(nets.size() == n_nets);

    // Verify number of positional args
    if (args.size() > 1)
    {
        cerr << "Error (" << name << "): too many positional arguments (" << args.size() << ", expected at most 1)" << endl;
        exit(1);
    }

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, false);

    // Parse positional args
    if (args.size() > 0)
    {
        if (args[0].is_list())
        {
            for(auto v: args[0].vec)
            {
                obj->m_lambdas.insert(v.as_double());
            }
        }
        else {
            cerr << "Positional arg to MLPROBE should be a list of wavelengths (e.g. [1.55e-6, 1.54e-6,...])" << endl;
            exit(1);
        }
    }

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "WAVELENGTHS" || kw == "WL") {
            if (p.second.is_list())
            {
                for(auto v: p.second.vec)
                {
                    obj->m_lambdas.insert(v.as_double());
                    //obj->m_lambdas.push_back(v.as_double());
                }
            }
            else {
                cerr << "value of kwarg \"" << kw << "\" to MLPROBE should be a list of wavelengths (e.g. [1.55e-6, 1.54e-6,...])" << endl;
                exit(1);
            }
        } else {
            cerr << name << ": unknown keyword: " << p.first << endl;
            exit(1);
        }
    }

    return obj;
}

/** ******************************************* **/
/**                Power meter                  **/
/** ******************************************* **/
INSTANTIATE_AND_CONNECT_UNI(PowerMeterElement, pt_helper)
{
    element_type_uni *obj = new element_type_uni(name.c_str());

    // connect p_in
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_in, nets[0], AS_READER);

    // return the module
    return obj;
}

sc_module *PowerMeterElement::create(ParseTreeCreationHelper &pt_helper) const
{
    element_type_base *obj = nullptr;

    // Verify number of nets
    assert(nets.size() == n_nets);

    // Verify number of positional args
    if (args.size() > 0)
    {
        cerr << "Error (" << name << "): too many positional arguments (" << args.size() << ", expected at most 0)" << endl;
        exit(1);
    }

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, false);

    // Parse positional args
    // nothing

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        cerr << name << ": unknown keyword: " << p.first << endl;
        exit(1);
    }

    return obj;
}


/** ******************************************* **/
/**              Phase-change cell              **/
/** ******************************************* **/
INSTANTIATE_AND_CONNECT_UNI(PCMCellElement, pt_helper)
{
    element_type_uni *obj = new element_type_uni(name.c_str());

    // connect ports
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_in,  nets[0], AS_READER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_out, nets[1], AS_WRITER);

    // return the module
    return obj;
}

sc_module *PCMCellElement::create(ParseTreeCreationHelper &pt_helper) const
{
    element_type_base *obj = nullptr;

    // Verify number of nets
    assert(nets.size() == n_nets);

    // Verify number of positional args
    if (args.size() > 4)
    {
        cerr << "Error (" << name << "): too many positional arguments (" << args.size() << ", expected at most 4)" << endl;
        exit(1);
    }

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, false);

    // Tc and Ta are required (B-051): no physical default
    bool tc_given = false;
    bool ta_given = false;

    // Parse positional arguments (emelt n tc ta)
    if(args.size() > 0)
        obj->m_meltEnergy = args[0].as_double();
    if(args.size() > 1)
        obj->m_nStates = int_parameter(args[1], name, "the number of states n", 1);
    if(args.size() > 2)
    {
        obj->m_Tc = args[2].as_double();
        tc_given = true;
    }
    if(args.size() > 3)
    {
        obj->m_Ta = args[3].as_double();
        ta_given = true;
    }

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "MELT_ENERGY" || kw == "EMELT" || kw == "E_MELT")
            obj->m_meltEnergy = p.second.as_double();
        else if (kw == "N" || kw == "NSTATES" || kw == "NLEVELS" || kw == "N_STATES" || kw == "N_LEVELS")
            obj->m_nStates = int_parameter(p.second, name, "the number of states n", 1);
        else if (kw == "K" || kw == "INITIAL_STATE")
            obj->m_stateCurrent = int_parameter(p.second, name, "the initial state k", 0);
        else if (kw == "SP" || kw == "TC" || kw == "T_C")
        {
            obj->m_Tc = p.second.as_double();
            tc_given = true;
        }
        else if (kw == "EP" || kw == "TA" || kw == "T_A")
        {
            obj->m_Ta = p.second.as_double();
            ta_given = true;
        }
        else if (kw == "TANH_COEF")
            obj->m_speed = p.second.as_double();
        else {
            cerr << "Unknown keyword: " << p.first << endl;
            exit(1);
        }
    }
    if (obj->m_meltEnergy == 0 || obj->m_nStates == 0)
    {
        cerr << "PCM Cell needs values for both Emelt and Nstates" << endl;
        exit(1);
    }
    if (!tc_given || !ta_given)
    {
        const char *missing = (!tc_given && !ta_given) ? "tc, ta" : (!tc_given ? "tc" : "ta");
        cerr << "Error (" << name << "): PCM Cell needs values for both Tc and Ta (missing: " << missing << ")" << endl;
        exit(1);
    }
    // Reject n <= 0 and an initial state k outside [0, n] (B-060)
    obj->check_parameters();
    return obj;
}


/** ******************************************* **/
/**                 Photodetector               **/
/** ******************************************* **/
INSTANTIATE_AND_CONNECT_UNI(PhotodetectorElement, pt_helper)
{
    element_type_uni *obj = new element_type_uni(name.c_str());

    // connect ports
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_in,      nets[0], AS_READER);
    pt_helper.connect_uni<spx::ea_signal_type>(obj->p_readout, nets[1], AS_WRITER);

    // return the module
    return obj;
}

sc_module *PhotodetectorElement::create(ParseTreeCreationHelper &pt_helper) const
{
    element_type_base *obj = nullptr;

    // Verify number of nets
    assert(nets.size() == n_nets);

    // Verify number of positional args
    if (args.size() > 2)
    {
        cerr << "Error (" << name << "): too many positional arguments (" << args.size() << ", expected at most 2)" << endl;
        exit(1);
    }

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, false);

    // Parse positional arguments
    if(args.size() > 0)
        obj->m_sampling_time = args[0].as_double();
    if(args.size() > 1)
        obj->m_responsivity_A_W = args[1].as_double();

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "SAMPLING_TIME" || kw == "TS")
            obj->m_sampling_time = p.second.as_double();
        else if (kw == "R" || kw == "RESPONSIVITY" || kw == "GAIN")
            obj->m_responsivity_A_W = p.second.as_double();
        else if (kw == "NOISE_BYPASS" || kw == "NB")
            obj->m_noiseBypass = p.second.as_boolean();
        else if (kw == "FREQUENCY" || kw == "FOP")
            obj->m_opFreq_Hz = p.second.as_double();            
        else if (kw == "DARKFIELD")
            obj->m_dark_field_injection = p.second.as_boolean();
        else {
            cerr << "Unknown keyword: " << p.first << endl;
            exit(1);
        }
    }
    return obj;
}

/** ******************************************* **/
/**           S-parameter device (F-03)         **/
/** ******************************************* **/
void SPARElement::set_net_roles(ParseTree &pt) const
{
    const bool odd = nets.size() % 2 != 0;
    for (size_t k = 0; k < nets.size(); ++k)
    {
        ParseNet &net = pt.nets[nets[k]];
        if (odd)
        {
            // no unidirectional variant: reader and writer of a bidirectional net (as net&)
            net.m_bidirectional = true;
            net.m_readers_count++;
            net.m_writers_count++;
        }
        else if (writes_net(k))
            net.m_writers_count++;
        else
            net.m_readers_count++;
    }
}

INSTANTIATE_AND_CONNECT_UNI(SPARElement, pt_helper)
{
    element_type_uni *obj = new element_type_uni(name.c_str(), nets.size());

    // first half: inputs, second half: outputs
    for (size_t k = 0; k < nets.size(); ++k)
    {
        if (writes_net(k))
            pt_helper.connect_uni<spx::oa_signal_type>(*obj->p_out[k - nets.size() / 2], nets[k], AS_WRITER);
        else
            pt_helper.connect_uni<spx::oa_signal_type>(*obj->p_in[k], nets[k], AS_READER);
    }
    return obj;
}

INSTANTIATE_AND_CONNECT_BI(SPARElement, pt_helper)
{
    element_type_bi *obj = new element_type_bi(name.c_str(), nets.size());
    const bool odd = nets.size() % 2 != 0;

    for (size_t k = 0; k < nets.size(); ++k)
    {
        // Update the unidirectional nets (role of the device on each net: as in the uni variant; an odd
        // number of nets has none, its nets are already bidirectional)
        if (!odd && !pt_helper.nets->at(nets[k]).bidirectional())
        {
            if (writes_net(k))
                (*pt_helper.nets)[nets[k]].m_readers_count++;
            else
                (*pt_helper.nets)[nets[k]].m_writers_count++;
        }
        pt_helper.upgrade_signal(nets[k], odd || writes_net(k));
    }
    for (size_t k = 0; k < nets.size(); ++k)
        pt_helper.connect_bi<spx::oa_signal_type>(*obj->p_in[k], *obj->p_out[k], nets[k]);
    return obj;
}

string SPARElement::resolve_file(const string &file) const
{
    // Relative path: next to the netlist file first, then in the working directory (as .include)
    ParserState state;
    state.include_dirs.push_back(include_dir);
    return state.resolve_include(file);
}

vector<string> SPARElement::data_files() const
{
    // the file= that create() keeps (the last one)
    string file;
    for (const auto &p : kwargs)
    {
        if (strutils::iequals(p.first, "FILE") && p.second.type == Variable::STRING)
            file = p.second.str;
    }
    if (file.empty())
        return {};
    return { resolve_file(file) };
}

sc_module *SPARElement::create(ParseTreeCreationHelper &pt_helper) const
{
    element_type_base *obj = nullptr;

    if (nets.empty())
    {
        cerr << "Error (" << name << "): an S-parameter device needs at least one net" << endl;
        exit(1);
    }
    if (!args.empty())
    {
        cerr << "Error (" << name << "): unexpected positional argument " << args[0].get_str()
             << " (give the file with file=\"...\")" << endl;
        exit(1);
    }

    // Keyword arguments
    string file;
    string ports;
    SParamData::Format format = SParamData::FORMAT_AUTO;
    SParamData::Convention convention = SParamData::CONVENTION_AUTO;
    SParamData::Interp interp = SParamData::INTERP_LINEAR;
    string passivity = "check";
    SParamDeviceBase::DelayMode delay_mode = SParamDeviceBase::DELAY_AUTO;
    double delay_s = 0;
    auto string_kw = [this](const pair<const string, Variable> &p) {
        if (p.second.type != Variable::STRING)
        {
            cerr << "Error (" << name << "): " << p.first << " must be a string in quotes (got "
                 << p.second.get_str() << ")" << endl;
            exit(1);
        }
        string v = p.second.as_string();
        return v;
    };
    auto bad_value = [this](const string &kw, const string &v, const string &expected) {
        cerr << "Error (" << name << "): invalid " << kw << "=\"" << v << "\" (expected " << expected << ")" << endl;
        exit(1);
    };
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "FILE")
            file = string_kw(p);
        else if (kw == "PORTS")
            ports = string_kw(p);
        else if (kw == "FORMAT")
        {
            const string v = string_kw(p);
            if (strutils::iequals(v, "auto")) format = SParamData::FORMAT_AUTO;
            else if (strutils::iequals(v, "sparam") || strutils::iequals(v, "lumerical")) format = SParamData::FORMAT_SPARAM;
            else if (strutils::iequals(v, "touchstone")) format = SParamData::FORMAT_TOUCHSTONE;
            else if (strutils::iequals(v, "csv")) format = SParamData::FORMAT_CSV;
            else if (strutils::iequals(v, "taylor")) format = SParamData::FORMAT_TAYLOR;
            else bad_value(p.first, v, "auto, sparam (or lumerical), touchstone, csv or taylor");
        }
        else if (kw == "CONVENTION")
        {
            const string v = string_kw(p);
            if (strutils::iequals(v, "auto")) convention = SParamData::CONVENTION_AUTO;
            else if (strutils::iequals(v, "physics")) convention = SParamData::CONVENTION_PHYSICS;
            else if (strutils::iequals(v, "engineering")) convention = SParamData::CONVENTION_ENGINEERING;
            else bad_value(p.first, v, "auto, physics or engineering");
        }
        else if (kw == "INTERP")
        {
            const string v = string_kw(p);
            if (strutils::iequals(v, "linear")) interp = SParamData::INTERP_LINEAR;
            else if (strutils::iequals(v, "pchip")) interp = SParamData::INTERP_PCHIP;
            else bad_value(p.first, v, "linear or pchip");
        }
        else if (kw == "PASSIVITY")
        {
            passivity = string_kw(p);
            if (!strutils::iequals(passivity, "check") && !strutils::iequals(passivity, "ignore")
                    && !strutils::iequals(passivity, "enforce"))
                bad_value(p.first, passivity, "check, ignore or enforce");
        }
        else if (kw == "DELAY")
        {
            if (p.second.is_number())
            {
                delay_mode = SParamDeviceBase::DELAY_FIXED;
                delay_s = p.second.as_double();
                if (!(delay_s >= 0) || !std::isfinite(delay_s))
                {
                    cerr << "Error (" << name << "): delay must be >= 0 (got " << delay_s << " s)" << endl;
                    exit(1);
                }
            }
            else
            {
                const string v = string_kw(p);
                if (strutils::iequals(v, "auto")) delay_mode = SParamDeviceBase::DELAY_AUTO;
                else if (strutils::iequals(v, "none")) delay_mode = SParamDeviceBase::DELAY_NONE;
                else bad_value(p.first, v, "auto, none or a delay in seconds");
            }
        }
        else {
            cerr << "Unknown keyword: " << p.first << endl;
            exit(1);
        }
    }
    if (file.empty())
    {
        cerr << "Error (" << name << "): missing file=\"...\" (the S-parameter file)" << endl;
        exit(1);
    }

    const string path = resolve_file(file);
    std::shared_ptr<const SParamData> data = SParamData::load(path, format, convention, interp);
    specsGlobalConfig.record_data_file(path); // run manifest (F-01)

    // Ports of the data, in net order
    vector<size_t> port_map;
    if (!ports.empty())
    {
        // comma-separated names or 1-based indices
        vector<string> items(1);
        for (char ch : ports)
        {
            if (ch == ',')
                items.emplace_back();
            else if (!isspace(static_cast<unsigned char>(ch)))
                items.back() += ch;
        }
        for (const auto &p : items)
        {
            const int idx = data->portIndex(p);
            if (idx < 0)
            {
                cerr << "Error (" << name << "): no port '" << p << "' in " << path << " (ports:";
                for (const auto &pn : data->portNames())
                    cerr << " '" << pn << "'";
                cerr << ")" << endl;
                exit(1);
            }
            port_map.push_back(static_cast<size_t>(idx));
        }
    }
    else
    {
        for (size_t k = 0; k < data->ports(); ++k)
            port_map.push_back(k);
    }
    if (port_map.size() != nets.size())
    {
        cerr << "Error (" << name << "): " << nets.size() << " nets for " << port_map.size()
             << (ports.empty() ? " ports in " + path : " ports in ports=") << endl;
        exit(1);
    }
    for (size_t k = 0; k < port_map.size(); ++k)
        for (size_t m = 0; m < k; ++m)
            if (port_map[k] == port_map[m])
            {
                cerr << "Error (" << name << "): port '" << data->portNames()[port_map[k]] << "' is given twice in ports=" << endl;
                exit(1);
            }

    // Passivity (doc/sparam.md): power leaving the device for a unit input, 3 % above 1 (INTERCONNECT)
    if (!strutils::iequals(passivity, "ignore"))
    {
        const auto report = data->passivity();
        if (report.max_power_sum > 1.03)
        {
            cerr << "Warning (" << name << "): the S-parameters of " << path << " are not passive: for an input on port '"
                 << data->portNames()[report.input] << "', the output power is " << report.max_power_sum
                 << " times the input power at " << report.wavelength_m << " m"
                 << (strutils::iequals(passivity, "enforce") ? " (passivity=enforce scales S where it exceeds 1)" : "")
                 << endl;
        }
    }

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Unidirectional unless the number of nets is odd or a net is bidirectional (as COUPLER)
    bool bidirectional = nets.size() % 2 != 0;
    for (const auto &net : nets)
        bidirectional |= pt_helper.nets->at(net).bidirectional();

    // Unidirectional: light enters only the inputs (the outputs are only written), and what the data send
    // from an input back to an input (reflections, couplings between inputs) is not used; the largest such
    // path, if above -40 dB
    if (!bidirectional)
    {
        double largest = 0;
        size_t largest_out = 0, largest_in = 0;
        for (size_t o = 0; o < nets.size(); ++o)
            for (size_t i = 0; i < nets.size(); ++i)
            {
                if (writes_net(o) || writes_net(i))
                    continue;
                const double m = data->maxMagnitude(port_map[o], port_map[i]);
                if (m > largest)
                {
                    largest = m;
                    largest_out = o;
                    largest_in = i;
                }
            }
        if (largest * largest > 1e-4)
        {
            cerr << "Warning (" << name << "): unidirectional, the paths from an input (first half of the nets) to an "
                 << "input are not used; the largest, from " << nets[largest_in] << " to " << nets[largest_out]
                 << ", reaches " << 20 * log10(largest) << " dB (make a net bidirectional with & to use them)" << endl;
        }
    }

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, bidirectional);

    obj->m_data = data;
    obj->m_port_map = port_map;
    obj->m_delay_mode = delay_mode;
    obj->m_delay_s = delay_s;
    obj->m_enforce_passivity = strutils::iequals(passivity, "enforce");
    obj->m_from_netlist = true;
    return obj;
}

/** ******************************************* **/
/**             Subcircuit instance             **/
/** ******************************************* **/
INSTANTIATE_AND_CONNECT_UNI(XElement, pt_helper)
{
    (void)pt_helper;
    return nullptr;
}

sc_module *XElement::create(ParseTreeCreationHelper &pt_helper) const
{
    (void)pt_helper;
    cerr << "X element should never be called directly" << endl;
    exit(1);
    return nullptr;
}
