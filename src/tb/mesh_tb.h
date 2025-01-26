#pragma once

#include "optical_signal.h"
#include <systemc.h>
#include "devices/probe.h"
#include "specs.h"
#include "devices/clements.h"

SC_MODULE(mesh_tb)
{
public:
    spx::oa_port_out_type IN;
    spx::oa_port_in_type OUT1, OUT2, OUT3, OUT4, OUT5;
    sc_out<double> vphi,vtheta;

    void run_1();
    void monitor();
    SC_CTOR(mesh_tb)
    {
        SC_HAS_PROCESS(mesh_tb);

        SC_THREAD(run_1);

        SC_THREAD(monitor);
        sensitive << IN << OUT1 << OUT2 << OUT3 << OUT4 << OUT5 << vphi << vtheta;
    }
};

void mesh_tb_run();
