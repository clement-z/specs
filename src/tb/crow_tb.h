#pragma once

#include <optical_signal.h>
#include <specs.h>
#include <systemc.h>

class crow_tb : public sc_module {
public:
    spx::oa_port_out_type IN;
    spx::oa_port_out_type ADD;
    spx::oa_port_in_type THROUGH;
    spx::oa_port_in_type DROP;

    void run_fd();
    void run_td();
    void monitor();

    SC_CTOR(crow_tb)
    {
        SC_THREAD(run_fd);
        SC_THREAD(run_td);
        SC_THREAD(monitor);
        if (false)
            sensitive << IN << ADD << THROUGH << DROP;
    }
};

void crow_tb_run();

extern size_t nrings_crow;
