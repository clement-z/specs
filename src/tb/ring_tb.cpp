#include <ctime>
#include <iomanip>
#include <tb/ring_tb.h>

#include "general_utils.h"

void ring_tb::run_1()
{
    IN.write(OpticalSignal(sqrt(1.0), 1550e-9));
    wait(10, SC_NS);
    IN.write(OpticalSignal(0, 1550e-9));
    wait(10, SC_NS);

    IN.write(OpticalSignal(sqrt(1.0), 1550e-9));
    wait(10, SC_NS);
    IN.write(OpticalSignal(0, 1550e-9));
    wait(10, SC_NS);

    while(true)
    {
        wait();
    }
}

void ring_tb::monitor()
{
    while(true)
    {
        wait();
        std::cout << sc_time_stamp() << ":" << std::endl
            << "\tIN:  " << IN.read() << std::endl
            << "\tOUT: " << OUT.read() << std::endl;
    }

}

void Ring_tb_run()
{
    // Apply SPECS resolution before creating any device
    specsGlobalConfig.applyEngineResolution();

    double neff = 1.0;
    double loss_db_cm = 0;
    double coupling_through = 0.85;

    spx::oa_signal_type IN, OUT;
    spx::oa_signal_type RING1, RING2;

    DirectionalCoupler dc1("dc1", coupling_through, 0);
    dc1.p_in1(IN);
    dc1.p_out1(OUT);
    dc1.p_in2(RING1);
    dc1.p_out2(RING2);

    Waveguide wg1("wg1", 1*100.0*1550.0e-6/(2*neff), loss_db_cm, neff, neff);
    wg1.p_in(RING2);
    wg1.p_out(RING1);

    ring_tb tb("tb");
    tb.IN(IN);
    tb.OUT(OUT);

    // Open Trace file
    std::string trace_filename = "traces/";
    trace_filename += "ring_tb";
    specsGlobalConfig.trace_filename = trace_filename;    

    // Apply SPECS options specific to the testbench
    specsGlobalConfig.simulation_mode = OpticalOutputPortMode::EVENT_DRIVEN;
    specsGlobalConfig.trace_all_optical_nets = 0;

    Probe probe_ring("probe_ring",specsGlobalConfig.default_trace_file);
    probe_ring.p_in(OUT);

    // Run SPECS pre-simulation code
    specsGlobalConfig.prepareSimulation();

    // Start simulation
    sc_start();

    std::cout << std::endl << std::endl;
    std::cout << ".vcd trace file: " << specsGlobalConfig.trace_filename << std::endl;

    sc_close_vcd_trace_file(specsGlobalConfig.default_trace_file);
}
