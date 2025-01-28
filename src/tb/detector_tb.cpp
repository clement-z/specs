#include <ctime>
#include <iomanip>
#include "tb/detector_tb.h"

#include "utils/general_utils.h"

void Detector_tb::run_1()
{
    IN->write(OpticalSignal(sqrt(1.0), 1550e-9));
    wait(100, SC_MS);
    IN->write(OpticalSignal(0, 1550e-9));
    wait(100, SC_MS);
    IN->write(OpticalSignal(sqrt(1.0), 1551e-9));



    while (true) { wait(); }
}

void Detector_tb::monitor()
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
            << "\tREADOUT:  " << READOUT.read() << std::endl
            << "\tREADOUT2:  " << READOUT2.read() << std::endl
            << "\tCOUNT: " << event_counter << std::endl;

        if(event_counter == 2)
        {
            if ((is_close(READOUT.read(), 1, 1e-3)) && (is_close(READOUT2.read(), 1, 1e-3)))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 1A as both output currents!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }
        }
        if(event_counter == 4)
        {
            if ((is_close(READOUT.read(), 0, 1e-4)) && (is_close(READOUT2.read(), 0, 1e-4)))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 0A as output currents!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }
        }
        if(event_counter == 6)
        {
            if (!is_close(READOUT.read(), READOUT2.read(), 1e-7))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected slightly different currents (different noise seed)!" << std::endl;
                std::cout << "Is the noise ON?" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }

            std::cout << "-----------------/! \\---------------" << std::endl;
            std::cout << "Test finished!" << std::endl;
            std::cout << "Success rate: " << success_counter << "/" << test_number << std::endl;
            std::cout << "-----------------/! \\---------------" << std::endl;
        }

    }
}

void Detector_tb_run()
{
    // Apply SPECS resolution before creating any device
    specsGlobalConfig.applyEngineResolution();

    spx::oa_signal_type IN;
    sc_signal<double> READOUT, READOUT2;

    Detector uut("uut", 1, 100e-12, false);
    uut.p_in(IN);
    uut.p_readout(READOUT);

    Detector uut2("uut2", 1, 100e-12, false);
    uut2.p_in(IN);
    uut2.p_readout(READOUT2);

    Detector_tb tb("tb");
    tb.IN(IN);
    tb.READOUT(READOUT);
    tb.READOUT2(READOUT2);

    // Open Trace file
    std::string trace_filename = "traces/";
    trace_filename += "detector_tb";
    specsGlobalConfig.trace_filename = trace_filename;

    // Apply SPECS options specific to the testbench
    specsGlobalConfig.simulation_mode = OpticalOutputPortMode::EVENT_DRIVEN;
    specsGlobalConfig.trace_all_optical_nets = 0;

    Probe probe_detector("probe_detector",specsGlobalConfig.default_trace_file);
    probe_detector.p_in(IN);

    // Run SPECS pre-simulation code
    specsGlobalConfig.prepareSimulation();

    // custom traces should come after prepareSimulation !!
    sc_trace(specsGlobalConfig.default_trace_file, READOUT, "READOUT");

    // Start simulation
    sc_start();

    std::cout << std::endl << std::endl;
    std::cout << ".vcd trace file: " << specsGlobalConfig.trace_filename << std::endl;

    sc_close_vcd_trace_file(specsGlobalConfig.default_trace_file);
}
