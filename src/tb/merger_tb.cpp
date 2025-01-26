#include <ctime>
#include <iomanip>
#include "tb/merger_tb.h"

#include "utils/general_utils.h"

void Merger_tb::run_1()
{
    IN1->write(OpticalSignal(1.0, 1550e-9));
    wait(100, SC_MS);
    IN2->write(OpticalSignal(1.0, 1550e-9));
    wait(100, SC_MS);
    IN1->write(OpticalSignal(sqrt(2.0), 1550e-9));
    IN2->write(OpticalSignal(sqrt(2.0), 1550e-9));
    wait(100, SC_MS);
}

void Merger_tb::monitor()
{
    unsigned int event_counter = 0;
    unsigned int success_counter = 0;
    const unsigned int test_number = 4;
    while(true)
    {
        wait();
        event_counter++;
        std::cout << sc_time_stamp() << ":" << std::endl
                << "\tIN1:  " << IN1->read() << std::endl
                << "\tIN2:  " << IN2->read() << std::endl
                << "\tOUT: " << OUT->read() << std::endl
                << "\tCOUNT: " << event_counter << std::endl;
        if(event_counter == 2)
        {
            if (is_close(norm(OUT->read().m_field), 0.5, 1e-4))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 0.5W as output power!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }
            if (is_close(arg(OUT->read().m_field), 0, 1e-4))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 0 as output phase!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }
        }
        if(event_counter == 4)
        {
            if (is_close(norm(OUT->read().m_field), 2, 1e-4))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 2W as output power!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }
        }
        if(event_counter == 6)
        {
            if (is_close(norm(OUT->read().m_field), 4, 1e-4))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 2W as output power!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }

            std::cout << "-----------------/! \\---------------" << std::endl;
            std::cout << "Test finished!" << std::endl;
            std::cout << "Success rate: " << success_counter << "/" << test_number << std::endl;
            std::cout << "-----------------/! \\---------------" << std::endl;
        }
    }
}

void Merger_tb_run()
{
    // Apply SPECS resolution before creating any device
    specsGlobalConfig.applyEngineResolution();

    spx::oa_signal_type IN1, IN2, OUT;

    Merger uut("uut", 3.0103);
    uut.m_attenuation_dB = 0;
    uut.p_in1(IN1);
    uut.p_in2(IN2);
    uut.p_out(OUT);

    Merger_tb tb("tb");
    tb.IN1(IN1);
    tb.IN2(IN2);
    tb.OUT(OUT);

    // Open Trace file
    // sc_trace_file *Tf;
    std::string trace_filename = "traces/";
    trace_filename += "merger_tb";
    specsGlobalConfig.trace_filename = trace_filename;

    // Apply SPECS options specific to the testbench
    specsGlobalConfig.simulation_mode = OpticalOutputPortMode::EVENT_DRIVEN;
    specsGlobalConfig.trace_all_optical_nets = 0;

    Probe probe_merger("probe_merger",specsGlobalConfig.default_trace_file);
    probe_merger.p_in(OUT);

    // Run SPECS pre-simulation code
    specsGlobalConfig.prepareSimulation();

    // Start simulation
    sc_start();

    std::cout << std::endl << std::endl;
    std::cout << ".vcd trace file: " << specsGlobalConfig.trace_filename << std::endl;

    sc_close_vcd_trace_file(specsGlobalConfig.default_trace_file);
}
