#pragma once

#include <systemc.h>

#include <optical_signal.h>
#include <pcm_device.h>
#include <probe.h>
#include <specs.h>

SC_MODULE(PCMElement_tb)
{
public:
    spx::oa_port_out_type IN;
    spx::oa_port_in_type OUT;

    void run_1();
    void monitor();
    SC_CTOR(PCMElement_tb)
    {
        SC_HAS_PROCESS(PCMElement_tb);

        SC_THREAD(run_1);

        SC_THREAD(monitor);
        sensitive << IN << OUT;
    }
};

void PCMElement_tb_run();
