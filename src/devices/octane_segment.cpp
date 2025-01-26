#include "devices/octane_segment.h"


#define __modname(SUFFIX, IDX) \
    ((""s + this->name() + "_" + SUFFIX + to_string(IDX)).c_str())

using std::size_t;
using std::pow;

void OctaneSegment::init_ports()
{
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    // Initializing input ports
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//

    p_in.clear();

    // cout << "Initializing input ports of " << name() << endl;
    // 0 is row, 1 is column  
    p_in.push_back(make_unique<port_in_type>("IN_R"));
    p_in.push_back(make_unique<port_in_type>("IN_C"));

    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    // Initializing output ports based on termination
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    
    p_out.clear();
    
    // cout << "Initializing output ports of " << name() << endl;
    // If there is only one output, the vector has only one element
    if (m_term_row && m_term_col)
    {
        ; // no optical output
    }
    else if(m_term_row)
    {
        // Last row, so only has row outputs
        p_out.push_back(make_unique<port_out_type>("OUT_R"));
    }
    else if(m_term_col)
    {
        // Last column, so only has only column outputs
        p_out.push_back(make_unique<port_out_type>("OUT_C"));
    }
    else
    {
        // Internal elements have both outputs
        p_out.push_back(make_unique<port_out_type>("OUT_R"));
        p_out.push_back(make_unique<port_out_type>("OUT_C"));
    }
    p_readout = make_unique<sc_out<double>>("OUT_PD");

    // cout << "Successful port initialization of  " << name() << endl;

}

void OctaneSegment::init()
{
    if(m_isCompact)
        init_compact();
    else
        init_full();

}

void OctaneSegment::init_full()
{
    // Physical parameters of the components
    double lambda = 1550e-9; // the wavelength used by the circuit
    double neff = 2.2;
    double ng = 2.2;
    double loss_per_cm = 0; // in waveguide
    double dc_loss = 0;
    double length_for_2pi = 100*lambda/(neff);
    double length_for_pi_2 = length_for_2pi/4;

    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    // Initializing circuit elements (waveguides, DCs, octanecell)
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//

    m_wg.clear();
    m_dc.clear();
    m_optical_connect.clear();

    // cout << "Initializing submodules of " << name() << endl;
    
    m_oct = make_unique<OctaneCell>(__modname("OCT_", 0), m_meltEnergy, m_nStates, m_isCompact);
    m_oct->init();

    if (m_term_row && m_term_col)
    {   
        // Last element
        for(size_t i = 0; i < 2; i++)
        {
            m_wg.push_back(make_unique<Waveguide>(__modname("WG_", i), length_for_2pi, loss_per_cm, neff, ng));
            m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("W_", i)));
        }
    }
    else if(m_term_row)
    {
        // Last row
        m_dc.push_back(make_unique<DirectionalCoupler>(__modname("DC_R_", 0), m_coupling_through_row, dc_loss));
        for(size_t i = 0; i < 4; i++)
        {
            m_wg.push_back(make_unique<Waveguide>(__modname("WG_", i), length_for_2pi, loss_per_cm, neff, ng));
            m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("W_", i)));
        }
        // Adjusting the last waveguide to give pi/2 shift
        // compensating for the lack of DC
        m_wg.at(3)->m_length_cm = length_for_2pi + length_for_pi_2;
        // We have one extra wire with respect to waveguides
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("W_", 4)));
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("TERM_", 0))); // terminator
    }
    else if(m_term_col)
    {
        // Last column
        m_dc.push_back(make_unique<DirectionalCoupler>(__modname("DC_C_", 0), m_coupling_through_col, dc_loss));
        for(size_t i = 0; i < 4; i++)
        {
            m_wg.push_back(make_unique<Waveguide>(__modname("WG_", i), length_for_2pi, loss_per_cm, neff, ng));
            m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("W_", i)));
        }
        // Adjusting the last waveguide to give pi/2 shift
        // compensating for the lack of DC
        m_wg.at(3)->m_length_cm = length_for_2pi + length_for_pi_2;
        // We have one extra wire with respect to waveguides
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("W_", 4)));
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("TERM_", 0))); // terminator
    }
    else
    {
        // Internal element
        m_dc.push_back(make_unique<DirectionalCoupler>(__modname("DC_R_", 0), m_coupling_through_row, dc_loss));
        m_dc.push_back(make_unique<DirectionalCoupler>(__modname("DC_C_", 0), m_coupling_through_col, dc_loss));
        for(size_t i = 0; i < 6; i++)
        {
            m_wg.push_back(make_unique<Waveguide>(__modname("WG_", i), length_for_2pi, loss_per_cm, neff, ng));
            m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("W_", i)));
        }
        // We have two extra wires with respect to waveguides
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("W_", 6)));
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("W_", 7)));
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("TERM_", 0))); // terminator
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("TERM_", 1))); // terminator
    }

    // cout << "Successful module initialization of " << name() << endl;

    // Modules are constructed in their vectors, time to connect !
    connect_submodules_full();

}

