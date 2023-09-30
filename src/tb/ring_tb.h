#pragma once

#include <optical_signal.h>
#include <systemc.h>
#include <waveguide.h>
#include <directional_coupler.h>
#include <probe.h>
#include <specs.h>

SC_MODULE(ring_tb)
{
public:
    sc_out<OpticalSignal> IN;
    sc_in<OpticalSignal> OUT;

    void run_1();
    void monitor();
    SC_CTOR(ring_tb)
    {
        SC_HAS_PROCESS(ring_tb);

        SC_THREAD(run_1);

        SC_THREAD(monitor);
        sensitive << IN << OUT;
    }
};

void Ring_tb_run();
