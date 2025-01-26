#pragma once

#include "optical_signal.h"
#include <systemc.h>
#include "devices/waveguide.h"
#include "devices/directional_coupler.h"
#include "devices/mzi.h"
#include "devices/probe.h"
#include "specs.h"

SC_MODULE(mzi_tb)
{
public:
    spx::oa_port_out_type IN;
    spx::oa_port_in_type OUT1, OUT2;
    sc_out<double> vphi,vtheta;

    void run_1();
    void monitor();
    SC_CTOR(mzi_tb)
    {
        SC_HAS_PROCESS(mzi_tb);

        SC_THREAD(run_1);

        SC_THREAD(monitor);
        sensitive << IN << OUT1 << OUT2 << vphi << vtheta;
    }
};

void MZI_tb_run();
