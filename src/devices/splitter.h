#pragma once

#include <systemc.h>

#include <spx_module.h>
#include <optical_output_port.h>
#include <optical_signal.h>
#include "specs.h"
#include "spx_module.h"

/** A 1x2 waveguide splitter module.
 *
 * This device copies any output to its two output, with power values defines by the
 * split ratio and attenuation.
 * */
class Splitter : public spx_module {
public:
    // Ports
    /** The optical input port. */
    spx::oa_port_in_type p_in;
    /** The first optical output port. */
    spx::oa_port_out_type p_out1;
    /** The second optical output port. */
    spx::oa_port_out_type p_out2;

private:
    // Timed ports writers
    /** Timed writer to the first output port.
     *
     * @sa DelayedWriter
     * */
    OpticalOutputPort m_out1_writer;
    /** Timed writer to the second output port.
     *
     * @sa DelayedWriter
     * */
    OpticalOutputPort m_out2_writer;

public:
    // Member variables
    /** The power transmission from input to branch 1, exclusive of losses
     *
     * The power transmission from input to branch 2 is (1 - m_split_ratio)
     * */
    double m_split_ratio;

    /** The delay for an output to propagate to the outputs in ns.
     *
     * The delay is the same for both outputs.
     * */
    // static double m_delay_ns;
    /** The attenuation of the signal in dB.
     *
     * The attenuation is applied before the split (although it does not really make a
     * difference. */
    double m_attenuation_dB;

    // Processes
    /** Main process of the module.
     *
     * It copies the input to both output, after attenuation and delay.
     *
     *   **SystemC type:** thread
     *
     *   **Sensitivity list:** p_in
     * */
    void on_port_in_changed();

    // Constructor
    /** Constructor for Splitter
     *
     * @param name name of the module
     * @param split_ratio ratio of the light which goes to the first ouptut
     * (pre-attenuation)
     * */
    Splitter(sc_module_name name, double split_ratio = 0.5, double attenuation_dB = 0)
        : spx_module(name)
        , m_out1_writer("out1_delayed_writer", p_out1)
        , m_out2_writer("out2_delayed_writer", p_out2)
        , m_split_ratio(split_ratio)
        , m_attenuation_dB(attenuation_dB)
    {
        SC_HAS_PROCESS(Splitter);

        SC_THREAD(on_port_in_changed);
        sensitive << p_in;
    }
};
