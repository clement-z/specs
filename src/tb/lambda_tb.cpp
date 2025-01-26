#include <ctime>
#include <iomanip>
#include "tb/lambda_tb.h"

#include "utils/general_utils.h"
#include "devices/generic_waveguide.h"

void lambda_tb::run_1()
{
    // IN1.write(OpticalSignal(sqrt(2.0), 1550e-9));
    // wait(0, SC_FS);
    // IN2.write(OpticalSignal(sqrt(2.0), 1660e-9));
    // while(true) {wait();}
    wait(1,SC_SEC);
    sc_stop();
}

void lambda_tb::monitor()
{
    unsigned int event_counter = 0;
    unsigned int success_counter = 0;
    const unsigned int test_number = 0;
    while(true)
    {
        wait();
        event_counter++;
        std::cout << sc_time_stamp() << ":" << std::endl
            << "\tOUT: " << OUT.read() << std::endl
            << "\tCOUNT: " << event_counter << std::endl;
    }

}

void lambda_tb_run()
{
    // Apply SPECS resolution before creating any device
    specsGlobalConfig.applyEngineResolution();

    spx::oa_signal_type I1, I2, YOUT, WGOUT;
    spx::oa_signal_type DCO1, DCO2, DCI1, DCI2;

    CWSource cw1("cw1");
    cw1.setWavelength(1550e-9);
    cw1.setPower(2);
    cw1.enable.write(SC_LOGIC_1);
    cw1.p_out(I1);

    CWSource cw2("cw2");
    cw2.setWavelength(1660e-9);
    cw2.setPower(2);
    cw2.enable.write(SC_LOGIC_1);
    cw2.p_out(I2);

    Merger merg1("merg1");
    merg1.p_in1(I1);
    merg1.p_in2(I2);
    merg1.p_out(YOUT);

    Waveguide wg1("wg1", 0.1, 0, 1, 1);
    wg1.m_attenuation_dB_cm = 0;
    wg1.p_in(YOUT);
    wg1.p_out(DCI1);

    DirectionalCoupler dc1("dc1");
    dc1.p_in1(DCI1);
    dc1.p_in2(DCI2);
    dc1.p_out1(DCO1);
    dc1.p_out2(DCO2);

    Waveguide wg2("wg2", 100*1001*1550e-9/(2), 0, 1, 1);
    wg2.p_in(DCO2);
    wg2.p_out(DCI2);

    lambda_tb tb("tb");
    tb.OUT(DCO1);
    // tb.IN1(I1);
    // tb.IN2(I2);

    // Open Trace file
    std::string trace_filename = "traces/";
    trace_filename += "lambda_tb";
    specsGlobalConfig.trace_filename = trace_filename;

    // Apply SPECS options specific to the testbench
    specsGlobalConfig.simulation_mode = OpticalOutputPortMode::EVENT_DRIVEN;
    specsGlobalConfig.trace_all_optical_nets = 0;

    MLambdaProbe probe("probe_lambda", {1550e-9, 1650e-9, 1660e-9});
    // Probe probe("test_probe",specsGlobalConfig.default_trace_file);
    probe.p_in(DCO1);

    // Run SPECS pre-simulation code
    specsGlobalConfig.prepareSimulation();

    // Start simulation
    sc_start();

    std::cout << std::endl << std::endl;
    std::cout << ".vcd trace file: " << specsGlobalConfig.trace_filename << std::endl;

    sc_close_vcd_trace_file(specsGlobalConfig.default_trace_file);
}
