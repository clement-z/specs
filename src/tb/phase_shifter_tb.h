#pragma once

#include "optical_signal.h"
#include <systemc.h>
#include "devices/waveguide.h"
#include "devices/phaseshifter.h"
#include "devices/merger.h"
#include "devices/probe.h"
#include "specs.h"

SC_MODULE(ps_tb)
{
public:
    spx::oa_port_out_type IN1;
    spx::oa_port_out_type IN2;
    sc_out<double> V_PS;
    spx::oa_port_in_type OUT;

    void run_1();
    void monitor();
    SC_CTOR(ps_tb)
    {
        SC_HAS_PROCESS(ps_tb);

        SC_THREAD(run_1);

        SC_THREAD(monitor);
        sensitive << IN1 << IN2 << OUT;
    }
};

void ps_tb_run();
