#include "specs.h"
#include "electrical_value_list_source.h"

using std::cout;
using std::cerr;
using std::endl;

void EVLSource::runner()
{
    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << ":" << endl;
        cout << "value list (" << m_values_queue.size() << " values)";
        if (false)
        {
            cout << ":" << endl;
            for (const auto& val : m_values_queue)
                cout << "\t" << "@" << sc_time(val.first, SC_SEC) << ": " << val.second << endl;
        }
        cout << endl;
        cout << "--> " <<
            (dynamic_cast<spx::ea_signal_type *>(p_out.get_interface()))->name() << endl;
        cout << endl;
    }

    // Wait for enable signal
    if (! enable.read().to_bool())
    {
        // cout << name() << " waiting for enable" << endl;
        wait(enable.posedge_event());
        cout << name() << " was enabled" << endl;
    }

    // Emitting 0 at enable (will only go through if there is no value at t=0)
    if (m_values_queue.cbegin() == m_values_queue.cend()
        || sc_time(m_values_queue.front().first, SC_SEC).value() > 0)
    {
        cout << "@" << sc_time_stamp() << ", " << name() << " emitted: " << spx::ea_value_type(0) << endl;
        p_out->write(spx::ea_value_type(0));
    }

    auto it = m_values_queue.cbegin();
    while (it != m_values_queue.cend())
    {
        sc_time now = sc_time_stamp();
        if (it->first < now.to_seconds())
        {
            ++it;
            continue;
        }
        sc_time delay = sc_time(it->first, SC_SEC) - now;

        // Wait until next output time
        wait(delay);

        auto Vout = it->second;

        // Write value to output
        p_out->write(Vout);
        cout << "@" << sc_time_stamp() << ", " << name() << " emitted: " << Vout << endl;

        ++it;
    }

    // cout << name() << " completed" << endl;
    while(true) { wait(); }
}
