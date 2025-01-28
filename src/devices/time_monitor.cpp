
#include "devices/time_monitor.h"
#include "optical_output_port.h"
#include "specs.h"
#include "utils/sysc_utils.h"
#include <vector>
#include <set>

using namespace std;

string vec2str(const vector<bool> &v)
{
    string str;
    for (const auto &x : v)
        if (x)
            str += "1|";
        else
            str += "0|";

    return str;
}

void TimeMonitor::on_trigger() {
    auto first = system_clock::now();
    auto last = first;

    set<OpticalOutputPort *> oops = sc_get_all_module_by_type<OpticalOutputPort>();
    sc_time delay = sc_time(m_poll_period, SC_SEC);

    //wait(1, SC_PS);
    while (true) {
        // Wait for next polling event
        if (!sc_pending_activity())
        {
            // cout << "a" << endl;
            wait();
        }
        else if (sc_pending_activity_at_current_time() && delay.value() != 0)
        {
            // cout << "b" << endl;
            wait(delay);
        }
        else if (sc_pending_activity_at_current_time() && delay.value() == 0)
        {
            //cout << "c" << endl;
            wait(delay);
        }
        else if (sc_pending_activity_at_future_time() && delay.value() != 0)
        {
            // cout << "d" << endl;
            wait(delay);
        }
        else if (sc_pending_activity_at_future_time() && delay.value() == 0)
        {
            // cout << "e" << endl;
            wait(sc_time_to_pending_activity());
        }
        else
            cerr << "How did you get here ?" << endl;


        // Get current wall clock
        auto now = system_clock::now();
        std::chrono::duration<double> elapsed_seconds = now - last;

        // Report if it's time to do it
        if (elapsed_seconds.count() >= m_wallclock_period)
        {
            #if 0
            double error_sum = 0;
            double max_error = 0;
            double emitted_power_sum = 0;
            double max_emitted_power = 0;
            string max_error_name;
            complex<double> culprit_emitted;
            double culprit_emitted_pow = 0.0;
            complex<double> culprit_wanted;
            vector<bool> is_empty(oops.size(), false);
            int i = 0;
            int max_i = 0;
            for (const auto &oop : oops)
            {
                is_empty[i] = oop->isempty();
                error_sum += oop->current_err_fd();
                max_error = max(max_error, oop->current_err_fd());
                if (max_error == oop->current_err_fd())
                // if (strcmp(oop->name(), "crow.dc_0.out1_delayed_writer") == 0)
                {
                    max_error_name = oop->name();
                    // deprecated following lines on 05/04:
                    // culprit_emitted = oop->m_emitted_val_fd;
                    // culprit_emitted_pow = oop->m_emitted_val_fd.power();
                    culprit_wanted = oop->m_cur_val_fd;
                    max_i = i;
                }
                // deprecated following lines on 05/04:
                // emitted_power_sum += oop->m_emitted_val_fd.power();
                // max_emitted_power = max(max_emitted_power, oop->m_emitted_val_fd.power());
                i++;
            }
            #endif

            std::chrono::duration<double> elapsed_seconds_since_start = now - first;
            last = now;
            auto t = sc_time_stamp().to_seconds();
            cout << "---------------------------------------------------------------------------" << endl;
            cerr << "Time monitor incompatible with new multi-wavelength features" << endl;
            exit(1);
            cout << "Current simulation time (after "<< (int)(elapsed_seconds_since_start.count()) <<"s): " << endl
            << "\tIn NS: " << t*1e9 << endl
            << "\tIn PS: " << t*1e12 << endl
            << "\tDelta-cycles at current time: " << sc_delta_count_at_current_time() << endl
            //<< "\tCumulated error in ports: " << error_sum << endl
            //<< "\tIsempty: " << vec2str(is_empty) << endl
            //<< "\tCulprit: " << max_error_name << endl
            //<< "\tCulprit i: " << max_i << endl
            //<< setprecision(12)
            //<< "\tWanted:  " << norm(culprit_wanted) << "W " << arg(culprit_wanted) << "rad" << endl
            //<< "\tEmitted: " << norm(culprit_emitted) << "W " << arg(culprit_emitted) << "rad" << endl
            //<< "\tDiff:    " << abs(norm(culprit_emitted) - norm(culprit_wanted)) << "W " << abs(arg(culprit_emitted) - arg(culprit_wanted)) << "rad" << endl
            //<< "\tMax error in ports: " << max_error << endl
            //<< "\tCumulated power output of all ports: " << emitted_power_sum << endl
            //<< "\tMax port power output: " << max_emitted_power << endl
            << "\tSimulation speed (NS/s): " << t*1e9 / elapsed_seconds_since_start.count() << endl;

            // if (max_error < specsGlobalConfig.oop_configs[0]->m_abs_tol_power)
            if (false)
            {
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                // DOES NOT WORK IF CODE IS COMPILED WITH -O3 ???
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                specsGlobalConfig.drop_all_events = true;
                wait(SC_ZERO_TIME);
                while (true) {
                    volatile bool isempty = true;
                    for (const auto &oop : oops) {
                        isempty &= oop->isempty();
                        if (oop->isempty())
                        {
                            cout << oop->name() << " didnt empty its queue" << endl;
                        }
                    }
                    if (isempty)
                        break;
                    wait(SC_ZERO_TIME);
                }
                specsGlobalConfig.drop_all_events = false;
                wait(sc_time::from_value(1));
            }
        }
    }
}
