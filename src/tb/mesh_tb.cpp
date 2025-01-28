#include <ctime>
#include <iomanip>
#include "tb/mesh_tb.h"

#include "utils/general_utils.h"

void mesh_tb::run_1()
{
    IN->write(OpticalSignal(sqrt(1.0), 1550e-9));
    vtheta.write(M_PI);
    vphi.write(1);
    wait(1, SC_NS);
    // vtheta.write(M_PI_2);
    // wait(1, SC_NS);
    // vtheta.write(M_PI);
    // wait(1, SC_NS);
    // vphi.write(1);
    // wait(1, SC_NS);

    while (true) { wait(); }
}

void mesh_tb::monitor()
{
    while(true)
    {
        wait();
        std::cout << sc_time_stamp() << ":" << std::endl
            << "\tIN:  " << IN->read() << std::endl
            << "\tPHI: " << vphi.read() << std::endl
            << "\tTHETA: " << vtheta.read() << std::endl
            << "\tOUT1: " << OUT1->read() << std::endl
            << "\tOUT2: " << OUT2->read() << std::endl
            << "\tOUT3: " << OUT3->read() << std::endl
            << "\tOUT4: " << OUT4->read() << std::endl
            << "\tOUT5: " << OUT5->read() << std::endl;
    }

}

void mesh_tb_run()
{
    // Apply SPECS resolution before creating any device
    specsGlobalConfig.applyEngineResolution();

    spx::oa_signal_type SIG, ZERO, OUT1, OUT2, OUT3, OUT4, OUT5;
    sc_signal<double> vphi,vtheta,e_zero;

    Clements mesh1("mesh1", 5, 1, 0, 0, 0, 0, 1);
    mesh1.p_in[0]->bind(SIG);
    mesh1.p_in[1]->bind(SIG);
    mesh1.p_in[2]->bind(SIG);
    mesh1.p_in[3]->bind(SIG);
    mesh1.p_in[4]->bind(SIG);
    mesh1.p_out[0]->bind(OUT1);
    mesh1.p_out[1]->bind(OUT2);
    mesh1.p_out[2]->bind(OUT3);
    mesh1.p_out[3]->bind(OUT4);
    mesh1.p_out[4]->bind(OUT5);

    mesh1.p_vphi[0]->bind(e_zero);
    mesh1.p_vtheta[0]->bind(vtheta);

    mesh1.p_vphi[1]->bind(e_zero);
    mesh1.p_vtheta[1]->bind(e_zero);

    mesh1.p_vphi[2]->bind(e_zero);
    mesh1.p_vtheta[2]->bind(vtheta);

    mesh1.p_vphi[3]->bind(e_zero);
    mesh1.p_vtheta[3]->bind(e_zero);

    mesh1.p_vphi[4]->bind(e_zero);
    mesh1.p_vtheta[4]->bind(e_zero);

    mesh1.p_vphi[5]->bind(e_zero);
    mesh1.p_vtheta[5]->bind(e_zero);

    mesh1.p_vphi[6]->bind(e_zero);
    mesh1.p_vtheta[6]->bind(e_zero);

    mesh1.p_vphi[7]->bind(e_zero);
    mesh1.p_vtheta[7]->bind(e_zero);

    mesh1.p_vphi[8]->bind(e_zero);
    mesh1.p_vtheta[8]->bind(e_zero);

    mesh1.p_vphi[9]->bind(e_zero);
    mesh1.p_vtheta[9]->bind(e_zero);
                      
    mesh1.init();

    mesh_tb tb("tb");
    tb.IN(SIG);
    tb.OUT1(OUT1);
    tb.OUT2(OUT2);
    tb.OUT3(OUT3);
    tb.OUT4(OUT4);
    tb.OUT5(OUT5);
    tb.vphi(vphi);
    tb.vtheta(vtheta);

    // Open Trace file
    std::string trace_filename = "traces/";
    trace_filename += "mesh_tb";
    specsGlobalConfig.trace_filename = trace_filename;

    // Apply SPECS options specific to the testbench
    specsGlobalConfig.simulation_mode = OpticalOutputPortMode::EVENT_DRIVEN;
    specsGlobalConfig.trace_all_optical_nets = 0;
    

    Probe probe_mzi_1("probe_mzi_1",specsGlobalConfig.default_trace_file);
    probe_mzi_1.p_in(OUT1);
    Probe probe_mzi_2("probe_mzi_2",specsGlobalConfig.default_trace_file);
    probe_mzi_2.p_in(OUT2);
    Probe probe_in("probe_in",specsGlobalConfig.default_trace_file);
    probe_in.p_in(SIG);

    // Run SPECS pre-simulation code
    specsGlobalConfig.prepareSimulation();

    // Start simulation
    sc_start();

    std::cout << std::endl << std::endl;
    std::cout << ".vcd trace file: " << specsGlobalConfig.trace_filename << std::endl;

    sc_close_vcd_trace_file(specsGlobalConfig.default_trace_file);
}
