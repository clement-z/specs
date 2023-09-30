#pragma once

#include <systemc.h>
#include <fstream>

#include <optical_output_port.h>
#include <optical_signal.h>
#include "specs.h"
#include "spx_module.h"

class CWSource : public spx_module {
public:
    // Ports
    spx::oa_port_out_type p_out;

    // Timed ports writers
    OpticalOutputPort m_out_writer;

    // Signal characteristic
    spx::oa_value_type m_signal_on;
    double m_source_wavelength;

    // Source emission control
    spx::ed_signal_type enable;
    spx::ed_signal_type reset;

    // Processes
    void runner();

    inline void setWavelength(const double &wl)
    {
        m_source_wavelength = wl;
        m_signal_on.m_wavelength_id = m_signal_on.getIDFromWavelength(wl);
    }
    inline void setFrequency(const double &f)
    {
        setWavelength(299792458.0 / f);
    }
    inline void setAmplitudePhase (const double &amplitude, const double &phase)
    {
        m_signal_on.m_field = polar(amplitude, phase);
    }
    inline void setPhase (const double &phase)
    {
        if (m_signal_on.modulus() == 0)
            cerr << "Warning: setting phase has no effect on 0" << endl;
        m_signal_on.m_field = polar(m_signal_on.modulus(), phase);
    }
    inline void setPower (const double &power)
    {
        m_signal_on.m_field = polar(sqrt(power), m_signal_on.phase());
    }
    inline void setPower(const double &power, double phase)
    {
        setAmplitudePhase(sqrt(power), phase);
    }

    // Constructor
    CWSource(sc_module_name name)
        : spx_module(name)
        , m_out_writer("out_delayed_writer", p_out)
    {
        SC_HAS_PROCESS(CWSource);

        SC_THREAD(runner);

        enable = sc_logic(0);
        reset = sc_logic(0);
    }

    CWSource(sc_module_name name, const OpticalSignal& signal_on)
        : CWSource(name)
    {
        m_signal_on = signal_on;
    }
};
