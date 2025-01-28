#pragma once

#include <systemc.h>
#include "devices/waveguide.h"
#include "devices/pcm_device.h"
#include "devices/merger.h"
#include "devices/detector.h"
#include "optical_output_port.h"
#include "optical_signal.h"

/** The minimal octane cell composed of merger, waveguides, PCM and a detector. */
/** For now, all photonic parameters will be internal and constant. The only */
/** one that can be changed is the PCM levels.*/

//TODO: pass element's characteristics as argument, for instance, pass an example of waveguide,
// or an example of DC, PCM such that it copies. Requires: constructors in each element to build
// by copying.

class OctaneCell : public sc_module {
public:
    // Ports
    /** The optical input ports. */
    sc_port<sc_signal_in_if<OpticalSignal>> p_in_r, p_in_c;
    /** The electrical output port. */
    sc_out<double> p_readout;

    // Member variables
    double m_meltEnergy;
    bool m_isCompact;
    int m_nStates;

    // Wires
    sc_signal<OpticalSignal> m_merg_in1, m_merg_in2, m_merg_out, m_pcm_in, m_pcm_out, m_pd_in;

    // Member submodules
    unique_ptr<Waveguide> m_wg1, m_wg2, m_wg3, m_wg4;
    unique_ptr<Merger> m_merger;
    unique_ptr<PCMElement> m_pcm;
    unique_ptr<Detector> m_pd;

    // This function should be called right after
    // Instantiation (either declaration or make_shared or new)
    void init();
    void init_compact();
    void init_full();

    /** Constructor for Waveguide
     *
     * @param name name of the module
     * @param meltEnergy energy needed to amorphize by one step
     * @param nStates number of internal states supported by the PCM
     * */
    OctaneCell(sc_module_name name, double meltEnergy = 10e-12, int nStates = 63, bool isCompact = false)
        : sc_module(name)
        , m_meltEnergy(meltEnergy)
        , m_isCompact(isCompact)
        , m_nStates(nStates)
    {
    }

};
