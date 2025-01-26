#include <ctime>
#include <iomanip>
#include "tb/crow_tb.h"

#include <cstdlib>
#include <unistd.h>
#include <chrono>

#include "devices/waveguide.h"
#include "devices/directional_coupler.h"
#include "devices/probe.h"
#include "devices/crow.h"
#include "specs.h"
#include "devices/time_monitor.h"

#include "utils/general_utils.h"

using namespace std;
using namespace std::chrono;


/* ----------------------------------------------------------------------------- *
    This testbench is called freqsweep because it is capable of this
    type of simulation, but it is also possible to set it as a time-domain
    simulation depending on the mode chosen when calling SPECS. Example:

    specs -t ac_add_drop -m fd     -> will perform the frequency sweep
    specs -t ac_add_drop -m td     -> will perform the time domain simulation

/  ----------------------------------------------------------------------------- */

// ---------------- Use this variable to change the size of the CROW -----------
                            size_t nrings_crow = 3;
// -----------------------------------------------------------------------------

void crow_tb::run_td()
{
    bool verbose = true;
    if (specsGlobalConfig.simulation_mode == OpticalOutputPortMode::TIME_DOMAIN)
    {
        int npulses = 4;
        double lambda = 1554.3719e-9;
        lambda = 1.561931948453715e-6;
        lambda = 1538.3277504e-9;
        lambda = 1543e-9;
        lambda = 1556.5742279e-9;
        lambda = 1556.32782563158025368466e-9;
        lambda = 1556e-9;
        lambda = 1553e-9;
        //lambda = 1550e-9;
        double tpulse = 1e-9;
        double deadtime = tpulse;

        
        // Wait one tick that all sc_threads are started and on their first `wait` call
        wait(SC_ZERO_TIME);

        if (verbose)
        {
            cout << "----------------------------" <<endl;
            cout << "Starting time-domain simulation" << endl;
        }
        for (int i = 0; i < npulses; ++i)
        {
            IN->write(OpticalSignal(1, lambda));
            wait(tpulse, SC_SEC);
            // //ADD.write(OpticalSignal(polar<double>(1, M_PI_2), lambda));
            // //ADD.write(OpticalSignal(polar<double>(0,0), lambda));
            IN->write(OpticalSignal(0, lambda));
            // ADD.write(OpticalSignal(0, lambda));
            wait(deadtime, SC_SEC);
        }
    }
    
    while (true) { wait(); }
}

void crow_tb::run_fd()
{
    bool verbose = true;
    if (specsGlobalConfig.simulation_mode == OpticalOutputPortMode::FREQUENCY_DOMAIN)
    {
        auto lambda_center = 1550e-9;
        auto lambda_span = 2e-9;
        auto dlambda = 0.001e-9;
        auto lambda_min = lambda_center - lambda_span/2;
        auto lambda_max = lambda_center + lambda_span/2;
        // lambda_min = 1545e-9;
        // lambda_max = 1569e-9;
        lambda_min = 299792458.0/195e12;
        lambda_max = 299792458.0/190e12;
        lambda_min = 1556e-9;
        lambda_span = lambda_max - lambda_min;
        dlambda = lambda_span / 10000;
        
        // Wait one tick that all sc_threads are started and on their first `wait` call
        wait(SC_ZERO_TIME);
        //wait(lambda_min, SC_SEC);

        auto tic = high_resolution_clock::now();
        auto toc = high_resolution_clock::now();

        if (verbose)
        {
            cout << "----------------------------" <<endl;
            cout << "Starting sweep" << endl;
        }
        int i = 0;
        int n = ceil(lambda_span / dlambda);
        for (auto lambda = lambda_min; lambda < lambda_max + dlambda; lambda += dlambda)
        {
            ++i;
            if (true || (verbose && i % (int)(n/20) == 0))
            {
                toc = high_resolution_clock::now();
                auto duration = duration_cast<microseconds>(toc - tic);
                tic = toc;
                cout << duration.count()/1000.0 << "ms" << endl;
                cout << endl;
                cout << fixed << setprecision(1) << (lambda-lambda_min)/lambda_span * 100.0 << "%" << endl;
                cout << setprecision(20) << lambda * 1e9 << "nm" << endl;
            }
            IN->write(OpticalSignal(sqrt(1), lambda));
            ADD->write(OpticalSignal(0, lambda));
            //ADD.write(OpticalSignal(polar<double>(0, 0), lambda));
            //ADD.write(OpticalSignal(polar<double>(sqrt(0.5), -M_PI_2), lambda));
            wait(dlambda, SC_SEC);
        }
        if (verbose)
        {
            cout << "----------------------------" <<endl;
            cout << "Sweep over (" << n << " points)" << endl;
        }
    }
    while (true) { wait(); }
}

void crow_tb::monitor()
{
    unsigned long long event_counter = 0;
    while(true)
    {
        wait();
        continue;
        event_counter++;
        std::cout << sc_time_stamp() << ":" << std::endl
            << "\tIN:      " << IN->read() << std::endl
            << "\tADD:     " << ADD->read() << std::endl
            << "\tTHROUGH: " << THROUGH->read() << std::endl
            << "\tDROP:    " << DROP->read() << std::endl
            << "\tCOUNT:   " << event_counter << std::endl;
    }

}

void crow_tb_run()
{
    specsGlobalConfig.applyEngineResolution();

    spx::oa_signal_type IN("IN"), THROUGH("THROUGH"), DROP("DROP"), ADD("ADD");

    crow_tb tb1("tb1");
    tb1.IN(IN);
    tb1.ADD(ADD);
    tb1.THROUGH(THROUGH);
    tb1.DROP(DROP);

    // pid_t pid = fork();
    CROW *pc;
    // if(pid)
    pc = new CROW("crow", nrings_crow);
    // else
    //     pc = new Crow("crow", 5);
    pc->p_in(IN);
    pc->p_add(ADD);
    pc->p_out_t(THROUGH);
    pc->p_out_d(DROP);

    pc->setRingLength(2*30e-6);
    pc->m_loss_db_cm = 2.0;
    pc->m_coupling_through = 1 - pow(0.83645, 2.0);
    // pc->m_coupling_through = 0.5;
    pc->m_neff = 2.6391;
    pc->m_ng = 4.3416;
    
    pc->m_ring_length = 500.0*1.55e-6 + 1.55e-6/2.0;
    pc->m_loss_db_cm = 2.0;
    pc->m_coupling_through = 1 - 0.2;
    pc->m_neff = 1;
    pc->m_ng = 2;

    pc->init(); //instantiating all nets

    Probe pthrough("pthrough", true, true, false, true);
    pthrough.p_in(THROUGH);
    //
    Probe pcross("pcross", true, true, false, false);
    pcross.p_in(DROP);

    specsGlobalConfig.trace_filename = "traces/crow_tb";

    // Apply SPECS options specific to the testbench
    // specsGlobalConfig.simulation_mode = OpticalOutputPortMode::EVENT_DRIVEN;
    specsGlobalConfig.trace_all_optical_nets = 0;

    // Run SPECS pre-simulation code
    specsGlobalConfig.prepareSimulation();

    // Start simulation
    sc_start(50e-9, SC_SEC);

    std::cout << std::endl << std::endl;
    std::cout << ".vcd trace file: " << specsGlobalConfig.trace_filename << std::endl;

    sc_close_vcd_trace_file(specsGlobalConfig.default_trace_file);
}
