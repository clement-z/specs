#pragma once

#include <optical_signal.h>
#include <systemc.h>
#include <waveguide.h>
#include <probe.h>
#include <specs.h>

SC_MODULE(wg_tb)
{
public:
    spx::oa_port_out_type IN;
    spx::oa_port_in_type OUT;

    void run_1();
    void monitor();
    SC_CTOR(wg_tb)
    {
        SC_HAS_PROCESS(wg_tb);

        SC_THREAD(run_1);

        SC_THREAD(monitor);
        sensitive << IN << OUT;
    }
};

void wg_tb_run();
