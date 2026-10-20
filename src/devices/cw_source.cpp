#include "specs.h"
#include "devices/cw_source.h"

using std::cout;
using std::endl;

void CWSource::runner()
{
    if (specsGlobalConfig.verbose_component_initialization)
    {
        cout << name() << ":" << endl;
        cout << "signal on: " << m_signal_on << endl;
        cout << "--> " <<
            port_channel_name(p_out) << endl;
        cout << endl;
    }

    while (true) {
        // Wait for enable signal
        if (! enable.read().to_bool())
        {
            // cout << name() << " waiting for enable" << endl;
            wait(enable.posedge_event());
        }
        // cout << name() << " was enabled" << endl;
        auto s = m_signal_on;
        s.getNewId();

        // Write value to output
        m_out_writer.delayedWrite(s, SC_ZERO_TIME);
        cout << name() << " emitted: " << s << endl;

        // Wait for reset
        if (!reset.read().to_bool())
        {
            // cout << name() << " waiting for reset" << endl;
            wait(reset.posedge_event());
            reset.write(sc_logic(0));
            wait(reset.negedge_event());
        }
        // cout << name() << " was reset" << endl;
    }
}
