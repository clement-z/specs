#pragma once

#include <systemc.h>
#include <fstream>
#include <algorithm>
#include <vector>
#include <utility>

#include "utils/strutils.h"
#include "optical_output_port.h"
#include "optical_signal.h"
#include "specs.h"
#include "devices/spx_module.h"

using std::vector;
using std::pair;

class EVLSource : public spx_module {
public:
    typedef pair<double, spx::ea_value_type> time_value_pair_type;

    // Ports
    spx::ea_port_out_type p_out;

    // Signal to emit
    vector<time_value_pair_type> m_values_queue;

    // Source emission control
    spx::ed_signal_type enable;

    // Processes
    void runner();

    EVLSource(sc_module_name name, const vector<time_value_pair_type> &values = {})
    : spx_module(name)
    , m_values_queue(values)
    {
        sortValues();
        enable = sc_logic(0);

        SC_HAS_PROCESS(EVLSource);
        SC_THREAD(runner);
    }

    void setValues(const vector<time_value_pair_type> &values)
    {
        m_values_queue = values;
        sortValues();
    }

    void setValues(const string &filename)
        {
        std::ifstream file(filename);

        if (!file.is_open()) {
            cerr << "Error: Cannot open the file." << endl;
            exit(1);
        }

        string line;

        getline(file, line);

        int lineNumber = 0;
        while (getline(file, line)) {
            ++lineNumber;
            istringstream s(line);
            string field;

            double t, V;

            // Skip if line is empty or starts with ;
            if (line.empty())
            {
                cout << "Skipping empty line." << endl;
                continue;
            }
            // Skip if line starts with ;
            if (line[0] == ';')
            {
                cout << "Skipping commented line: " << line << endl;
                continue;
            }

            // Read first field (time)
            getline(s, field, ',');
            strutils::trim(field);

            // Skip header line if it exists
            if(lineNumber == 1 && field == "time")
            {
                cout << "Skipping header line: " << line << endl;
                continue;
            }

            try {
                t = stod(field);
            } catch (std::invalid_argument const& ex) {
                cerr << "Invalid value for time: \"" << field << "\"" << endl;
                exit(1);
            }

            // Read second field (V)
            getline(s, field, ',');
            try {
                V = stod(field);
            } catch (std::invalid_argument const& ex) {
                cerr << "Invalid value for voltage: \"" << field << "\"" << endl;
                exit(1);
            }

            cout << t << ": " << V << "V" << endl;
            m_values_queue.emplace_back(t, V);
        }

        file.close();
        sortValues();
    }

    void sortValues()
    {
        auto cmp = [](const time_value_pair_type& p1, const time_value_pair_type& p2) {
            return p1.first < p2.first;
        };
        // Stable sort will keep initial ordering for values with identical time
        std::stable_sort(m_values_queue.begin(), m_values_queue.end(), cmp);
    }
};
