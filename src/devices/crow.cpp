#include <crow.h>

using namespace std;

void CROW::connect_submodules()
{
    // Physical parameters of the components
    // double neff = 2.6391; //2.2;
    // double ng = 4.3416; //neff;
    // double loss_db_cm = 2; //1;
    // double coupling_through = (1 - pow(0.83645, 2.0));//0.85;
    // double coupling_through = 1 - pow(0.83645, 2.0);//0.85;
    // double coupling_through = 1 - pow(0.83645, 2.0);//0.85;
    // coupling_through = 1 - 0.83645;//0.85;
    //double pi_length = 1.55e-6/2/neff;
    //double internal_length = 200*pi_length;
    //double internal_length = 1001*pi_length;
    // double internal_length = 2.0*((2.0 * M_PI * 3.2544e-6) + 10e-6); // 2pi*R+10um (R = 3.2544um)
    // double internal_length = 2.0 * M_PI * (3.2544e-6 + 10e-6/(2.0*M_PI)); // 2pi*R+10um (R = 3.2544um)
    //double var_factor = 1.01*pi_length;
    // double var_factor = 0;

    // -- Parameterizing components -- //
    for (size_t i = 0; i < N; i++)
    {
        // here is where variation is disabled (0 and uncomment)
        // double _wg_length_variation = var_factor*i;
        double _wg_length = m_ring_length / 2.0;
        // double _wg_length = M_PI * 3.2544e-6 + 10e-6;
        //double _wg_length = 200*100.0*(1550e-9 + 0.1e-9 * i)/(2.0*neff);
        
        //cout << _wg_length_variation << endl;
        cout << wg_top.at(i)->name() << endl;
        wg_top.at(i)->m_length_cm = _wg_length * 100.0;
        wg_top.at(i)->m_neff = m_neff;
        wg_top.at(i)->m_ng = m_ng;
        wg_top.at(i)->m_attenuation_dB_cm = m_loss_db_cm;

        cout << wg_bot.at(i)->name() << endl;
        wg_bot.at(i)->m_length_cm = _wg_length * 100.0;
        wg_bot.at(i)->m_neff = m_neff;
        wg_bot.at(i)->m_ng = m_ng;
        wg_bot.at(i)->m_attenuation_dB_cm = m_loss_db_cm;

        cout << dc.at(i)->name() << endl;
        dc.at(i)->m_dc_through_coupling_power = m_coupling_through;
    }
    dc.at(N)->m_dc_through_coupling_power = m_coupling_through;

    // -- Connecting components -- //
    // Before loop, outer connections on left
    dc.at(0)->p_in1(p_in);
    dc.at(0)->p_out1(p_out_t);
    //p_through.p_in(OUT_THROUGH);

    // Loop, inner connections
    for (size_t i = 0; i < N; i++)
    {
        // Need to take care if odd or even, 
        // as inputs come from different directions
        if (i%2 == 0)
        {
            // If i even, DC(i) input at bottom
            dc.at(i)->p_in2(*S_BL.at(i));
            dc.at(i)->p_out2(*S_TL.at(i));
            // If i even, DC(i+1) input at the top
            dc.at(i+1)->p_in1(*S_TR.at(i));
            dc.at(i+1)->p_out1(*S_BR.at(i));
            // If even, top waveguide(i) is L->R
            wg_top.at(i)->p_in(*S_TL.at(i));
            wg_top.at(i)->p_out(*S_TR.at(i));
            // If even, bottom waveguide(i) is R->L
            wg_bot.at(i)->p_in(*S_BR.at(i));
            wg_bot.at(i)->p_out(*S_BL.at(i));
        }
        else{
            // If i odd, DC(i) input at top
            dc.at(i)->p_in2(*S_TL.at(i));
            dc.at(i)->p_out2(*S_BL.at(i));
            // If i odd, DC(i+1) input at the bottom
            dc.at(i+1)->p_in1(*S_BR.at(i));
            dc.at(i+1)->p_out1(*S_TR.at(i));
            // If odd, top waveguide(i) is R->L
            wg_top.at(i)->p_in(*S_TR.at(i));
            wg_top.at(i)->p_out(*S_TL.at(i));
            // If odd, bottom waveguide(i) is L->R
            wg_bot.at(i)->p_in(*S_BL.at(i));
            wg_bot.at(i)->p_out(*S_BR.at(i));
        }
    }    
    // After loop, outer connections on right
    //pd1.p_readout(PD_OUT);
    dc.at(N)->p_in2(p_add);
    dc.at(N)->p_out2(p_out_d);

    dc.at(0)->m_out1_writer.m_converger = false;
}