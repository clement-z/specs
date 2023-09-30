#include <octane_matrix.h>


#define __modname(SUFFIX, IDX) \
    ((""s + this->name() + "_" + SUFFIX + to_string(IDX)).c_str())

#define __outname(SUFFIX, IDX, IDY) \
    ((""s + this->name() + "_" + SUFFIX + to_string(IDX) + "_" + to_string(IDY)).c_str())

using std::size_t;
using std::pow;

void OctaneMatrix::init_ports()
{
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    // Initializing input ports
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    
    p_in_r.clear();
    p_in_c.clear();

    // cout << "Initializing input ports of " << name() << endl;

    for(size_t i = 0; i < m_rows; i++)
        p_in_r.push_back(make_unique<port_in_type>(__modname("IN_R_", i)));
    
    for(size_t i = 0; i < m_columns; i++)
        p_in_c.push_back(make_unique<port_in_type>(__modname("IN_C_", i)));

    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    // Initializing output port matrix
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    
    p_readout.clear(); // clearing before start

    // cout << "Initializing output ports of " << name() << endl;

    vector<unique_ptr<sc_out<double>>> temp_vector;

    for(size_t i = 0; i < m_rows; i++)
    {
        temp_vector.clear(); // moving to new row
        for(size_t j = 0; j < m_columns; j++)
        {
            temp_vector.push_back(make_unique<sc_out<double>>(__outname("PD_", i, j)));
        } 
        p_readout.push_back(std::move(temp_vector)); // adding new row
    }

    // cout << "Successful port initialization of  " << name() << endl;

}

void OctaneMatrix::init()
{

    double crossing_loss = 0;

    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//
    // Initializing circuit elements (waveguides, DCs, octanecell)
    // --------------------------------------------------------------------------//
    // --------------------------------------------------------------------------//

    m_optical_connect_horizontal.clear();
    m_optical_connect_vertical.clear();
    m_segment.clear();
    m_cross.clear();

    // cout << "Initializing submodules of " << name() << endl;

    // Optical wires
    unsigned long num_of_nets_horizontal = 3*m_rows*(m_columns-1);
    unsigned long num_of_nets_vertical = (m_rows-1)*m_columns;

    for(unsigned long i = 0; i < num_of_nets_horizontal; i++)
        m_optical_connect_horizontal.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("wh_", i)));
    for(unsigned long i = 0; i < num_of_nets_vertical; i++)
        m_optical_connect_vertical.push_back(make_unique<sc_signal<OpticalSignal>>(__modname("wv_", i)));

    // Crossings
    for(unsigned long i = 0; i < m_rows; i++)
    {   // Last column doesn't have crossings, thus the -1 in the loop
        for(unsigned long j = 0; j < (m_columns - 1); j++)
        {
            m_cross.push_back(make_unique<Crossing>(__outname("X_", i, j), crossing_loss));
        } 
    }

    // Octane segments
    bool term_row = false;
    bool term_col = false;
    double coupling_through_row = 0;
    double coupling_through_col = 0;

    for(unsigned long i = 0; i < m_rows; i++)
    {
        for(unsigned long j = 0; j < m_columns; j++)
        {
            if(isLastRow(i) && isLastCol(j))
            {
                // Last element
                term_col = true;
                term_row = true;
                coupling_through_row = 0;
                coupling_through_col = 0;
            }
            else if(isLastRow(i))
            {
                // Term row
                term_col = false;
                term_row = true;
                coupling_through_row = (1.0 - 1.0/(m_columns - j));
                coupling_through_col = 0;
            }
            else if(isLastCol(j))
            {
                // Term col
                term_col = true;
                term_row = false;
                coupling_through_row = 0;
                coupling_through_col = (1.0 - 1.0/(m_rows - i));
            }
            else
            {
                // Inner element
                term_col = false;
                term_row = false;
                coupling_through_row = (1.0 - 1.0/(m_columns - j));
                coupling_through_col = (1.0 - 1.0/(m_rows - i));
            }
            auto temp_segment = make_unique<OctaneSegment>
                    (
                    __outname("SEG_", i, j),
                    term_row, term_col,
                    coupling_through_row, coupling_through_col,
                    m_meltEnergy, m_nStates, m_isCompact
                    );
            temp_segment->init(); // mandatory initialization
            m_segment.push_back(std::move(temp_segment));
        }   
    }

    // cout << "Successful module initialization of " << name() << endl;

    // Modules are constructed in their vectors, time to connect !
    connect_submodules();

}

