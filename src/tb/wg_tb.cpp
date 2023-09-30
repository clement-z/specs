#include <ctime>
#include <iomanip>
#include <tb/wg_tb.h>

#include "general_utils.h"
#include "devices/generic_waveguide.h"
#include "specs.h"

void wg_tb::run_1()
{
    IN->write(OpticalSignal(sqrt(1.0), 1550e-9));
    wait(100, SC_MS);
    IN->write(OpticalSignal(0, 1550e-9));
    wait(100, SC_MS);
    IN->write(OpticalSignal(sqrt(2.0), 1550e-9));
    wait(100, SC_MS);
    IN->write(OpticalSignal(0, 1550e-9));
    wait(10, SC_PS);
    IN->write(OpticalSignal(sqrt(3.0), 1550e-9));
    wait(100, SC_MS);

    while (true) { wait(); }
}

void wg_tb::monitor()
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
            if (is_close(norm(OUT->read().m_field), 0.5, 1e-4))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 0.5W as output power!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }
        }
        if(event_counter == 6)
        {
            if (is_close(arg(OUT->read().m_field), -2.4322, 1e-4))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected -2.4322 rad as output phase!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }
        }
        if(event_counter == 10)
        {
            if (is_close(norm(OUT->read().m_field), 1.5, 1e-4) && (is_close(arg(OUT->read().m_field), -2.4322, 1e-4)))
                success_counter++;
            else
            {
                std::cout << "-----------------/! \\---------------" << std::endl;
                std::cout << "Failure!" << std::endl;
                std::cout << "Expected 1.5W as output power and -2.4322 as output phase!" << std::endl;
                std::cout << "-----------------/! \\---------------" << std::endl;
            }

            std::cout << "-----------------/! \\---------------" << std::endl;
            std::cout << "Test finished!" << std::endl;
            std::cout << "Success rate: " << success_counter << "/" << test_number << std::endl;
            std::cout << "-----------------/! \\---------------" << std::endl;
        }
    }

}

void wg_tb_run()
{
    // Apply SPECS resolution before creating any device
    specsGlobalConfig.applyEngineResolution();

    spx::oa_signal_type IN("sig_in"), OUT("sig_out");
    spx::oa_signal_type TERM_r, TERM_w;

    // Unidirectional variant
    // Waveguide uut("uut", 1, 0, 1, 1);
    // uut.m_attenuation_dB_cm = 3.0103;
    // uut.p_in(IN);
    // uut.p_out(OUT);

    // Bidirectional variant
    WaveguideBi uut("uut", 1, 0, 1, 1);
    uut.m_attenuation_dB_cm = 3.0103;
    uut.p0_in(IN);
    uut.p0_out(TERM_w);
    uut.p1_in(TERM_r);
    uut.p1_out(OUT);

    // GenericWaveguide uut("uut",3.0103*100,1,1,0.01);
    // uut.ports_in[0]->bind(IN);
    // uut.ports_in[1]->bind(TERM_r);
    // uut.ports_out[0]->bind(TERM_w);
    // uut.ports_out[1]->bind(OUT);

    // Connect testbench to uut
    wg_tb tb("tb");
    tb.IN(IN);
    tb.OUT(OUT);

    // Attach probes
    Probe probe_in("in");
    probe_in.p_in(IN);

    Probe probe_out("out");
    probe_out.p_in(OUT);

    // Open Trace file
    std::string trace_filename = "traces/";
    trace_filename += "waveguide_tb";
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
