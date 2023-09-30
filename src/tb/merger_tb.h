#pragma once

#include <merger.h>
#include <optical_signal.h>
#include <systemc.h>
#include <probe.h>
#include <specs.h>

SC_MODULE(Merger_tb)
{
public:
    spx::oa_port_out_type IN1;
    spx::oa_port_out_type IN2;
    spx::oa_port_in_type OUT;

    void run_1();
    void monitor();
    SC_CTOR(Merger_tb)
    {
        SC_HAS_PROCESS(Merger_tb);

        SC_THREAD(run_1);

        SC_THREAD(monitor);
        sensitive << IN1 << IN2 << OUT;
    }
};

void Merger_tb_run();

