#include <ctime>
#include <iomanip>
#include <tb/directional_coupler_tb.h>

#include "general_utils.h"
#include "generic_2x2_coupler.h"

void DirectionalCoupler_tb::run_1()
{
    IN1->write(OpticalSignal(1.0, 1550e-9));
    wait(100, SC_MS);
    IN2->write(OpticalSignal(1.0, 1550e-9));
    wait(100, SC_MS);

    while (true) { wait(); }
}

void DirectionalCoupler_tb::monitor()
{
    unsigned int event_counter = 0;
    unsigned int success_counter = 0;
    const unsigned int test_number = 3;

    while(true)
    {
        wait();
        event_counter++;
        std::cout << sc_time_stamp() << ":" << std::endl
            << "\tIN1:  " << IN1->read() << std::endl
            << "\tIN2:  " << IN2->read() << std::endl
            << "\tOUT1: " << OUT1->read() << std::endl
            << "\tOUT2: " << OUT2->read() << std::endl
            << "\tCOUNT: " << event_counter << std::endl;

        if(event_counter == 2)
        {
            if (is_close(norm(OUT1->read().m_field), 0.05, 1e-4) && is_close(norm(OUT2->read().m_field), 0.45, 1e-4))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 0.05W/0.45 as output power!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }
            if (is_close(arg(OUT2->read().m_field), 1.5708, 1e-4))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected pi/2 as phase in OUT2!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }
        }
        if(event_counter == 4)
        {
            if (is_close(arg(OUT1->read().m_field), 1.2490, 1e-4))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 1.2490 as phase in OUT1!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }

            std::cout << "-----------------/! \\---------------" << std::endl;
            std::cout << "Test finished!" << std::endl;
            std::cout << "Success rate: " << success_counter << "/" << test_number << std::endl;
            std::cout << "-----------------/! \\---------------" << std::endl;
        }
    }
}

void DirectionalCoupler_tb_run()
{
    // Apply SPECS resolution before creating any device
    specsGlobalConfig.applyEngineResolution();


    spx::oa_signal_type IN1, IN2, IN3, IN4, OUT1, OUT2;
    spx::oa_signal_type TERM_r, TERM_w;

    // Unidirectional variant
    // DirectionalCoupler uut("uut", 0.5, 0);
    // uut.m_dc_through_coupling_power = 0.1;
    // uut.m_dc_loss = 3.0103;

    // uut.p_in1(IN1);
    // uut.p_in2(IN2);
    // uut.p_out1(OUT1);
    // uut.p_out2(OUT2);

    // Bidirectional variant
    DirectionalCouplerBi uut("uut", 0.5, 0);
    uut.m_dc_through_coupling_power = 0.1;
    uut.m_dc_loss = 3.0103;

    uut.p0_in(IN1);
    uut.p0_out(TERM_w);
    uut.p1_in(IN2);
    uut.p1_out(TERM_w);
    uut.p2_in(TERM_r);
    uut.p2_out(OUT1);
    uut.p3_in(TERM_r);
    uut.p3_out(OUT2);

    // Generic bidirectional variant
    // Generic2x2Coupler uut("uut", 1-0.1, 3.0103);
    // uut.ports_in[0]->bind(IN1);
    // uut.ports_in[1]->bind(IN2);
    // uut.ports_in[2]->bind(TERM_r);
    // uut.ports_in[3]->bind(TERM_r);
    // uut.ports_out[0]->bind(TERM_w);
    // uut.ports_out[1]->bind(TERM_w);
    // uut.ports_out[2]->bind(OUT1);
    // uut.ports_out[3]->bind(OUT2);

    DirectionalCoupler_tb tb("tb");
    tb.IN1(IN1);
    tb.IN2(IN2);
    tb.OUT1(OUT1);
    tb.OUT2(OUT2);

    // Open Trace file
    std::string trace_filename = "traces/";
    trace_filename += "directional_coupler_tb";
    specsGlobalConfig.trace_filename = trace_filename;

    // Apply SPECS options specific to the testbench
    specsGlobalConfig.simulation_mode = OpticalOutputPortMode::EVENT_DRIVEN;
    specsGlobalConfig.trace_all_optical_nets = 0;

    Probe probe_dc_1("probe_dc_1",specsGlobalConfig.default_trace_file);
    probe_dc_1.p_in(OUT1);
    Probe probe_dc_2("probe_dc_2",specsGlobalConfig.default_trace_file);
    probe_dc_2.p_in(OUT2);

    // Run SPECS pre-simulation code
    specsGlobalConfig.prepareSimulation();

    // Start simulation
    sc_start();

    std::cout << std::endl << std::endl;
    std::cout << ".vcd trace file: " << specsGlobalConfig.trace_filename << std::endl;

    sc_close_vcd_trace_file(specsGlobalConfig.default_trace_file);
}
