#include <ctime>
#include <iomanip>
#include "tb/freqsweep_tb.h"

#include <cstdlib>
#include <unistd.h>

#include "utils/general_utils.h"

/* ----------------------------------------------------------------------------- *
    This testbench is called freqsweep because it is capable of this
    type of simulation, but it is also possible to set it as a time-domain
    simulation depending on the mode chosen when calling SPECS. Example:

    specs -t ac_add_drop -m fd     -> will perform the frequency sweep
    specs -t ac_add_drop -m td     -> will perform the time domain simulation

/  ----------------------------------------------------------------------------- */

void freqsweep_tb::run_1()
{
    if (specsGlobalConfig.simulation_mode == OpticalOutputPortMode::FREQUENCY_DOMAIN)
    {
        auto lambda_center = 1550.3e-9;
        auto lambda_span = 1e-9;
        auto dlambda = 0.002e-9;
        auto lambda_min = lambda_center - lambda_span/2;
        auto lambda_max = lambda_center + lambda_span/2;
        // lambda_min = 1550.250e-9;
        // lambda_max = 1550.303e-9;
        //wait(lambda_min * 1e9, SC_SEC);
        
        wait(SC_ZERO_TIME);
        cout << "----------------------------" <<endl;
        cout << "Starting sweep" << endl;
        for (auto lambda = lambda_min; lambda < lambda_max + dlambda; lambda += dlambda)
        {
            cout << setprecision(7) << lambda * 1e9 << "nm" << endl;
            IN->write(OpticalSignal(1, lambda));
            wait(dlambda, SC_SEC);
        }
    } else {
        for (int i = 0; i < 1; ++i)
        {
            auto lambda = 1550.302e-9;
            IN->write(OpticalSignal(1,lambda));
            wait(1, SC_NS);
            IN->write(OpticalSignal(0, lambda));
            wait(1, SC_NS);
        }
    }
    
    while (true) { wait(); }
}

void freqsweep_tb::monitor()
{
    while(true)
    {
        wait();
        continue;
    }
}

void freqsweep_tb_run_add_drop()
{
    specsGlobalConfig.applyEngineResolution();
    //specsGlobalConfig.oop_configs[0]->m_mode = OpticalOutputPortMode::EVENT_DRIVEN;
    //specsGlobalConfig.oop_configs[0]->m_mode = OpticalOutputPortMode::FREQUENCY_DOMAIN;

    // First test for the LBR paper to DAC
    // Single ring resonator, compare the response to theoretical
    // Also compare time of simulation wrt photontorch for same ring.

    // Characteristics:
    // Add-drop ring
    // Internal wg length (cm):
    // Internal wg loss (dB/cm): 
    // Internal wg neff:
    // Internal wg ng: 
    // DC ratio : 0.15 cross, 0.85 through
    // DC coupling loss (dB) : 0
    // Total length for resonance: lambda/2neff
    double neff = 1.0;
    double loss_db_cm = 1.0;
    double coupling_through = 0.85;

    spx::oa_signal_type IN, T_OUT, X_OUT, X_TERM;
    spx::oa_signal_type INNER_RING[4];

    freqsweep_tb tb1("tb1");
    tb1.IN(IN);
    tb1.OUT(X_OUT);

    DirectionalCoupler dc1("dc1", coupling_through, 0);
    dc1.p_in1(INNER_RING[0]);
    dc1.p_out1(INNER_RING[1]);
    dc1.p_in2(IN);
    dc1.p_out2(T_OUT);

    Waveguide wg1("wg1", 100.0*1550.0e-6/(2*neff), loss_db_cm, neff, neff);
    wg1.p_in(INNER_RING[1]);
    wg1.p_out(INNER_RING[2]);

    Waveguide wg2("wg2", 100.0*1550.0e-6/(2*neff), loss_db_cm, neff, neff);
    wg2.p_in(INNER_RING[3]);
    wg2.p_out(INNER_RING[0]);

    DirectionalCoupler dc2("dc2", coupling_through, 0);
    dc2.p_in1(X_TERM);
    dc2.p_out1(X_OUT);
    dc2.p_in2(INNER_RING[2]);
    dc2.p_out2(INNER_RING[3]);

    Probe pthrough("ptrough");
    pthrough.p_in(T_OUT);

    Probe pcross("pcross");
    pcross.p_in(X_OUT);

    // Open Trace file
    std::string trace_filename = "traces/";
    trace_filename += "freqsweep_tb";
    specsGlobalConfig.trace_filename = trace_filename;

    // Apply SPECS options specific to the testbench
    // could have forced frequency domain like this, but taking command line input instead
    // specsGlobalConfig.simulation_mode = OpticalOutputPortMode::FREQUENCY_DOMAIN;
    specsGlobalConfig.trace_all_optical_nets = 0;

    // Run SPECS pre-simulation code
    specsGlobalConfig.prepareSimulation();

    // Start simulation
    sc_start();

    std::cout << std::endl << std::endl;
    std::cout << ".vcd trace file: " << specsGlobalConfig.trace_filename << std::endl;

    sc_close_vcd_trace_file(specsGlobalConfig.default_trace_file);
}
