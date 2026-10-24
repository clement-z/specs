#pragma once

#include <systemc.h>

#include "optical_signal.h"

// An optical net (spx::oa_signal_type): an sc_signal that can also be written with a value-change
// event when the value does not change (B-003, B-185). A plain sc_signal notifies only a new value,
// so a port that writes the value its net already holds wakes nothing downstream. writeWithEvent()
// is used by OpticalOutputPort for the first emission of each wavelength at a .dc point
// (OpticalOutputPort::m_emit_each_wavelength); every other write is the plain sc_signal write
class OpticalNet : public sc_signal<OpticalSignal, SC_MANY_WRITERS> {
public:
    typedef sc_signal<OpticalSignal, SC_MANY_WRITERS> base_type;

    OpticalNet()
        : base_type()
    {}

    explicit OpticalNet(const char *name_)
        : base_type(name_)
    {}

    OpticalNet(const char *name_, const OpticalSignal &initial_value_)
        : base_type(name_, initial_value_)
    {}

    using base_type::operator=;

    // Writes value; the update notifies its value-change event even if value equals the current one
    // (as sc_buffer does for every write)
    void writeWithEvent(const OpticalSignal &value)
    {
        m_force_event = true;
        write(value);
        request_update();
    }

protected:
    virtual void update() override
    {
        if (m_force_event)
        {
            m_force_event = false;
            policy_type::update();
            do_update();
        }
        else
            base_type::update();
    }

private:
    bool m_force_event = false;
};
