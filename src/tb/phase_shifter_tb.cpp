#include <ctime>
#include <iomanip>
#include "tb/phase_shifter_tb.h"

#include "utils/general_utils.h"
#include "specs.h"

void ps_tb::run_1()
{
    V_PS.write(1.0);
    wait(SC_ZERO_TIME);
    IN1->write(OpticalSignal(sqrt(1.0), 1550e-9));
    IN2->write(OpticalSignal(sqrt(1.0), 1660e-9));
    wait(100,SC_MS);
    V_PS.write(2.0);
    wait(100, SC_MS);

    while (true) { wait(); }
}

void ps_tb::monitor()
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
            << "\tOUT: " << OUT->read() << std::endl
            << "\tCOUNT: " << event_counter << std::endl;

        if(event_counter == 2)
        {
            if (is_close(abs(OUT->read().m_field), 0.630210, 1e-4))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 0.630210W as output field!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }
        }
        if(event_counter == 4)
        {
            if (is_close(arg(OUT->read().m_field), 2, 1e-4))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 2 rad as output phase!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }
        }
        if(event_counter == 5)
        {
            if (is_close(arg(OUT->read().m_field), 2, 1e-4))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 2 as output phase!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }

            std::cout << "-----------------/! \\---------------" << std::endl;
            std::cout << "Test finished!" << std::endl;
            std::cout << "Success rate: " << success_counter << "/" << test_number << std::endl;
            std::cout << "-----------------/! \\---------------" << std::endl;
        }
    }

}

void ps_tb_run()
{
    // Apply SPECS resolution before creating any device
    specsGlobalConfig.applyEngineResolution();

    spx::oa_signal_type IN1, IN2, OUT, YOUT;
    spx::ea_signal_type V_PS;

    Merger merg1("merg1");
    merg1.p_in1(IN1);
    merg1.p_in2(IN2);
    merg1.p_out(YOUT);

    PhaseShifter ps1("ps1", 1);
    ps1.p_in(YOUT);
    ps1.p_vin(V_PS);
    ps1.p_out(OUT);

    // Connect testbench to uut
    ps_tb tb("tb");
    tb.IN1(IN1);
    tb.IN2(IN2);
    tb.OUT(OUT);
    tb.V_PS(V_PS);

    // Attach probes
    MLambdaProbe probe_out("out", {1550e-9, 1660e-9});
    probe_out.p_in(OUT);

    // Open Trace file
    std::string trace_filename = "traces/";
    trace_filename += "ps_tb";
    specsGlobalConfig.trace_filename = trace_filename;

    // Apply SPECS options specific to the testbench
    specsGlobalConfig.simulation_mode = OpticalOutputPortMode::EVENT_DRIVEN;
    specsGlobalConfig.trace_all_optical_nets = 0;

    // Run SPECS pre-simulation code
    specsGlobalConfig.prepareSimulation();

    // Start simulation
    sc_start();

    std::cout << std::endl << std::endl;
    std::cout << ".vcd trace file: " << specsGlobalConfig.trace_filename << std::endl;

    sc_close_vcd_trace_file(specsGlobalConfig.default_trace_file);
}
