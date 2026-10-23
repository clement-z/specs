#pragma once

#include <systemc.h>
#include <map>

#include "devices/spx_module.h"
#include "specs.h"

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
/* Passive, reciprocal S-matrix (B-032), the same at every wavelength:
 *   through (in1->out1, in2->out2): t   (phase 0)
 *   cross   (in1->out2, in2->out1): j*x (in quadrature, as in the directional coupler)
 * with
 *   x = 10^(-|xtalk|/20), or x = 0 when the crosstalk is not given (NaN)
 *   t = 10^(-att/20) * sqrt(1 - x^2)
 * - att (m_attenuation_power_dB) is the excess loss in dB, on top of the power
 *   lost to crosstalk: |t|^2 = 10^(-att/10) (1 - |x|^2), so |t|^2 + |x|^2 <= 1
 *   for att >= 0 (= 1 for att = 0);
 * - xtalk (m_crosstalk_power_dB) is the cross power ratio out2/in1 in dB; its sign
 *   is ignored, i.e. a positive value is read as an isolation: xtalk = 20 and
 *   xtalk = -20 both put 1 % of the input power on the cross port.
 * The two columns of the S-matrix are orthogonal and have the same norm, so the
 * output power is (|t|^2 + |x|^2) times the input power for any input phases:
 * unitary for att = 0, and two coherent inputs never give more output power than
 * input power. Each wavelength propagates independently
 * (per-wavelength input memories, B-033). */
class CrossingBase : public spx_module {
public:
    // Member variables
    double m_attenuation_power_dB;
    double m_crosstalk_power_dB;

    // Constructor with crosstalk
    // Attenuation: excess loss (dB), on top of the power lost to crosstalk
    // Crosstalk: out2/in1 power ratio (dB) when there's nothing in in2, sign ignored;
    // NAN (default): no crosstalk
    CrossingBase(sc_module_name name,
                       double attenuation_power_dB = 0,
                       double crosstalk_power_dB = NAN)
        : spx_module(name)
        , m_attenuation_power_dB(attenuation_power_dB)
        , m_crosstalk_power_dB(crosstalk_power_dB)
    {
    }

    // Field coefficients of the S-matrix (see above), the same in both directions:
    // through = t, cross = j*x
    void s_parameters(OpticalSignal::field_type &through, OpticalSignal::field_type &cross) const;

    virtual void start_of_simulation();

protected:
    // F-15: s_parameters(), computed once before the simulation (the device threads computed
    // them when they started)
    OpticalSignal::field_type m_through;
    OpticalSignal::field_type m_cross;
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

    // Memory for multi-wavelength purposes
    std::map<uint32_t,OpticalSignal::field_type> m_memory_in1;
    std::map<uint32_t,OpticalSignal::field_type> m_memory_in2;

    // Processes (methods)
    void on_port_in1_changed();
    void on_port_in2_changed();
    // --vci only: prints the parameters at initialization (a thread, so that the devices print
    // in the order their threads used to)
    void print_initialization();

    // Write both outputs at one wavelength, from the input memories
    void write_outputs(uint32_t wavelength_id,
                       const OpticalSignal::field_type &through,
                       const OpticalSignal::field_type &cross);

    // Constructor with crosstalk
    // Attenuation: excess loss (dB), on top of the power lost to crosstalk
    // Crosstalk: out2/in1 power ratio (dB) when there's nothing in in2, sign ignored;
    // NAN (default): no crosstalk
    CrossingUni(sc_module_name name,
                       double attenuation_power_dB = 0,
                       double crosstalk_power_dB = NAN)
        : CrossingBase(name, attenuation_power_dB, crosstalk_power_dB)
        , m_out1_writer("out1_delayed_writer", p_out1)
        , m_out2_writer("out2_delayed_writer", p_out2)
    {
        SC_METHOD(on_port_in1_changed);
        sensitive << p_in1;
        dont_initialize();
        if (specsGlobalConfig.verbose_component_initialization)
            SC_THREAD(print_initialization);

        SC_METHOD(on_port_in2_changed);
        sensitive << p_in2;
        dont_initialize();
    }
};

typedef CrossingUni Crossing;

class CrossingBi : public CrossingBase {
public:
    // Ports (p0 = in1, p1 = in2, p2 = out1, p3 = out2)
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

    // Memory for multi-wavelength purposes
    std::map<uint32_t,OpticalSignal::field_type> m_memory_in0;
    std::map<uint32_t,OpticalSignal::field_type> m_memory_in1;
    std::map<uint32_t,OpticalSignal::field_type> m_memory_in2;
    std::map<uint32_t,OpticalSignal::field_type> m_memory_in3;

    // Processes (methods)
    void on_p0_in_changed();
    void on_p1_in_changed();
    void on_p2_in_changed();
    void on_p3_in_changed();
    // --vci only: prints the parameters at initialization (a thread, so that the devices print
    // in the order their threads used to)
    void print_initialization();

    // Write the forward (p2, p3) or backward (p0, p1) outputs at one wavelength,
    // from the input memories
    void write_forward_outputs(uint32_t wavelength_id,
                               const OpticalSignal::field_type &through,
                               const OpticalSignal::field_type &cross);
    void write_backward_outputs(uint32_t wavelength_id,
                                const OpticalSignal::field_type &through,
                                const OpticalSignal::field_type &cross);

    // Constructor with crosstalk
    // Attenuation: excess loss (dB), on top of the power lost to crosstalk
    // Crosstalk: out2/in1 power ratio (dB) when there's nothing in in2, sign ignored;
    // NAN (default): no crosstalk
    CrossingBi(sc_module_name name,
                       double attenuation_power_dB = 0,
                       double crosstalk_power_dB = NAN)
        : CrossingBase(name, attenuation_power_dB, crosstalk_power_dB)
        , m_p0_out_writer("p0_out_delayed_writer", p0_out)
        , m_p1_out_writer("p1_out_delayed_writer", p1_out)
        , m_p2_out_writer("p2_out_delayed_writer", p2_out)
        , m_p3_out_writer("p3_out_delayed_writer", p3_out)
    {
        SC_METHOD(on_p0_in_changed);
        sensitive << p0_in;
        dont_initialize();
        if (specsGlobalConfig.verbose_component_initialization)
            SC_THREAD(print_initialization);

        SC_METHOD(on_p1_in_changed);
        sensitive << p1_in;
        dont_initialize();

        SC_METHOD(on_p2_in_changed);
        sensitive << p2_in;
        dont_initialize();

        SC_METHOD(on_p3_in_changed);
        sensitive << p3_in;
        dont_initialize();
    }
};
