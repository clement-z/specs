#pragma once

#include <directional_coupler.h>
#include <optical_signal.h>
#include <systemc.h>
#include <probe.h> 
#include <specs.h>

SC_MODULE(DirectionalCoupler_tb)
{
public:
    spx::oa_port_out_type IN1;
    spx::oa_port_out_type IN2;
    spx::oa_port_in_type OUT1;
    spx::oa_port_in_type OUT2;

    void run_1();
    void monitor();
    SC_CTOR(DirectionalCoupler_tb)
    {
        SC_HAS_PROCESS(DirectionalCoupler_tb);

        SC_THREAD(run_1);

        SC_THREAD(monitor);
        sensitive << IN1 << IN2 << OUT1 << OUT2;
    }
};

void DirectionalCoupler_tb_run();

