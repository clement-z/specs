#include "run_progress.h"

#include <cmath>
#include <iomanip>
#include <sstream>

RunProgress::RunProgress(double period_s, double end_time_s, std::ostream &os)
    : m_period_s(period_s)
    , m_end_time_s(end_time_s)
    , m_os(os)
{
}

RunProgress::~RunProgress()
{
    stop();
}

void RunProgress::start()
{
    m_start = clock_type::now();
    m_next = m_start + std::chrono::duration_cast<clock_type::duration>(std::chrono::duration<double>(m_period_s));
    if (!m_registered)
        sc_register_stage_callback(*this, SC_POST_UPDATE);
    m_registered = true;
}

void RunProgress::stop()
{
    if (m_registered)
        sc_unregister_stage_callback(*this, SC_POST_UPDATE);
    m_registered = false;
}

void RunProgress::stage_callback(const sc_stage &stage)
{
    (void)stage;
    const clock_type::time_point now = clock_type::now();
    if (now < m_next)
        return;
    m_os << line() << std::endl;
    ++m_lines_printed;
    // the next line one period later, at the earliest
    m_next = now + std::chrono::duration_cast<clock_type::duration>(std::chrono::duration<double>(m_period_s));
}

std::string RunProgress::line() const
{
    const double wall_s = std::chrono::duration<double>(clock_type::now() - m_start).count();
    std::ostringstream ss;
    ss << "Progress: " << std::fixed << std::setprecision(1) << wall_s << " s wall, simulated "
       << sc_time_stamp();
    if (std::isfinite(m_end_time_s) && m_end_time_s > 0)
        ss << " (" << std::setprecision(1) << 100 * sc_time_stamp().to_seconds() / m_end_time_s << " % of "
           << sc_time(m_end_time_s, SC_SEC) << ")";
    ss << ", " << sc_delta_count() << " delta cycles";
    return ss.str();
}
