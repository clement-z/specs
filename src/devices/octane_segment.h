#pragma once

#include <systemc.h>
#include "devices/waveguide.h"
#include "devices/directional_coupler.h"
#include "devices/octane_cell.h"
#include "optical_output_port.h"
#include "optical_signal.h"

// TODO: remove these (are in spx_module.h)
typedef sc_signal_in_if<OpticalSignal > if_in_type;
typedef sc_signal_out_if<OpticalSignal > if_out_type;
typedef sc_port<if_in_type> port_in_type;
typedef sc_port<if_out_type> port_out_type;


/** Building block for the OCTANE circuit
 * The current implementation includes all
 * intermediate components allowing modelling of imperfections,
 * but at the cost of increased module count.
 *
 * A simpler way would be to disregard all intermediate waveguides
 * and model their effects on `octane_cell`
 *
 * Maybe I could add an argument to init(bool isCompact), defining
 * if the model will be compact or not.
*/

//TODO: pass element's characteristics as argument, for instance, pass an example of waveguide,
// or an example of DC, PCM such that it copies. Requires: constructors in each element to build
// by copying.

class OctaneSegment : public sc_module {
public:
    // Ports
    /** The optical input ports. */
    vector<unique_ptr<port_in_type>> p_in;

    /** The optical output ports. */
    vector<unique_ptr<port_out_type>> p_out;

    /** The electric output port*/
    unique_ptr<sc_out<double>> p_readout;

    // Member variables
    // Defines the number of ports and elements
    bool m_term_row;
    bool m_term_col;
    bool m_isCompact;

    // Defines the coupling in the DCs (should change as more elements are added)
    double m_coupling_through_row;
    double m_coupling_through_col;

    // For internal octane_cell
    double m_meltEnergy;
    int m_nStates;


    // Wires
    vector<unique_ptr<sc_signal<OpticalSignal>>> m_optical_connect;

    // Member submodules
    vector<unique_ptr<Waveguide>> m_wg;
    vector<unique_ptr<DirectionalCoupler>> m_dc;
    unique_ptr<OctaneCell> m_oct;

    // init() should be called after declaration
    void init();
    void init_compact();
    void init_full();
    void init_ports();
    void connect_submodules_full();
    void connect_submodules_compact();

    /** Constructor for Octane Segment
     *
     * @param name name of the module
     * @param meltEnergy energy needed to amorphize by one step
     * @param nStates number of internal states supported by the PCM
     * */
    OctaneSegment(sc_module_name name, bool term_row, bool term_col,
                    double coupling_through_row, double coupling_through_col,
                    double meltEnergy, int nStates, bool isCompact = false)
        : sc_module(name)
        , m_term_row(term_row)
        , m_term_col(term_col)
        , m_isCompact(isCompact)
        , m_coupling_through_row(coupling_through_row)
        , m_coupling_through_col(coupling_through_col)
        , m_meltEnergy(meltEnergy)
        , m_nStates(nStates)
    {
        init_ports(); // At least the ports should always be
        // in the constructor
    }

};
