#pragma once

#include <systemc.h>
#include "devices/octane_segment.h"
#include "devices/crossing.h"
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

class OctaneMatrix : public sc_module {
public:
    // Ports
    /** The optical input ports. */
    vector<unique_ptr<port_in_type>> p_in_r;
    vector<unique_ptr<port_in_type>> p_in_c;

    /** The optical output ports. */
    vector<unique_ptr<port_out_type>> p_out;

    /** The electric output ports in matrix format*/
    // TODO: verify if a matrix library should be used
    vector<vector<unique_ptr<sc_out<double>>>> p_readout;

    // Define the size of the matrix
    std::size_t m_rows;
    std::size_t m_columns;

    bool m_isCompact;

    // For internal octane_cell
    double m_meltEnergy;
    int m_nStates;

    // Wires
    vector<unique_ptr<sc_signal<OpticalSignal>>> m_optical_connect_horizontal;
    vector<unique_ptr<sc_signal<OpticalSignal>>> m_optical_connect_vertical;

    // Member submodules
    vector<unique_ptr<Crossing>> m_cross;
    vector<unique_ptr<OctaneSegment>> m_segment;

    //Init() must be called after declaration
    void init();
    void init_ports();
    void connect_submodules();

    // auxiliar functions for readability
    // they were written with 0 to N-1 indexing in mind!
    inline bool isFirstRow(unsigned long i){return (i==0);}
    bool isFirstCol(unsigned long j){return (j==0);}
    bool isLastRow(unsigned long i){return (i==m_rows-1);}
    bool isLastCol(unsigned long j){return (j==m_columns-1);}

    /** Constructor for Octane Matrix
     *
     * @param name name of the module
     * @param meltEnergy energy needed to amorphize by one step
     * @param nStates number of internal states supported by the PCM
     * */
    OctaneMatrix(sc_module_name name, std::size_t rows, std::size_t columns,
                    double meltEnergy = 10e-12, int nStates = 63, bool isCompact = false)
        : sc_module(name)
        , m_rows(rows)
        , m_columns(columns)
        , m_isCompact(isCompact)
        , m_meltEnergy(meltEnergy)
        , m_nStates(nStates)
    {

        assert(rows > 0);
        assert(columns > 0);

        init_ports(); // At least the ports should always be
        // in the constructor
    }

};
