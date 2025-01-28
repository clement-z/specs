#include <ctime>
#include <iomanip>
#include "tb/mzi_tb.h"

#include "utils/general_utils.h"

void mzi_tb::run_1()
{
    IN->write(OpticalSignal(sqrt(1.0), 1550e-9));
    wait(1, SC_NS);
    vtheta.write(M_PI_2);
    wait(1, SC_NS);
    vtheta.write(M_PI);
    wait(1, SC_NS);
    vphi.write(1);
    wait(1, SC_NS);

    while (true) { wait(); }
}

void mzi_tb::monitor()
{
    while(true)
    {
        wait();
        std::cout << sc_time_stamp() << ":" << std::endl
            << "\tIN:  " << IN->read() << std::endl
            << "\tPHI: " << vphi.read() << std::endl
            << "\tTHETA: " << vtheta.read() << std::endl
            << "\tOUT1: " << OUT1->read() << std::endl
            << "\tOUT2: " << OUT2->read() << std::endl;
    }

}

void MZI_tb_run()
{
    // Apply SPECS resolution before creating any device
    specsGlobalConfig.applyEngineResolution();

    spx::oa_signal_type IN1, IN2, OUT1, OUT2;
    spx::ea_signal_type vphi,vtheta;

    MZI mzi1("mzi1",0,0,0,0,0,1);
    mzi1.p_in1(IN1);
    mzi1.p_in2(IN2);
    mzi1.p_out1(OUT1);
    mzi1.p_out2(OUT2);
    mzi1.p_vphi(vphi);
    mzi1.p_vtheta(vtheta);

    mzi_tb tb("tb");
    tb.IN(IN1);
    tb.OUT1(OUT1);
    tb.OUT2(OUT2);
    tb.vphi(vphi);
    tb.vtheta(vtheta);

    // Open Trace file
    std::string trace_filename = "traces/";
    trace_filename += "mzi_tb";
    specsGlobalConfig.trace_filename = trace_filename;

    // Apply SPECS options specific to the testbench
    specsGlobalConfig.simulation_mode = OpticalOutputPortMode::EVENT_DRIVEN;
    specsGlobalConfig.trace_all_optical_nets = 0;


    Probe probe_mzi_1("probe_mzi_1",specsGlobalConfig.default_trace_file);
    probe_mzi_1.p_in(OUT1);
    Probe probe_mzi_2("probe_mzi_2",specsGlobalConfig.default_trace_file);
    probe_mzi_2.p_in(OUT2);
    Probe probe_in("probe_in",specsGlobalConfig.default_trace_file);
    probe_in.p_in(IN1);

    // Run SPECS pre-simulation code
    specsGlobalConfig.prepareSimulation();

    // Start simulation
    sc_start();

    std::cout << std::endl << std::endl;
    std::cout << ".vcd trace file: " << specsGlobalConfig.trace_filename << std::endl;

    sc_close_vcd_trace_file(specsGlobalConfig.default_trace_file);
}
