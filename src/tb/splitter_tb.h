#pragma once

#include <optical_signal.h>
#include <splitter.h>
#include <systemc.h>
#include <probe.h>
#include <specs.h>

SC_MODULE(Splitter_tb)
{
public:
    spx::oa_port_out_type IN;
    spx::oa_port_in_type OUT1;
    spx::oa_port_in_type OUT2;

    void run_1();
    void monitor();
    SC_CTOR(Splitter_tb)
    {
        SC_HAS_PROCESS(Splitter_tb);

        SC_THREAD(run_1);

        SC_THREAD(monitor);
        sensitive << IN << OUT1 << OUT2;
    }
};

void Splitter_tb_run();
