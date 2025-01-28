#include "parser/parse_element.h"
#include "devices/alldevices.h"
#include "specs.h"
#include "utils/strutils.h"

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
    for (const auto &net : nets)
        pt_helper.upgrade_signal(net);

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
    for (const auto &net : nets)
        pt_helper.upgrade_signal(net);

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
    // TODO: change from field to power?
    cout << "Warning: coupling power for DC currently set to field." << endl;

    element_type_base *obj = nullptr;

    // Verify number of nets
    assert(nets.size() == n_nets);

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

    // Parse positional arguments
    if(args.size() > 0)
        // convert from cross-coupling field coef
        obj->m_dc_through_coupling_power = 1 - pow(args[0].as_double(), 2);
    if(args.size() > 1)
        obj->m_dc_loss = args[1].as_double();

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "K" || kw == "KF" || kw == "KFIELD")
            // convert from cross-coupling field coef
            obj->m_dc_through_coupling_power = 1 - pow(p.second.as_double(), 2);
        else if (kw == "KP" || kw == "KPOW" || kw == "KPOWER")
            // convert from cross-coupling field coef
            obj->m_dc_through_coupling_power = 1 - p.second.as_double();
        else if (kw == "T")
            // convert from cross-coupling field coef
            obj->m_dc_through_coupling_power = pow(p.second.as_double(), 2);
        else if (kw == "LOSS")
            obj->m_dc_loss = p.second.as_double();
        else {
            cerr << "Unknown keyword: " << p.first << endl;
            exit(1);
        }
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
    pt_helper.upgrade_signal(nets[0]);
    pt_helper.upgrade_signal(nets[1]);

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

    cout << "Warning (" << name << "): default coupling power for DC currently set to field." << endl;

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
    assert(args.size() <= 2);

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
    pt_helper.upgrade_signal(nets[0]);
    pt_helper.upgrade_signal(nets[1]);
    pt_helper.upgrade_signal(nets[2]);
    pt_helper.upgrade_signal(nets[3]);

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

    cout << "Warning (" << name << "): default coupling power for DC currently set to field." << endl;

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

    assert(args.size() <= 1);
    // Parse positional arguments
    if(args.size() > 0)
        // convert from cross-coupling field coef
        obj->m_ps_sens_rad_v = args[0].as_double();
    // TODO: other positional args

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "SENSITIVITY")
            // convert from cross-coupling field coef
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
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_out1, nets[2], AS_READER);
    pt_helper.connect_uni<spx::oa_signal_type>(obj->p_out2, nets[3], AS_READER);

    // return the module
    return obj;
}

INSTANTIATE_AND_CONNECT_BI(CrossingElement, pt_helper)
{
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

    // Bidirectional crossing needs all optical nets as bidir
    pt_helper.upgrade_signal(nets[0]);
    pt_helper.upgrade_signal(nets[1]);
    pt_helper.upgrade_signal(nets[2]);
    pt_helper.upgrade_signal(nets[3]);

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
    assert(args.size() <= 2);

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, false);

    // Parse positional arguments
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
    assert(args.size() <= 3);

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, false);

    // Parse positional arguments
    if(args.size() > 1)
        // convert from cross-coupling field coef
        obj->setWavelength(args[1].as_double());
    if(args.size() > 2)
        obj->setPower(args[2].as_double());
    if(args.size() > 3)
        obj->setPhase(args[3].as_double());

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "WL" || kw == "WAVELENGTH")
            // convert from cross-coupling field coef
            obj->setWavelength(p.second.as_double());
        else if (kw == "POWER")
            // convert from cross-coupling field coef
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
    assert(args.size() <= 1);

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, false);

    // Parse positional arguments
    if(args.size() > 1)
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
    assert(args.size() <= 1);

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, false);

    // Parse positional arguments
    if(args.size() > 1)
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

    // Verify number of positional args
    assert(args.size() <= 4);

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
                obj->m_trace_wavelength = args[i].as_boolean();
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
            obj->m_trace_wavelength = p.second.as_boolean();
        else {
            cerr << name << ": unknown keyword: " << p.first << endl;
            exit(1);
        }
    }
    cout<< "created " << obj->name() << endl;
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
    assert(args.size() <= 1);

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
    assert(args.size() == 0);

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
    assert(args.size() <= 2);

    // Create signals if they don't exist
    pt_helper.create_signals(this);

    // Create the object and connect ports to signals
    INSTANTIATE(pt_helper, false);

    // Parse positional arguments
    if(args.size() > 0)
        obj->m_meltEnergy = args[0].as_double();
    if(args.size() > 1)
        obj->m_nStates = args[1].as_integer();
    if(args.size() > 2)
        obj->m_Tc = args[2].as_double();
    if(args.size() > 3)
        obj->m_Ta = args[3].as_double();

    // Parse keyword arguments
    for (auto &p: kwargs)
    {
        string kw = p.first;
        strutils::toupper(kw);
        if (kw == "MELT_ENERGY" || kw == "EMELT" || kw == "E_MELT")
            obj->m_meltEnergy = p.second.as_double();
        else if (kw == "N" || kw == "NSTATES" || kw == "NLEVELS" || kw == "N_STATES" || kw == "N_LEVELS")
            obj->m_nStates = p.second.as_integer();
        else if (kw == "K" || kw == "INITIAL_STATE")
            obj->m_stateCurrent = p.second.as_integer();
        else if (kw == "SP" || kw == "TC" || kw == "T_C")
            obj->m_Tc = p.second.as_double();
        else if (kw == "EP" || kw == "TA" || kw == "T_A")
            obj->m_Ta = p.second.as_double();
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
    cout << "-----" << endl;
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
    assert(args.size() <= 2);

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
        else {
            cerr << "Unknown keyword: " << p.first << endl;
            exit(1);
        }
    }
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