void OctaneSegment::connect_submodules_full()
{

    // cout << "Connecting submodules of " << name() << endl;

    if (m_term_row && m_term_col)
    {   
        // Last element
        m_wg.at(0)->p_in(*p_in.at(0));     
        m_wg.at(0)->p_out(*m_optical_connect.at(0));

        m_wg.at(1)->p_in(*p_in.at(1));     
        m_wg.at(1)->p_out(*m_optical_connect.at(1));  

        m_oct->p_in_r(*m_optical_connect.at(0));
        m_oct->p_in_c(*m_optical_connect.at(1));
        
    }
    else if(m_term_row)
    {
        // Last row
        m_wg.at(0)->p_in(*p_in.at(0)); // input at row
        m_wg.at(0)->p_out(*m_optical_connect.at(0));

        m_dc.at(0)->p_in1(*m_optical_connect.at(0));
        m_dc.at(0)->p_in2(*m_optical_connect.at(5)); // Terminated
        m_dc.at(0)->p_out1(*m_optical_connect.at(1)); // Through
        m_dc.at(0)->p_out2(*m_optical_connect.at(2)); // Cross

        m_wg.at(1)->p_in(*m_optical_connect.at(1));     
        m_wg.at(1)->p_out(*p_out.at(0)); // Out row

        m_wg.at(2)->p_in(*m_optical_connect.at(2));     
        m_wg.at(2)->p_out(*m_optical_connect.at(3));

        // Side without DC (must be conmpensated with pi/2)
        m_wg.at(3)->p_in(*p_in.at(1));
        m_wg.at(3)->p_out(*m_optical_connect.at(4));

        m_oct->p_in_r(*m_optical_connect.at(3));
        m_oct->p_in_c(*m_optical_connect.at(4));
    }
    else if(m_term_col)
    {
        // Last column
        m_wg.at(0)->p_in(*p_in.at(1)); // input at col
        m_wg.at(0)->p_out(*m_optical_connect.at(0));

        m_dc.at(0)->p_in1(*m_optical_connect.at(0));
        m_dc.at(0)->p_in2(*m_optical_connect.at(5)); // Terminated
        m_dc.at(0)->p_out1(*m_optical_connect.at(1)); // Through
        m_dc.at(0)->p_out2(*m_optical_connect.at(2)); // Cross

        m_wg.at(1)->p_in(*m_optical_connect.at(1));     
        m_wg.at(1)->p_out(*p_out.at(0)); // Out column

        m_wg.at(2)->p_in(*m_optical_connect.at(2));     
        m_wg.at(2)->p_out(*m_optical_connect.at(3));

        // Side without DC (must be conmpensated with pi/2)
        m_wg.at(3)->p_in(*p_in.at(0));     
        m_wg.at(3)->p_out(*m_optical_connect.at(4));

        m_oct->p_in_r(*m_optical_connect.at(4));
        m_oct->p_in_c(*m_optical_connect.at(3));
    }
    else
    {
        // Internal element
        m_wg.at(0)->p_in(*p_in.at(0)); // input at row
        m_wg.at(0)->p_out(*m_optical_connect.at(0));

        m_dc.at(0)->p_in1(*m_optical_connect.at(0));
        m_dc.at(0)->p_in2(*m_optical_connect.at(8)); // Terminated
        m_dc.at(0)->p_out1(*m_optical_connect.at(1)); // Through
        m_dc.at(0)->p_out2(*m_optical_connect.at(2)); // Cross

        m_wg.at(1)->p_in(*m_optical_connect.at(1));     
        m_wg.at(1)->p_out(*p_out.at(0)); // Out row

        m_wg.at(2)->p_in(*m_optical_connect.at(2));     
        m_wg.at(2)->p_out(*m_optical_connect.at(3));

        m_wg.at(3)->p_in(*p_in.at(1));     
        m_wg.at(3)->p_out(*m_optical_connect.at(4));

        m_dc.at(1)->p_in1(*m_optical_connect.at(4));
        m_dc.at(1)->p_in2(*m_optical_connect.at(9)); // Terminated
        m_dc.at(1)->p_out1(*m_optical_connect.at(5)); // Through
        m_dc.at(1)->p_out2(*m_optical_connect.at(6)); // Cross

        m_wg.at(4)->p_in(*m_optical_connect.at(5));     
        m_wg.at(4)->p_out(*p_out.at(1)); // Out col

        m_wg.at(5)->p_in(*m_optical_connect.at(6));     
        m_wg.at(5)->p_out(*m_optical_connect.at(7));    

        m_oct->p_in_r(*m_optical_connect.at(3));
        m_oct->p_in_c(*m_optical_connect.at(7));
    }
    m_oct->p_readout(*p_readout);

    // cout << "Successful connection of " << name() << endl;

}

