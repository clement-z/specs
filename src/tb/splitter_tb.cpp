#include <ctime>
#include <iomanip>
#include <tb/splitter_tb.h>

#include "general_utils.h"

void Splitter_tb::run_1()
{
    IN->write(OpticalSignal(sqrt(1.0), 1550e-9));
    wait(100, SC_MS);
    IN->write(OpticalSignal(-sqrt(1.0), 1550e-9));
    wait(100, SC_MS);
    IN->write(OpticalSignal(sqrt(2.0), 1550e-9));
    wait(100, SC_MS);
    IN->write(OpticalSignal(-sqrt(2.0), 1550e-9));
    wait(10, SC_PS);
    IN->write(OpticalSignal(sqrt(3.0), 1550e-9));
    wait(100, SC_MS);

    wait(OUT1->value_changed_event());
    wait(1, SC_SEC);
    sc_stop();
}

void Splitter_tb::monitor()
{
    unsigned int event_counter = 0;
    unsigned int success_counter = 0;
    const unsigned int test_number = 3;
    while(true)
    {
        wait();
        event_counter++;
        std::cout << sc_time_stamp() << ":" << std::endl
                << "\tIN: " << IN->read() << std::endl
                << "\tOUT1:  " << OUT1->read() << std::endl
                << "\tOUT2:  " << OUT2->read() << std::endl
                << "\tCOUNT: " << event_counter << std::endl;

        if(event_counter == 10)
        {
            if (is_close(norm(OUT1->read().m_field), 0.75, 1e-4))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 0.75W as output power!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }

            if (is_close(norm(OUT1->read().m_field), norm(OUT2->read().m_field), 1e-4))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected same power in both ends!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }

            if (is_close(arg(OUT1->read().m_field), 0, 1e-4) && is_close(arg(OUT2->read().m_field), 0, 1e-4))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected zero phase in both ends!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }

            std::cout << "-----------------/! \\---------------" << std::endl;
            std::cout << "Test finished!" << std::endl;
            std::cout << "Success rate: " << success_counter << "/" << test_number << std::endl;
            std::cout << "-----------------/! \\---------------" << std::endl;
        }
    }
}

void Splitter_tb_run()
{
    // Apply SPECS resolution before creating any device
    specsGlobalConfig.applyEngineResolution();

    spx::oa_signal_type IN, OUT1, OUT2;

    Splitter uut("uut");
    uut.m_attenuation_dB = 3.0103;
    uut.p_in(IN);
    uut.p_out1(OUT1);
    uut.p_out2(OUT2);

    Splitter_tb tb("tb");
    tb.IN(IN);
    tb.OUT1(OUT1);
    tb.OUT2(OUT2);

    // Open Trace file
    std::string trace_filename = "traces/";
    trace_filename += "splitter_tb";
    specsGlobalConfig.trace_filename = trace_filename;

    // Apply SPECS options specific to the testbench
    specsGlobalConfig.simulation_mode = OpticalOutputPortMode::EVENT_DRIVEN;
    specsGlobalConfig.trace_all_optical_nets = 0;

    Probe probe_splitter_1("probe_splitter_1",specsGlobalConfig.default_trace_file);
    probe_splitter_1.p_in(OUT1);

    Probe probe_splitter_2("probe_splitter_2",specsGlobalConfig.default_trace_file);
    probe_splitter_2.p_in(OUT2);

    // Run SPECS pre-simulation code
    specsGlobalConfig.prepareSimulation();

    // Start simulation
    sc_start();

    std::cout << std::endl << std::endl;
    std::cout << ".vcd trace file: " << specsGlobalConfig.trace_filename << std::endl;

    sc_close_vcd_trace_file(specsGlobalConfig.default_trace_file);
}
