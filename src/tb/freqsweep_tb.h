#pragma once

#include "optical_signal.h"
#include <systemc.h>
#include "devices/waveguide.h"
#include "devices/directional_coupler.h"
#include "devices/probe.h"
#include "devices/crow.h"
#include "specs.h"

SC_MODULE(freqsweep_tb)
{
public:
    spx::oa_port_out_type IN;
    spx::oa_port_in_type OUT;

    void run_1();
    void monitor();
    SC_CTOR(freqsweep_tb)
    {
        SC_HAS_PROCESS(freqsweep_tb);

        SC_THREAD(run_1);

        SC_THREAD(monitor);
        sensitive << IN << OUT;
    }
};

//void freqsweep_tb_run();
void freqsweep_tb_run_add_drop();
void freqsweep_tb_run_crow();

