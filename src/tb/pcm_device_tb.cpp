#include <ctime>
#include <iomanip>
#include <tb/pcm_device_tb.h>

#include "general_utils.h"

void PCMElement_tb::run_1()
{
    IN->write(OpticalSignal(sqrt(1.0e-3), 1.55e-6));
    wait(10, SC_MS);
    IN->write(OpticalSignal(sqrt(2*1e-3), 1.55e-6)); // to 2W
    wait(10, SC_MS);
    IN->write(OpticalSignal(0, 1.55e-6));
    wait(10, SC_MS);
    IN->write(OpticalSignal(sqrt(1.0e-3), 1.55e-6));
    wait(10, SC_MS);
    IN->write(OpticalSignal(sqrt(2*1.0e-3), 1.55e-6));
    wait(10, SC_MS);
    IN->write(OpticalSignal(0, 1.55e-6));
    wait(10, SC_MS);
    IN->write(OpticalSignal(sqrt(1.0e-3), 1.55e-6));


    while (true) { wait(); }
}

void PCMElement_tb::monitor()
{
    unsigned int event_counter = 0;
    unsigned int success_counter = 0;
    const unsigned int test_number = 3;
    while(true)
    {
        wait();
        event_counter++;
        std::cout << sc_time_stamp() << ":" << std::endl
            << "\tIN:  " << IN->read() << std::endl
            << "\tOUT: " << OUT->read() << std::endl
            << "\tCOUNT: " << event_counter << std::endl;

        if(event_counter == 2)
        {
            if (is_close(norm(OUT->read().m_field), 0.000850, 1e-8))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 0.850mW as output power!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }
        }
        if(event_counter == 8)
        {
            if (is_close(norm(OUT->read().m_field), 0.000854758, 1e-8))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 0.854mW as output power!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }
        }
        if(event_counter == 14)
        {
            if (is_close(norm(OUT->read().m_field), 0.000859495, 1e-8))
                success_counter++; 
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 0.859mW as output power!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }

            std::cout << "-----------------/! \\---------------" << std::endl;
            std::cout << "Test finished!" << std::endl;
            std::cout << "Success rate: " << success_counter << "/" << test_number << std::endl;
            std::cout << "-----------------/! \\---------------" << std::endl;
        }
    }
}

void PCMElement_tb_run()
{
    // Apply SPECS resolution before creating any device
    specsGlobalConfig.applyEngineResolution();

    spx::oa_signal_type IN, OUT;

    PCMElement pcm("pcm", 25e-6, 63, 0, 0.85, 0.95);
    pcm.p_in(IN);
    pcm.p_out(OUT);

    PCMElement_tb tb("tb");
    tb.IN(IN);
    tb.OUT(OUT);

    // Open Trace file

    std::string trace_filename = "traces/";
    trace_filename += "pcm_device_tb";
    specsGlobalConfig.trace_filename = trace_filename;

    Probe probe_in("in",specsGlobalConfig.default_trace_file);
    probe_in.p_in(IN);

    Probe probe_out("out",specsGlobalConfig.default_trace_file);
    probe_out.p_in(OUT);

    // Apply SPECS options specific to the testbench
    specsGlobalConfig.simulation_mode = OpticalOutputPortMode::EVENT_DRIVEN;
    specsGlobalConfig.trace_all_optical_nets = 0;

    // Run SPECS pre-simulation code
    specsGlobalConfig.prepareSimulation();

    // extra traces should come after prepareSimulation
    sc_trace(specsGlobalConfig.default_trace_file, pcm.m_stateCurrent, "STATE");

    // Start simulation
    sc_start();

    std::cout << std::endl << std::endl;
    std::cout << ".vcd trace file: " << specsGlobalConfig.trace_filename << std::endl;

    sc_close_vcd_trace_file(specsGlobalConfig.default_trace_file);
}
