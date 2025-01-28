#pragma once

#include <systemc.h>

#include "devices/spx_module.h"

#include "optical_output_port.h"
#include "optical_signal.h"


/* A waveguide crossing that accounts for */
/* loss and interchannel crosstalk        */
/*                                        */
/*           (2)p_in2                     */
/*                |                       */
/*    (1)p_in1----+---->p_out1(3)         */
/*                |                       */
/*                v                       */
/*              p_out2(4)                 */
/*                                        */
/* ---------------------------------------*/
class CrossingBase : public spx_module {
public:
    // Member variables
    double m_attenuation_power_dB;
    double m_crosstalk_power_dB;

    // Constructor with crosstalk
    // Attenuation relates to out1/in1 when there's nothing in in2
    // Crosstalk relates to out2/in1 when there's nothing in in1
    CrossingBase(sc_module_name name,
                       double attenuation_power_dB = 0,
                       double crosstalk_power_dB = NAN)
        : spx_module(name)
        , m_attenuation_power_dB(attenuation_power_dB)
        , m_crosstalk_power_dB(crosstalk_power_dB)
    {
    }
};

class CrossingUni : public CrossingBase {
public:
    // Ports
    spx::oa_port_in_type p_in1;
    spx::oa_port_in_type p_in2;
    spx::oa_port_out_type p_out1;
    spx::oa_port_out_type p_out2;

    // Timed ports writers
    OpticalOutputPort m_out1_writer;
    OpticalOutputPort m_out2_writer;

    // Processes
    void on_input_changed();

    // Constructor with crosstalk
    // Attenuation relates to out1/in1 when there's nothing in in2
    // Crosstalk relates to out2/in1 when there's nothing in in1
    CrossingUni(sc_module_name name,
                       double attenuation_power_dB = 0,
                       double crosstalk_power_dB = NAN)
        : CrossingBase(name, attenuation_power_dB, crosstalk_power_dB)
        , m_out1_writer("out1_delayed_writer", p_out1)
        , m_out2_writer("out2_delayed_writer", p_out2)
    {
        SC_HAS_PROCESS(CrossingUni);

        SC_THREAD(on_input_changed);
        sensitive << p_in1 << p_in2;
    }
};

typedef CrossingUni Crossing;

class CrossingBi : public CrossingBase {
public:
    // Ports
    spx::oa_port_in_type p0_in;
    spx::oa_port_in_type p1_in;
    spx::oa_port_in_type p2_in;
    spx::oa_port_in_type p3_in;
    spx::oa_port_out_type p0_out;
    spx::oa_port_out_type p1_out;
    spx::oa_port_out_type p2_out;
    spx::oa_port_out_type p3_out;

    // Timed ports writers
    OpticalOutputPort m_p0_out_writer;
    OpticalOutputPort m_p1_out_writer;
    OpticalOutputPort m_p2_out_writer;
    OpticalOutputPort m_p3_out_writer;

    // Processes
    void on_input_changed();

    // Constructor with crosstalk
    // Attenuation relates to out1/in1 when there's nothing in in2
    // Crosstalk relates to out2/in1 when there's nothing in in1
    CrossingBi(sc_module_name name,
                       double attenuation_power_dB = 0,
                       double crosstalk_power_dB = NAN)
        : CrossingBase(name, attenuation_power_dB, crosstalk_power_dB)
        , m_p0_out_writer("p0_out_delayed_writer", p0_out)
        , m_p1_out_writer("p1_out_delayed_writer", p1_out)
        , m_p2_out_writer("p2_out_delayed_writer", p2_out)
        , m_p3_out_writer("p3_out_delayed_writer", p3_out)
    {
        SC_HAS_PROCESS(CrossingBi);

        SC_THREAD(on_input_changed);
        sensitive << p0_in << p1_in << p2_in << p3_in;
    }
};