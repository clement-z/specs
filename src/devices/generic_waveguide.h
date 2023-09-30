#pragma once

#include "devices/generic_transmission_device.h"

using std::isfinite;

class GenericWaveguide : public GenericTransmissionDevice {
public:
    GenericWaveguide(sc_module_name name
                    , const double &length = 0
                    , const double &loss = 0
                    , const double &neff = 1
                    , const double &ng = 1
                    , const double &D = 0
                    , const double &lambda0 = 1.55e-6)
    : GenericTransmissionDevice(name, 2)
    {
        cerr << "GenericWaveguide is disabled (not working)." << endl;
        exit(1);

        setLength(length);
        setLoss(loss);
        setEffectiveIndex(neff);
        setGroupIndex(ng);
        setDispersion(D);
        setLambda0(lambda0);
    }

    void setLength(const double &length)
    {
        assert(isfinite(length) && length >= 0);
        m_length = length;
    }

    void setLoss(const double &loss)
    {
        cout << loss << endl;
        assert(isfinite(loss));
        m_loss = loss;
    }

    void setEffectiveIndex(const double &neff)
    {
        assert(isfinite(neff) && neff >= 0);
        m_neff = neff;
    }

    void setGroupIndex(const double &ng)
    {
        assert(isfinite(ng) && ng >= 0);
        m_ng = ng;
    }

    void setDispersion(const double &D)
    {
        assert(isfinite(D));
        m_D = D;
    }

    void setLambda0(const double &lambda0)
    {
        assert(isfinite(lambda0) && lambda0 > 0);
        m_lambda0 = lambda0;
    }


private:

    virtual void prepareTM()
    {
        TM.clear();
        TM.resize(nports);

        const double c = 299792458.0;
        double lambda0 = m_lambda0;
        double transmission = pow(10.0, -(m_loss * m_length)/20.0);
        double two_pi_L = 2 * M_PI * m_length;
        double dphi_0 = fmod(two_pi_L * m_neff, 2 * M_PI);

        // The following 2 lines are wrong...
        double dphi_1 = - two_pi_L * m_ng / lambda0 / lambda0; // Here it should be -2pi*L*ng/(lambda*lambda0) (instead of lambda0^2)
        double dphi_2 = two_pi_L * 2 * m_ng / lambda0 / lambda0 / lambda0;

        double group_delay_0 = m_length * m_ng / c;
        double group_delay_1 = m_length * m_D;

        TM.lambda0 = lambda0;
        TM.Mactive = {false, true, true, false};
        TM.Malpha = {
            {0},
            {transmission},
            {transmission},
            {0}};
        TM.Mphi = {
            {0},
            {dphi_0, dphi_1, dphi_2},
            {dphi_0, dphi_1, dphi_2},
            {0}};
        TM.Mtau = {
            {0},
            {group_delay_0, group_delay_1},
            {group_delay_0, group_delay_1},
            {0}};

        // cout << "----------- " << name() << " -----------" << endl;
        // cout << "- alpha:        " << TM.Malpha[1][0] << " (V/m)/(V/m)" << endl;
        // cout << "- transmission: " << pow(TM.Malpha[1][0],2.0) << " W/W" << endl;
        // cout << "- phi:          " << TM.Mphi[1][0] << " rad" << endl;
        // cout << "- dphi/dlambda: " << TM.Mphi[1][1] << " rad/m" << endl;
        // cout << "- group delay:  " << TM.Mtau[1][0] << " s" << endl;
    }

    double m_loss; // loss in dB/m
    double m_neff; // real part of neff
    double m_ng; // (real part of) ng
    double m_D; // dispersion
    double m_lambda0; // dispersion
    double m_length; // waveguide length
};