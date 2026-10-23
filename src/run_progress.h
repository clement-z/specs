#pragma once

#include <systemc.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

// Progress line (F-15, specs --progress): at most once per period of wall-clock time, one line
// on stderr with the wall time, the simulated time (and the share of the .tran duration) and the
// delta cycles so far. It is a SystemC stage callback (SC_POST_UPDATE, after each delta cycle)
// that only reads the kernel state: it never advances simulated time and never stops or exits
// the simulation.
class RunProgress : public sc_stage_callback_if {
public:
    typedef std::chrono::steady_clock clock_type;

    // end_time_s: simulated duration of the run (infinity: unknown, no percentage)
    RunProgress(double period_s, double end_time_s, std::ostream &os = std::cerr);
    ~RunProgress();

    // Register / unregister the callback (start() also starts the wall-clock time)
    void start();
    void stop();

    void stage_callback(const sc_stage &stage) override;

    // The line printed now (without the newline)
    std::string line() const;

    uint64_t lines_printed() const
    {
        return m_lines_printed;
    }

private:
    double m_period_s;
    double m_end_time_s;
    std::ostream &m_os;
    bool m_registered = false;
    clock_type::time_point m_start;
    clock_type::time_point m_next;
    uint64_t m_lines_printed = 0;
};