void OctaneSegment::init_compact()
{
    // Physical parameters of the components
    double lambda = 1550e-9; // the wavelength used by the circuit
    double neff = 2.2;
    double ng = 2.2;
    double loss_per_cm = 0; // in waveguide
    double dc_loss = 0;
    double length_for_2pi = 100*lambda/(neff);
    double length_for_pi_2 = length_for_2pi/4;

    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    // Initializing circuit elements (waveguides, DCs, octanecell)
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//

    m_wg.clear();
    m_dc.clear();
    m_optical_connect.clear();

    // cout << "Initializing submodules of " << name() << endl;
    
    m_oct = make_unique<OctaneCell>(__modname("OCT_", 0), m_meltEnergy, m_nStates, m_isCompact);
    m_oct->init();

    if (m_term_row && m_term_col)
    {   
        ;
    }
    else if(m_term_row)
    {
        // Last row
        m_dc.push_back(make_unique<DirectionalCoupler>(__modname("DC_R_", 0), m_coupling_through_row, dc_loss));
        m_wg.push_back(make_unique<Waveguide>(__modname("WG_", 0), length_for_pi_2, loss_per_cm, neff, ng));
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("W_", 0)));
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("W_", 1)));
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("TERM_", 0))); // terminator
    }
    else if(m_term_col)
    {
        // Last column
        m_dc.push_back(make_unique<DirectionalCoupler>(__modname("DC_C_", 0), m_coupling_through_col, dc_loss));
        m_wg.push_back(make_unique<Waveguide>(__modname("WG_", 0), length_for_pi_2, loss_per_cm, neff, ng));
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("W_", 0)));
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("W_", 1)));
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("TERM_", 0))); // terminator
    }
    else
    {
        // Internal element
        m_dc.push_back(make_unique<DirectionalCoupler>(__modname("DC_R_", 0), m_coupling_through_row, dc_loss));
        m_dc.push_back(make_unique<DirectionalCoupler>(__modname("DC_C_", 0), m_coupling_through_col, dc_loss));
        // We have two extra wires with respect to waveguides
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("W_", 0)));
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("W_", 1)));
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("TERM_", 0))); // terminator
        m_optical_connect.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("TERM_", 1))); // terminator
    }

    // cout << "Successful module initialization of " << name() << endl;

    // Modules are constructed in their vectors, time to connect !
    connect_submodules_compact();

}

void OctaneSegment::connect_submodules_compact()
{

    // cout << "Connecting submodules of " << name() << endl;

    if (m_term_row && m_term_col)
    {   
        // Last element
        m_oct->p_in_r(*p_in.at(0));
        m_oct->p_in_c(*p_in.at(1));
        
    }
    else if(m_term_row)
    {
        // Last row
        m_dc.at(0)->p_in1(*p_in.at(0));
        m_dc.at(0)->p_in2(*m_optical_connect.at(2)); // Terminated
        m_dc.at(0)->p_out1(*p_out.at(0)); // Through
        m_dc.at(0)->p_out2(*m_optical_connect.at(0)); // Cross

        // Side without DC (must be conmpensated with pi/2)
        m_wg.at(0)->p_in(*p_in.at(1));
        m_wg.at(0)->p_out(*m_optical_connect.at(1));

        m_oct->p_in_r(*m_optical_connect.at(0));
        m_oct->p_in_c(*m_optical_connect.at(1));
    }
    else if(m_term_col)
    {
        // Last column
        m_dc.at(0)->p_in1(*p_in.at(1));
        m_dc.at(0)->p_in2(*m_optical_connect.at(2)); // Terminated
        m_dc.at(0)->p_out1(*p_out.at(0)); // Through
        m_dc.at(0)->p_out2(*m_optical_connect.at(0)); // Cross

        // Side without DC (must be conmpensated with pi/2)
        m_wg.at(0)->p_in(*p_in.at(0));     
        m_wg.at(0)->p_out(*m_optical_connect.at(1));

        m_oct->p_in_r(*m_optical_connect.at(1));
        m_oct->p_in_c(*m_optical_connect.at(0));
    }
    else
    {
        // Internal element
        m_dc.at(0)->p_in1(*p_in.at(0));
        m_dc.at(0)->p_in2(*m_optical_connect.at(2)); // Terminated
        m_dc.at(0)->p_out1(*p_out.at(0)); // Through
        m_dc.at(0)->p_out2(*m_optical_connect.at(0)); // Cross

        m_dc.at(1)->p_in1(*p_in.at(1));
        m_dc.at(1)->p_in2(*m_optical_connect.at(3)); // Terminated
        m_dc.at(1)->p_out1(*p_out.at(1)); // Through
        m_dc.at(1)->p_out2(*m_optical_connect.at(1)); // Cross

        m_oct->p_in_r(*m_optical_connect.at(0));
        m_oct->p_in_c(*m_optical_connect.at(1));
    }
    m_oct->p_readout(*p_readout);

    // cout << "Successful connection of " << name() << endl;

}