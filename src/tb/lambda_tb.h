#pragma once

#include "optical_signal.h"
#include <systemc.h>
#include "devices/waveguide.h"
#include "devices/merger.h"
#include "devices/directional_coupler.h"
#include "devices/cw_source.h"
#include "devices/probe.h"
#include "specs.h"

SC_MODULE(lambda_tb)
{
public:
    sc_in<OpticalSignal> OUT;
    // sc_out<OpticalSignal> IN1, IN2;

    void run_1();
    void monitor();
    SC_CTOR(lambda_tb)
    {
        SC_HAS_PROCESS(lambda_tb);

        SC_THREAD(run_1);

        SC_THREAD(monitor);
        sensitive << OUT;
    }
};

void lambda_tb_run();