void OctaneMatrix::connect_submodules()
{

    // cout << "Connecting submodules of " << name() << endl;

    // If there is only one element in the matrix
    // the pins are connected directly
    if((m_rows == 1) && (m_columns == 1))
    {
        // cout << name() << "contains only one element. " << endl;

        m_segment.at(0)->p_in[0]->bind(*p_in_r.at(0));
        m_segment.at(0)->p_in[1]->bind(*p_in_c.at(0)); 
        m_segment.at(0)->p_readout->bind(*p_readout[0][0]);

        // cout << "Successful connection of " << name() << endl;
        return;
    }

    // Column vector
    if(m_columns == 1)
    {
        // cout << name() << "contains only one column. " << endl;

        for(unsigned long i = 0; i < m_rows; i++)
        {
            // in segment, in row is p_in[0]
            m_segment.at(i)->p_in[0]->bind(*p_in_r.at(i)); 

            if(isFirstRow(i)) // should connect to the port
                m_segment.at(i)->p_in[1]->bind(*p_in_c.at(0)); 
            else // connects to intermediate net which is output of the previous block
                m_segment.at(i)->p_in[1]->bind(*m_optical_connect_vertical.at(i-1)); 

            if(!isLastRow(i)) // there's only one output in vector segment
                m_segment.at(i)->p_out[0]->bind(*m_optical_connect_vertical.at(i)); 
            
            m_segment.at(i)->p_readout->bind(*p_readout[i][0]);

        }
        // cout << "Successful connection of " << name() << endl;
        return;
    }

    // Row vector
    if(m_rows == 1)
    {
        // cout << name() << "contains only one row. " << endl;

        for(unsigned long j = 0; j < m_columns; j++)
        {
            if(isFirstCol(j))
            {
                // in segment, in row is p_in[0]
                m_segment.at(j)->p_in[0]->bind(*p_in_r.at(0)); 
                m_segment.at(j)->p_in[1]->bind(*m_optical_connect_horizontal.at(3*j+1)); 
                m_segment.at(j)->p_out[0]->bind(*m_optical_connect_horizontal.at(3*j));

                // in1, out1 are horizontal connections
                m_cross.at(j)->p_in1.bind(*m_optical_connect_horizontal.at(3*j));
                m_cross.at(j)->p_in2.bind(*p_in_c.at(j));
                m_cross.at(j)->p_out1.bind(*m_optical_connect_horizontal.at(3*j+2));
                m_cross.at(j)->p_out2.bind(*m_optical_connect_horizontal.at(3*j+1));
            }
            else if(!isLastCol(j))
            { // middle element
                m_segment.at(j)->p_in[0]->bind(*m_optical_connect_horizontal.at(3*j-1)); 
                m_segment.at(j)->p_in[1]->bind(*m_optical_connect_horizontal.at(3*j+1)); 
                m_segment.at(j)->p_out[0]->bind(*m_optical_connect_horizontal.at(3*j));

                // in1, out1 are horizontal connections
                m_cross.at(j)->p_in1.bind(*m_optical_connect_horizontal.at(3*j));
                m_cross.at(j)->p_in2.bind(*p_in_c.at(j));
                m_cross.at(j)->p_out1.bind(*m_optical_connect_horizontal.at(3*j+2));
                m_cross.at(j)->p_out2.bind(*m_optical_connect_horizontal.at(3*j+1));
            }
            else
            { // last column here
                m_segment.at(j)->p_in[0]->bind(*m_optical_connect_horizontal.at(3*j-1)); 
                m_segment.at(j)->p_in[1]->bind(*p_in_c.at(j));            
            }
            m_segment.at(j)->p_readout->bind(*p_readout[0][j]);
        }
        // cout << "Successful connection of " << name() << endl;
        return;
    }

    // Getting here means it's an actual matrix with more than one element
    unsigned long cur_pos;
    unsigned long cur_cross;
    unsigned long seg_out_h;
    unsigned long seg_out_v;
    for(unsigned long i = 0; i < m_rows; i++)
    {
        for(unsigned long j = 0; j < m_columns; j++)
        {
            cur_pos = m_columns*i + j;
            cur_cross = (m_columns-1)*i + j;
            seg_out_h = 3*(m_columns-1)*i+3*j;
            seg_out_v = m_columns*i+j;
            // cout << "cur_pos: " << cur_pos << endl;
            // cout << "cur_cross: " << cur_cross << endl;
            // cout << "out_h: " << seg_out_h << endl;
            // cout << "out_v: " << seg_out_v << endl;

            if(isFirstRow(i) && isFirstCol(j))
            {
                // cout << "first element" << endl;
                m_segment.at(cur_pos)->p_in[0]->bind(*p_in_r.at(i));
                m_segment.at(cur_pos)->p_in[1]->bind(*m_optical_connect_horizontal.at(seg_out_h+1)); 
                m_segment.at(cur_pos)->p_out[0]->bind(*m_optical_connect_horizontal.at(seg_out_h));
                m_segment.at(cur_pos)->p_out[1]->bind(*m_optical_connect_vertical.at(seg_out_v));

                m_cross.at(cur_cross)->p_in1.bind(*m_optical_connect_horizontal.at(seg_out_h));
                m_cross.at(cur_cross)->p_in2.bind(*p_in_c.at(j));
                m_cross.at(cur_cross)->p_out1.bind(*m_optical_connect_horizontal.at(seg_out_h+2));
                m_cross.at(cur_cross)->p_out2.bind(*m_optical_connect_horizontal.at(seg_out_h+1));
            }
            else if(isFirstRow(i) && !isLastCol(j))
            {
                // cout << "first row not last col" << endl;
                m_segment.at(cur_pos)->p_in[0]->bind(*m_optical_connect_horizontal.at(seg_out_h-1));
                m_segment.at(cur_pos)->p_in[1]->bind(*m_optical_connect_horizontal.at(seg_out_h+1)); 
                m_segment.at(cur_pos)->p_out[0]->bind(*m_optical_connect_horizontal.at(seg_out_h));
                m_segment.at(cur_pos)->p_out[1]->bind(*m_optical_connect_vertical.at(seg_out_v));

                m_cross.at(cur_cross)->p_in1.bind(*m_optical_connect_horizontal.at(seg_out_h));
                m_cross.at(cur_cross)->p_in2.bind(*p_in_c.at(j));
                m_cross.at(cur_cross)->p_out1.bind(*m_optical_connect_horizontal.at(seg_out_h+2));
                m_cross.at(cur_cross)->p_out2.bind(*m_optical_connect_horizontal.at(seg_out_h+1));                
            }
            else if(isFirstRow(i) && isLastCol(j))
            {
                // cout << "first row last col" << endl;
                m_segment.at(cur_pos)->p_in[0]->bind(*m_optical_connect_horizontal.at(seg_out_h-1));
                m_segment.at(cur_pos)->p_in[1]->bind(*p_in_c.at(j)); 
                // no row output
                m_segment.at(cur_pos)->p_out[0]->bind(*m_optical_connect_vertical.at(seg_out_v));
            }
            else if(!isFirstRow(i) && !isLastRow(i) && isFirstCol(j))
            {
                // cout << "middle row first col" << endl;
                m_segment.at(cur_pos)->p_in[0]->bind(*p_in_r.at(i));
                m_segment.at(cur_pos)->p_in[1]->bind(*m_optical_connect_horizontal.at(seg_out_h+1)); 
                m_segment.at(cur_pos)->p_out[0]->bind(*m_optical_connect_horizontal.at(seg_out_h));
                m_segment.at(cur_pos)->p_out[1]->bind(*m_optical_connect_vertical.at(seg_out_v));

                m_cross.at(cur_cross)->p_in1.bind(*m_optical_connect_horizontal.at(seg_out_h));
                m_cross.at(cur_cross)->p_in2.bind(*m_optical_connect_vertical.at(seg_out_v-m_columns));
                m_cross.at(cur_cross)->p_out1.bind(*m_optical_connect_horizontal.at(seg_out_h+2));
                m_cross.at(cur_cross)->p_out2.bind(*m_optical_connect_horizontal.at(seg_out_h+1));
            }
            else if(!isFirstRow(i) && !isLastRow(i) && !isLastCol(j))
            {
                // cout << "middle row middle col" << endl;
                m_segment.at(cur_pos)->p_in[0]->bind(*m_optical_connect_horizontal.at(seg_out_h-1));
                m_segment.at(cur_pos)->p_in[1]->bind(*m_optical_connect_horizontal.at(seg_out_h+1)); 
                m_segment.at(cur_pos)->p_out[0]->bind(*m_optical_connect_horizontal.at(seg_out_h));
                m_segment.at(cur_pos)->p_out[1]->bind(*m_optical_connect_vertical.at(seg_out_v));

                m_cross.at(cur_cross)->p_in1.bind(*m_optical_connect_horizontal.at(seg_out_h));
                m_cross.at(cur_cross)->p_in2.bind(*m_optical_connect_vertical.at(seg_out_v-m_columns));
                m_cross.at(cur_cross)->p_out1.bind(*m_optical_connect_horizontal.at(seg_out_h+2));
                m_cross.at(cur_cross)->p_out2.bind(*m_optical_connect_horizontal.at(seg_out_h+1));
            }
            else if(!isFirstRow(i) && !isLastRow(i) && isLastCol(j))
            {
                // cout << "middle row last col" << endl;
                m_segment.at(cur_pos)->p_in[0]->bind(*m_optical_connect_horizontal.at(seg_out_h-1));
                m_segment.at(cur_pos)->p_in[1]->bind(*m_optical_connect_vertical.at(seg_out_v-m_columns)); 
                // no row output
                m_segment.at(cur_pos)->p_out[0]->bind(*m_optical_connect_vertical.at(seg_out_v));
            }
            else if(isLastRow(i) && isFirstCol(j))
            {
                // cout << "last row first col" << endl;
                m_segment.at(cur_pos)->p_in[0]->bind(*p_in_r.at(i));
                m_segment.at(cur_pos)->p_in[1]->bind(*m_optical_connect_horizontal.at(seg_out_h+1)); 
                m_segment.at(cur_pos)->p_out[0]->bind(*m_optical_connect_horizontal.at(seg_out_h));

                m_cross.at(cur_cross)->p_in1.bind(*m_optical_connect_horizontal.at(seg_out_h));
                m_cross.at(cur_cross)->p_in2.bind(*m_optical_connect_vertical.at(seg_out_v-m_columns));
                m_cross.at(cur_cross)->p_out1.bind(*m_optical_connect_horizontal.at(seg_out_h+2));
                m_cross.at(cur_cross)->p_out2.bind(*m_optical_connect_horizontal.at(seg_out_h+1));
            }
            else if(isLastRow(i) && !isLastCol(j))
            {
                // cout << "last row middle col" << endl;
                m_segment.at(cur_pos)->p_in[0]->bind(*m_optical_connect_horizontal.at(seg_out_h-1));
                m_segment.at(cur_pos)->p_in[1]->bind(*m_optical_connect_horizontal.at(seg_out_h+1)); 
                m_segment.at(cur_pos)->p_out[0]->bind(*m_optical_connect_horizontal.at(seg_out_h));

                m_cross.at(cur_cross)->p_in1.bind(*m_optical_connect_horizontal.at(seg_out_h));
                m_cross.at(cur_cross)->p_in2.bind(*m_optical_connect_vertical.at(seg_out_v-m_columns));
                m_cross.at(cur_cross)->p_out1.bind(*m_optical_connect_horizontal.at(seg_out_h+2));
                m_cross.at(cur_cross)->p_out2.bind(*m_optical_connect_horizontal.at(seg_out_h+1));
            }
            else if(isLastRow(i) && isLastCol(j))
            {
                // cout << "last row last col" << endl;
                m_segment.at(cur_pos)->p_in[0]->bind(*m_optical_connect_horizontal.at(seg_out_h-1));
                m_segment.at(cur_pos)->p_in[1]->bind(*m_optical_connect_vertical.at(seg_out_v-m_columns)); 
            }
            else
                cout<< endl << endl << "This condition should not appear" << endl << endl;
            
            m_segment.at(cur_pos)->p_readout->bind(*p_readout[i][j]);

        }   
    }
}