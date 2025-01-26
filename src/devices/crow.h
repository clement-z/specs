#pragma once

#include <systemc.h>
#include "devices/waveguide.h"
#include "devices/detector.h"
#include "optical_output_port.h"
#include "optical_signal.h"
#include "devices/pcm_device.h"
#include "devices/directional_coupler.h"
#include "devices/merger.h"

#include <cassert>

class CROW : public sc_module {
public:
    // Ports
    /** The optical input ports. */
    sc_port<sc_signal_in_if<OpticalSignal>> p_in;
    sc_port<sc_signal_in_if<OpticalSignal>> p_add;
    /** The electrical output port. */
    sc_port<sc_signal_out_if<OpticalSignal>> p_out_t;
    sc_port<sc_signal_out_if<OpticalSignal>> p_out_d;

    // Member variables
    size_t N;
    double m_ring_length;
    double m_neff = 2.2;
    double m_ng = 4.3;
    double m_loss_db_cm = 2;
    double m_coupling_through = 0.85;

    // Wires
    vector<shared_ptr<sc_signal<OpticalSignal>>> S_TL;
    vector<shared_ptr<sc_signal<OpticalSignal>>> S_BL;
    vector<shared_ptr<sc_signal<OpticalSignal>>> S_TR;
    vector<shared_ptr<sc_signal<OpticalSignal>>> S_BR;
    //sc_signal<OpticalSignal> terminator;

    // Member submodules
    vector<shared_ptr<Waveguide>> wg_top;
    vector<shared_ptr<Waveguide>> wg_bot;
    vector<shared_ptr<DirectionalCoupler>> dc;

    
    virtual void init()
    {
        assert(N > 0);
        S_TL.clear();
        S_BL.clear();
        S_TR.clear();
        S_BR.clear();
        wg_bot.clear();
        wg_top.clear();
        dc.clear();
        stringstream ss;
        string _i;
        for (size_t i = 0; i < N; ++i)
        {
            ss.str(std::string());
            ss << i;
            _i = ss.str();
            cout << _i << endl;
            S_TL.push_back(make_shared<sc_signal<OpticalSignal>>((string("S_TL") + "_" + _i).c_str()));
            S_BL.push_back(make_shared<sc_signal<OpticalSignal>>((string("S_BL") + "_" + _i).c_str()));
            S_TR.push_back(make_shared<sc_signal<OpticalSignal>>((string("S_TR") + "_" + _i).c_str()));
            S_BR.push_back(make_shared<sc_signal<OpticalSignal>>((string("S_BR") + "_" + _i).c_str()));
            wg_bot.push_back(make_shared<Waveguide>((string("wg_bot") + "_" + _i).c_str()));
            wg_top.push_back(make_shared<Waveguide>((string("wg_top") + "_" + _i).c_str()));
            dc.push_back(make_shared<DirectionalCoupler>((string("dc") + "_" + _i).c_str()));
        }
        ss.str(std::string());
        ss << N;
        _i = ss.str();
        cout << _i << endl;
        dc.push_back(make_shared<DirectionalCoupler>((string("dc") + "_" + _i).c_str()));
        connect_submodules();
    }

    void connect_submodules();

    /** Constructor for Waveguide
     *
     * @param name name of the module
     * */
    CROW(sc_module_name name, const size_t &nrings = 3, const double &ring_length = 0.0)
        : sc_module(name)
        , N(nrings)
        , m_ring_length(ring_length)
    {
    }

    void setRingLength(const double &ring_length)
    {
        assert(ring_length >= 0);
        m_ring_length = ring_length;
    }
};
