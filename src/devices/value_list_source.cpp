#include "specs.h"
#include "devices/value_list_source.h"

using std::cout;
using std::cerr;
using std::endl;

void VLSource::runner()
{
    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << ":" << endl;
        cout << "value list (" << m_values_queue.size() << " values)";
        cout << endl;
        cout << "--> " <<
            port_channel_name(p_out) << endl;
        cout << endl;
    }

    // Wait for enable signal
    if (! enable.read().to_bool())
    {
        // cout << name() << " waiting for enable" << endl;
        wait(enable.posedge_event());
        if (specsGlobalConfig.verbose_component_initialization)
            cout << name() << " was enabled" << endl;
    }

    // B-066: one warning for all the values the loop below skips (time only advances in its waits)
    size_t n_past = 0;
    sc_time t_emit = sc_time_stamp();
    for (const auto &v : m_values_queue)
    {
        if (isPast(v.first, t_emit))
            ++n_past;
        else
            t_emit = sc_time(v.first, SC_SEC);
    }
    if (n_past)
        cerr << "Warning: " << name() << ": invalid time for signal emission (in the past) for " << n_past
             << " of " << m_values_queue.size() << " values, not emitted" << endl;

    // Emitting 0 at enable (will only go through if there is no value at t=0)
    if (m_values_queue.cbegin() == m_values_queue.cend()
        || sc_time(m_values_queue.front().first, SC_SEC).value() > 0)
    {
        if (specsGlobalConfig.verbose_component_initialization)
            cout << "@" << sc_time_stamp() << ", " << name() << " emitted: " << spx::oa_value_type(0) << endl;
        m_out_writer.delayedWrite(spx::oa_value_type(0), SC_ZERO_TIME);
    }

    auto it = m_values_queue.cbegin();
    while (it != m_values_queue.cend())
    {
        sc_time now = sc_time_stamp();
        sc_time next_emit;
        if (isPast(it->first, now))
        {
            ++it;
            continue;
        }
        sc_time delay = sc_time(it->first, SC_SEC) - now;

        // Wait until next output time
        wait(delay);

        auto s = it->second;

        // Write value to output
        m_out_writer.delayedWrite(s, SC_ZERO_TIME);
        if (specsGlobalConfig.verbose_component_initialization)
            cout << "@" << sc_time_stamp() << ", " << name() << " emitted: " << s << endl;

        ++it;
    }

    // cout << name() << " completed" << endl;
    while(true) { wait(); }
}
