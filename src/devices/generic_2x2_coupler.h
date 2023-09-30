#pragma once

#include "devices/generic_transmission_device.h"

class Generic2x2Coupler : public GenericTransmissionDevice {
public:
    Generic2x2Coupler(sc_module_name name
                    , const double &k_power = 0.0
                    , const double &insertion_loss = 0.0)
    : GenericTransmissionDevice(name, 4)
    {
        setCouplingFactor(k_power);
        setInsertionLoss(insertion_loss);
    }

    void setCouplingFactor(const double &k_power)
    {
        assert(isfinite(k_power) && k_power >= 0 && k_power <= 1);
        m_k_power = k_power;
    }

    void setInsertionLoss(const double &insertion_loss)
    {
        assert(isfinite(insertion_loss) && insertion_loss >= 0);
        m_insertion_loss = insertion_loss;
    }

private:

    virtual void prepareTM()
    {
        TM.clear();
        TM.resize(nports);

        double lambda0 = 1550e-9;
        double transmission_cross = sqrt(m_k_power) * pow(10.0, -m_insertion_loss/20.0);
        transmission_cross = max(0.0, min(1.0, transmission_cross));
        double transmission_through = sqrt(1 - m_k_power) * pow(10.0, -m_insertion_loss/20.0);
        transmission_through = max(0.0, min(1.0, transmission_through));
        double phi_through = 0.0;
        double phi_cross = M_PI / 2.0;
        double tau = 0.0;
        size_t k;

        // cout << transmission_through << endl;
        // cout << transmission_cross << endl;
        // cout << m_k_power << endl << endl << endl;

        // (13, 14, 23, 24 are the only active ones)

        // S13
        k = 0*nports + 2;
        TM.Mactive[k] = true;
        TM.Malpha[k] = {transmission_through};
        TM.Mphi[k] = {phi_through};
        TM.Mtau[k] = {tau};
        
        // S14
        k = 0*nports + 3;
        TM.Mactive[k] = true;
        TM.Malpha[k] = {transmission_cross};
        TM.Mphi[k] = {phi_cross};
        TM.Mtau[k] = {tau};
        
        // S23
        k = 1*nports + 2;
        TM.Mactive[k] = true;
        TM.Malpha[k] = {transmission_cross};
        TM.Mphi[k] = {phi_cross};
        TM.Mtau[k] = {tau};
        
        // S24
        k = 1*nports + 3;
        TM.Mactive[k] = true;
        TM.Malpha[k] = {transmission_through};
        TM.Mphi[k] = {phi_through};
        TM.Mtau[k] = {tau};

        // cout << TM.Mphi.size() << endl;
        // cout << TM.Mphi[0].size() << endl;
        // cout << TM.Mphi[1].size() << endl;
    }

    double m_insertion_loss;
    double m_k_power; // power coupling factor (1-k = transmission factor)
};