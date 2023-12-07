#pragma once

#include "alldevices.h"
#include "power_meter.h"
#include "subcircuit_instance.h"
#include "parse_tree.h"
#include "specs.h"

#include <initializer_list>
#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iostream>

using std::pair;
using std::make_pair;
using std::string;
using std::vector;
using std::map;
using std::cout;
using std::cerr;
using std::endl;
using std::pair;
using std::unique_ptr;

/** ******************************************* **/
/**  Helper macros for declaring devices        **/
/** ******************************************* **/
#define DECLARE_UNIDIR_ELEMENT(ELEM_NAME, ELEM_NAME_LONG, MOD_CLASS_PREFIX, N_NETS)    \
    struct ELEM_NAME : public ParseElement {                                           \
        typedef MOD_CLASS_PREFIX element_type_base;                                    \
        typedef MOD_CLASS_PREFIX element_type_uni;                                     \
                                                                                       \
        const size_t n_nets = N_NETS;                                                  \
                                                                                       \
        /* Import constructor from ParseElement */                                     \
        using ParseElement::ParseElement;                                              \
                                                                                       \
        virtual ParseElement *clone() const                                            \
        { return new ELEM_NAME(*this); }                                               \
        /* Implement virtual function create(...) to construct element */              \
        virtual sc_module *create(ParseTreeCreationHelper &pt_helper) const;           \
        virtual element_type_base *                                                    \
        instantiate_and_connect_uni(ParseTreeCreationHelper &pt_helper) const;         \
                                                                                       \
        virtual string kind() const { return ELEM_NAME_LONG; }                         \
        virtual bool bidir_capable() const { return false; }                           \
    };

/*************************************************/

#define DECLARE_BIDIR_ELEMENT(ELEM_NAME, ELEM_NAME_LONG, MOD_CLASS_PREFIX, N_NETS)     \
                                                                                       \
    struct ELEM_NAME : public ParseElement {                                           \
        typedef MOD_CLASS_PREFIX##Base element_type_base;                              \
        typedef MOD_CLASS_PREFIX##Uni element_type_uni;                                \
        typedef MOD_CLASS_PREFIX##Bi element_type_bi;                                  \
                                                                                       \
        const size_t n_nets = N_NETS;                                                  \
                                                                                       \
        /* Import constructor from ParseElement */                                     \
        using ParseElement::ParseElement;                                              \
                                                                                       \
        virtual ParseElement *clone() const                                            \
        { return new ELEM_NAME(*this); }                                               \
        /* Implement virtual function create(...) to construct element */              \
        virtual sc_module *create(ParseTreeCreationHelper &pt_helper) const;           \
        virtual element_type_base *                                                    \
        instantiate_and_connect_uni(ParseTreeCreationHelper &pt_helper) const;         \
        virtual element_type_base *                                                    \
        instantiate_and_connect_bi(ParseTreeCreationHelper &pt_helper) const;          \
                                                                                       \
        virtual string kind() const { return string(ELEM_NAME_LONG) + " (bidir)"; }    \
        virtual bool bidir_capable() const { return true; }                            \
    };

/** ******************************************* **/
/**            Devices declarations             **/
/** ******************************************* **/
DECLARE_BIDIR_ELEMENT(WGElement, "WAVEGUIDE", Waveguide, 2);
DECLARE_BIDIR_ELEMENT(DCElement, "DIRECTIONAL COUPLER", DirectionalCoupler, 4);
DECLARE_UNIDIR_ELEMENT(MergerElement, "MERGER", Merger, 3);
DECLARE_UNIDIR_ELEMENT(SplitterElement, "SPLITTER", Splitter, 3);
DECLARE_BIDIR_ELEMENT(PhaseShifterElement, "PHASE SHIFTER", PhaseShifter, 3);
DECLARE_BIDIR_ELEMENT(MZIElement, "MZI MODULATOR", MZIActive, 5);
//DECLARE_UNIDIR_ELEMENT(MZIElement, "MZI MODULATOR (2 PHASE-SHIFTER)", MZI, 6);
DECLARE_BIDIR_ELEMENT(CrossingElement, "CROSSING", Crossing, 4);
DECLARE_UNIDIR_ELEMENT(CWSourceElement, "CW SOURCE", CWSource, 1);
DECLARE_UNIDIR_ELEMENT(VLSourceElement, "VALUE LIST SOURCE (OPTICAL)", VLSource, 1);
DECLARE_UNIDIR_ELEMENT(EVLSourceElement, "VALUE LIST SOURCE (ELECTRICAL)", EVLSource, 1);

// TODO: make the following bidirectional
DECLARE_UNIDIR_ELEMENT(PCMCellElement, "PCM CELL", PCMElement, 2);
DECLARE_UNIDIR_ELEMENT(PhotodetectorElement, "PHOTODETECTOR", Detector, 2);
DECLARE_UNIDIR_ELEMENT(ProbeElement, "PROBE", Probe, 1);
DECLARE_UNIDIR_ELEMENT(MLProbeElement, "MULTIWAVELENGTH PROBE", MLambdaProbe, 1);
DECLARE_UNIDIR_ELEMENT(PowerMeterElement, "POWER METER", PowerMeter, 1);

// TODO: take care of subcircuit instance...
DECLARE_UNIDIR_ELEMENT(XElement, "SUBCIRCUIT", SubcircuitInstance, 1);

/** ******************************************* **/
/**        Undefine macros                      **/
/** ******************************************* **/
#undef DECLARE_UNIDIR_ELEMENT
#undef DECLARE_BIDIR_ELEMENT
