#pragma once

#include <systemc.h>
#include <chrono>
#include <iostream>

using std::chrono::system_clock;
using std::shared_ptr;
using std::make_shared;

class TimeMonitor : public sc_module {
public:
    double m_poll_period; // in seconds
    double m_wallclock_period; // in seconds

    void on_trigger();

    TimeMonitor(sc_module_name name, double poll_period=100e-12, double wallclock_period = 2)
        : sc_module(name),
        m_poll_period(poll_period),
        m_wallclock_period(wallclock_period)
    {
        SC_HAS_PROCESS(TimeMonitor);
        SC_THREAD(on_trigger);
    }
};
