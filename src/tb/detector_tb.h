#pragma once

#include "devices/detector.h"
#include "optical_signal.h"
#include <systemc.h>
#include "devices/probe.h"
#include "specs.h"
#include "devices/waveguide.h"

SC_MODULE(Detector_tb)
{
public:
    spx::oa_port_out_type IN;
    sc_in<double> READOUT, READOUT2;

    void run_1();
    void monitor();
    SC_CTOR(Detector_tb)
    {
        SC_HAS_PROCESS(Detector_tb);

        SC_THREAD(run_1);

        SC_THREAD(monitor);
        sensitive << IN << READOUT << READOUT2;
    }
};

void Detector_tb_run();

