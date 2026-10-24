#pragma once

#include <systemc.h>
#include <map>
#include "optical_output_port.h"
#include "optical_signal.h"
#include "utils/wavelength_map.h"
#include "devices/spx_module.h"
#include "specs.h"


class DirectionalCouplerBase : public spx_module {
public:
    // Member variables
    double m_delay_ns = 0;
    double m_through_phase_rad = 0;
    double m_cross_phase_rad = M_PI/2;
    double m_dc_through_coupling_power; // power ratio to the through ports, in [0, 1] (P_cross = 1 - this; netlist k/kp/t are converted to it)

    double m_through_power_dB;
    double m_cross_power_dB;
    double m_dc_loss; // insertion loss in dB (power), applied to both through and cross paths

    // Constructor
    DirectionalCouplerBase(sc_module_name name,
                       double dc_through_coupling_power = 0.5,
                       double dc_loss = 0)
        : spx_module(name)
        , m_dc_through_coupling_power(dc_through_coupling_power)
        , m_dc_loss(dc_loss)
    {
        // nothing to do
    }

    virtual void start_of_simulation();

protected:
    // F-15: computed once before the simulation (the device threads computed them when they
    // started): S-parameters of the through and cross paths, delay
    OpticalSignal::field_type m_S_through;
    OpticalSignal::field_type m_S_cross;
    sc_time m_delay;
};

class DirectionalCouplerUni : public DirectionalCouplerBase {
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
    WavelengthMap<OpticalSignal::field_type> m_memory_in1;
    WavelengthMap<OpticalSignal::field_type> m_memory_in2;

    // Processes (methods)
    void on_port_in1_changed();
    void on_port_in2_changed();
    // --vci only: prints the parameters at initialization (a thread, so that the devices print
    // in the order their threads used to)
    void print_initialization();

    // Constructor
    DirectionalCouplerUni(sc_module_name name,
                       double dc_through_coupling_power = 0.5,
                       double dc_loss = 0)
        : DirectionalCouplerBase(name, dc_through_coupling_power, dc_loss)
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

typedef DirectionalCouplerUni DirectionalCoupler;

class DirectionalCouplerBi : public DirectionalCouplerBase {
public:
    // Ports
    /** The optical input ports. */
    spx::oa_port_in_type p0_in;
    spx::oa_port_in_type p1_in;
    spx::oa_port_in_type p2_in;
    spx::oa_port_in_type p3_in;

    /** The optical output ports. */
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
    WavelengthMap<OpticalSignal::field_type> m_memory_in0;
    WavelengthMap<OpticalSignal::field_type> m_memory_in1;
    WavelengthMap<OpticalSignal::field_type> m_memory_in2;
    WavelengthMap<OpticalSignal::field_type> m_memory_in3;

    // Processes (methods)
    void on_p0_in_changed();
    void on_p1_in_changed();
    void on_p2_in_changed();
    void on_p3_in_changed();
    // --vci only: prints the parameters at initialization (a thread, so that the devices print
    // in the order their threads used to)
    void print_initialization();

    // Constructor
    DirectionalCouplerBi(sc_module_name name,
                       double dc_through_coupling_power = 0.5,
                       double dc_loss = 0)
        : DirectionalCouplerBase(name, dc_through_coupling_power, dc_loss)
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
