#include "devices/generic_transmission_device.h"

#define __modname(SUFFIX, IDX) \
    ((""s + this->name() + SUFFIX + "_" + to_string(IDX)).c_str())

using std::size_t;
using std::pow;

TransmissionMatrix::paramset_type TransmissionMatrix::operator()(const size_t &i, const size_t &j
    , const wavelength_type &lambda, const size_t max_order) const
{
    // row/col to index
    const size_t k = i * N + j;

    if (Mactive[k] == false)
        return {0,0,0};

    if (max_order == 0)
        return {Malpha[k][0], Mphi[k][0], Mtau[k][0]};

    // Alias relevant taylor series
    auto &Malpha_ = Malpha[k];
    auto &Mphi_ = Mphi[k];
    auto &Mtau_ = Mtau[k];

    // Get initial parameters (values at lambda0)
    double alpha = Malpha_[0];
    double phi = Mphi_[0];
    double tau = Mtau_[0];

    // Calculate distance to lambda0
    const wavelength_type dlambda = lambda - lambda0;

    // Perform taylor expansion of alpha
    double dlambda_n_over_fact_n = 1.0;
    for (size_t order = 1; order < min(1 + max_order, Malpha_.size()); ++order)
    {
        double dalpha_over_dlambda_n = Malpha_[order];
        dlambda_n_over_fact_n *= dlambda / order;
        alpha += dalpha_over_dlambda_n * dlambda_n_over_fact_n;
    }

    // Perform taylor expansion of phi
    dlambda_n_over_fact_n = 1.0;
    for (size_t order = 1; order < min(1 + max_order, Mphi_.size()); ++order)
    {
        double dphi_over_dlambda_n = Mphi_[order];
        dlambda_n_over_fact_n *= dlambda / order;
        phi += dphi_over_dlambda_n * dlambda_n_over_fact_n;
    }

    // Perform taylor expansion of tau
    dlambda_n_over_fact_n = 1.0;
    for (size_t order = 1; order < min(1 + max_order, Mtau_.size()); ++order)
    {
        double dtau_over_dlambda_n = Mtau_[order];
        dlambda_n_over_fact_n *= dlambda / order;
        tau += dtau_over_dlambda_n * dlambda_n_over_fact_n;
    }

    //cout << lambda << ": " << alpha << ", " << fmod(phi, 2*M_PI) << ", " << tau << endl;
    return {alpha,phi,tau};
}

void GenericTransmissionDevice::pre_init()
{
    ports_in.clear();
    ports_out.clear();
    ports_out_writers.clear();

    for (size_t i = 0; i < nports; ++i)
    {
        ports_in.push_back(make_shared<port_in_type>(__modname("_IN", i)));
        ports_out.push_back(make_shared<port_out_type>(__modname("_OUT", i)));
        ports_out_writers.push_back(make_shared<OpticalOutputPort>(__modname("_OOP", i), (*ports_out[i])));
        ports_out_writers[i]->m_use_deltas = true;
    }
}

void GenericTransmissionDevice::init()
{
    prepareTM();
    assert(nports == TM.N);
    for (size_t i = 0; i < nports; ++i)
    {
        for (size_t j = 0; j < nports; ++j)
        {
            sc_spawn_options opts;
            if (TM.isActive(i, j))
            {
                opts.set_sensitivity(ports_in[i].get());
                sc_spawn( sc_bind(&GenericTransmissionDevice::input_on_i_output_on_j, this, i, j), 
                    (string(name()) + "process_" + to_string(i) + "_" + to_string(j)).c_str(), &opts);
            }
        }
    }
}

string GenericTransmissionDevice::describe() const
{
    return ""s;
}

void GenericTransmissionDevice::input_on_i(size_t i)
{
    auto last_signal = OpticalSignal(0);
    const auto &p_in = (*ports_in[i]);
    const bool active = TM.isInputActive(i);

    while(active)
    {
        // Wait for new input on i_in
        wait();

        // Read new input from i_in
        const auto &s = p_in->read();

        // cout << "received:" << endl << "\t" << s;

        // Compute delta
        const auto deltaE_in = s - last_signal;

        // Store new input signal for next change
        last_signal = s;

        // Loop on all device ports
        for (size_t j = 0; j < nports; ++j)
        {
            if (!TM.isActive(i,j))
                continue;
            // cout << "(i,j):" << i << ", " << j << endl;

            // Get transmission parameters at this wavelength
            auto Tij = TM(i, j, s.getWavelength());

            // Apply parameters
            auto deltaE_out = deltaE_in * polar(Tij.alpha, Tij.phi);

            // cout << Tij.alpha << ", " << Tij.phi << ", " << Tij.tau << endl;

            deltaE_out.getNewId();

            // Schedule writing the change in output port
            ports_out_writers[j]->delayedWrite(deltaE_out, sc_time(Tij.tau, SC_SEC));
        }
    }
    while (true) { wait(); }
}

void GenericTransmissionDevice::input_on_i_output_on_j(size_t i, size_t j)
{
    auto last_signal = OpticalSignal(0);
    const auto &p_in = (*ports_in[i]);
    //auto &p_out = (*ports_out[j]);
    auto &p_out_writer = (*ports_out_writers[j]);
    const bool active = TM.isActive(i, j);
    cout << i << ", " << j << " is active" << endl;

    // Block if non active
    while (!active) { wait(); }

    complex<double> Sij;
    double delay;
    
    // Wait for first signal to calculate Sij and delay
    volatile bool init_done = false;
    while (!init_done)
    {
        // Wait for first input on p_in
        wait();

        // Read new input from i_in
        const auto &s = p_in->read();

        // cout << "received:" << endl << "\t" << s << endl;

        if (isnan(s.getWavelength()))
            continue;

        // Find Sij at that wavelength
        const auto Tij = TM(i, j, s.getWavelength());
        Sij = polar(Tij.alpha, Tij.phi);
        delay = Tij.tau;
        
        auto deltaE = (s - last_signal) * Sij;
        last_signal = s;
        p_out_writer.delayedWrite(deltaE, sc_time(delay, SC_SEC));

        init_done = true;

        // cout << "init done:" << endl << "\t" << norm(Sij) << ", " << arg(Sij) << ", " << delay << endl;
    }

    while(true)
    {
        // Wait for new input on i_in
        wait();

        // Read new input from i_in
        const auto &s = p_in->read();

        if (s.getWavelength() != last_signal.getWavelength())
        {
            cerr << "Cannot handle different wavelength (" << __FUNCTION__ << ")" << endl;
            cerr << s << endl;
            cerr << last_signal << endl;
            sc_stop();
        }

        // cout << "received:" << endl << "\t" << s << endl;

        // Compute new delta
        auto deltaE = (s - last_signal) * Sij;

        // Store new input signal for next change
        last_signal = s;

        //deltaE_out.getNewId();

        // Schedule writing the change in output port
        p_out_writer.delayedWrite(deltaE, sc_time(delay, SC_SEC));
        //wait(SC_ZERO_TIME);
    }
}