#include "run_plan.h"

#include <cerrno>
#include <charconv>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "run_manifest.h"
#include "utils/rng.h"

using std::string;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;

RunPlan::RunPlan(const ParsePlan &plan, uint64_t master_seed)
    : m_plan(plan)
    , m_master_seed(master_seed)
{
    for (const auto &s : m_plan.steps)
    {
        if (s.values.empty() || m_size > max_runs() / s.values.size())
        {
            m_size = 0;
            return;
        }
        m_size *= s.values.size();
    }
    if (m_plan.mc)
    {
        if (m_plan.mc_runs > max_runs() / m_size)
        {
            m_size = 0;
            return;
        }
        m_size *= m_plan.mc_runs;
    }
}

RunPlan::Point RunPlan::point(uint64_t run) const
{
    Point p;
    p.run = run;
    uint64_t rest = run;
    if (m_plan.mc)
    {
        p.mc = true;
        p.mc_run = rest % m_plan.mc_runs;
        rest /= m_plan.mc_runs;
        p.seed = mc_run_seed(m_master_seed, p.mc_run);
    }
    else
        p.seed = m_master_seed;
    // the last .step line is the innermost loop
    p.params.resize(m_plan.steps.size());
    for (size_t i = m_plan.steps.size(); i-- > 0;)
    {
        const auto &s = m_plan.steps[i];
        p.params[i] = { s.param, s.values[rest % s.values.size()] };
        rest /= s.values.size();
    }
    return p;
}

string RunPlan::describe(const Point &p) const
{
    string out;
    for (const auto &kv : p.params)
        out += kv.first + "=" + format_value(kv.second) + " ";
    if (p.mc)
        out += "mc_run=" + std::to_string(p.mc_run) + " ";
    return out + "seed=" + std::to_string(p.seed);
}

vector<string> RunPlan::param_names() const
{
    vector<string> names;
    for (const auto &s : m_plan.steps)
        names.push_back(s.param);
    return names;
}

uint64_t RunPlan::mc_run_seed(uint64_t master_seed, uint64_t k)
{
    spx::Rng rng = spx::rng_for(master_seed, ".mc/" + std::to_string(k));
    return rng.next();
}

string RunPlan::format_value(const Variable &v)
{
    if (v.type == Variable::DOUBLE)
    {
        char buf[32];
        const auto res = std::to_chars(buf, buf + sizeof(buf), v.num);
        return string(buf, res.ptr);
    }
    if (v.type == Variable::STRING)
        return v.str;
    return v.get_str();
}

string table_manifest_json(const RunManifest &m, const RunPlan &plan, const vector<string> &drawn)
{
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"format\": \"specs-run-table\",\n";
    ss << "  \"format_version\": 2,\n";
    ss << "  \"specs_version\": " << RunManifest::json_quote(m.specs_version) << ",\n";
    ss << "  \"systemc_version\": " << RunManifest::json_quote(m.systemc_version) << ",\n";
    ss << "  \"seed\": " << m.seed << ",\n";
    ss << "  \"command_line\": [";
    for (size_t i = 0; i < m.command_line.size(); ++i)
        ss << (i ? ", " : "") << RunManifest::json_quote(m.command_line[i]);
    ss << "],\n";
    ss << "  \"netlist_files\": " << RunManifest::json_file_list(m.netlist_files) << ",\n";
    ss << "  \"data_files\": " << RunManifest::json_file_list(m.data_files) << ",\n";
    ss << "  \"steps\": [";
    for (size_t i = 0; i < plan.plan().steps.size(); ++i)
    {
        const auto &s = plan.plan().steps[i];
        ss << (i ? "," : "") << "\n    {\"param\": " << RunManifest::json_quote(s.param) << ", \"values\": [";
        for (size_t j = 0; j < s.values.size(); ++j)
            ss << (j ? ", " : "") << RunPlan::format_value(s.values[j]);
        ss << "]}";
    }
    ss << (plan.plan().steps.empty() ? "" : "\n  ") << "],\n";
    ss << "  \"mc_runs\": ";
    if (plan.plan().mc)
        ss << plan.plan().mc_runs;
    else
        ss << "null";
    ss << ",\n";
    ss << "  \"runs\": " << plan.size() << ",\n";
    ss << "  \"drawn_parameters\": [";
    for (size_t i = 0; i < drawn.size(); ++i)
        ss << (i ? ", " : "") << RunManifest::json_quote(drawn[i]);
    ss << "]\n";
    ss << "}\n";
    return ss.str();
}

string last_log_line(const string &log_path)
{
    std::ifstream f(log_path, std::ios::binary);
    string line, last, last_error;
    while (std::getline(f, line))
    {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
            line.pop_back();
        if (!line.empty())
            last = line;
        if (line.rfind("Error", 0) == 0 || line.rfind("Parsing error", 0) == 0)
            last_error = line;
    }
    // the error, rather than the explanations that may follow it
    if (!last_error.empty())
        last = last_error;
    if (last.size() > 500)
        last = last.substr(0, 497) + "...";
    return last;
}

namespace {

// The run's process: stdout on /dev/null, stderr in its log file
void redirect_child_output(const string &log_path)
{
    const int null_fd = ::open("/dev/null", O_WRONLY);
    const int log_fd = ::open(log_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (null_fd < 0 || log_fd < 0 || ::dup2(null_fd, STDOUT_FILENO) < 0 || ::dup2(log_fd, STDERR_FILENO) < 0)
        _exit(125);
    ::close(null_fd);
    ::close(log_fd);
}

}

RunnerOutcome run_plan(const RunPlan &plan, RunTable &table, unsigned jobs)
{
    RunnerOutcome outcome;
    if (jobs < 1)
        jobs = 1;

    // runs to do, in run order (a resumed plan skips the runs recorded ok); not listed in memory,
    // a plan may have up to 2^31 - 1 runs
    const auto done = [&](uint64_t run) { return table.completed().count(run) != 0; };
    const auto next_todo = [&](uint64_t run) {
        while (run < plan.size() && done(run))
            ++run;
        return run;
    };
    uint64_t n_done = 0;
    for (uint64_t run : table.completed())
        n_done += run < plan.size();
    const uint64_t n_todo = plan.size() - n_done;
    cout << "Run table: " << table.dir() << " (" << plan.size() << " runs, " << n_done << " already done, "
         << n_todo << " to run, " << jobs << " at a time)" << endl;

    uint64_t next_start = next_todo(0);  // next run to start
    uint64_t next_commit = next_start;   // next run to commit
    std::map<pid_t, uint64_t> running;
    std::map<uint64_t, RunTable::Result> finished; // not committed yet
    uint64_t n_ok = 0, n_failed = 0;

    // on an error of the parent: the runs still running are stopped, so that they do not hold the
    // table's lock after specs exits
    const auto stop_children = [&]() {
        for (const auto &r : running)
            ::kill(r.first, SIGKILL);
        for (const auto &r : running)
            ::waitpid(r.first, nullptr, 0);
        running.clear();
    };

    while (next_commit < plan.size())
    {
        while (running.size() < jobs && next_start < plan.size())
        {
            const uint64_t run = next_start;
            next_start = next_todo(next_start + 1);
            // nothing buffered may be written twice (by the parent and by the child)
            cout << std::flush;
            cerr << std::flush;
            const pid_t pid = ::fork();
            if (pid < 0)
            {
                cerr << "Error: cannot start the process of run " << run << ": " << strerror(errno) << endl;
                stop_children();
                outcome.exit_code = 1;
                return outcome;
            }
            if (pid == 0)
            {
                redirect_child_output(RunTable::log_path(table.dir(), run));
                outcome.child = true;
                outcome.run = run;
                return outcome;
            }
            running.emplace(pid, run);
        }

        int status = 0;
        const pid_t pid = ::waitpid(-1, &status, 0);
        if (pid < 0)
        {
            if (errno == EINTR)
                continue;
            cerr << "Error: waitpid: " << strerror(errno) << endl;
            stop_children();
            outcome.exit_code = 1;
            return outcome;
        }
        const auto it = running.find(pid);
        if (it == running.end())
            continue;
        const uint64_t run = it->second;
        running.erase(it);

        const RunPlan::Point p = plan.point(run);
        RunTable::Result r;
        r.run = run;
        r.seed = p.seed;
        r.mc_run = p.mc ? std::to_string(p.mc_run) : "";
        for (const auto &kv : p.params)
            r.param_values.push_back(RunPlan::format_value(kv.second));
        const string log = RunTable::log_path(table.dir(), run);
        r.message = last_log_line(log);
        if (WIFSIGNALED(status))
        {
            r.status = "crashed";
            r.exit_code = 128 + WTERMSIG(status);
            r.message = "killed by signal " + std::to_string(WTERMSIG(status)) + " (" + strsignal(WTERMSIG(status))
                        + ")" + (r.message.empty() ? "" : ": " + r.message);
        }
        else
        {
            r.exit_code = WEXITSTATUS(status);
            r.status = r.exit_code == 0 ? "ok" : "failed";
        }
        finished.emplace(run, r);

        // commit in run order
        while (next_commit < plan.size() && finished.count(next_commit))
        {
            const uint64_t c = next_commit;
            next_commit = next_todo(next_commit + 1);
            RunTable::Result &res = finished.at(c);
            RunChunk chunk;
            const string chunk_file = RunTable::chunk_path(table.dir(), c);
            bool has_chunk = false;
            if (res.status == "ok")
            {
                has_chunk = chunk.read(chunk_file);
                if (!has_chunk)
                {
                    res.status = "failed";
                    res.message = "the run wrote no result" + (res.message.empty() ? "" : ": " + res.message);
                }
            }
            if (!table.commit(res, has_chunk ? &chunk : nullptr))
            {
                stop_children();
                outcome.exit_code = 1;
                return outcome;
            }
            std::remove(chunk_file.c_str());
            // keep the log only if it has something
            std::ifstream log_file(RunTable::log_path(table.dir(), c));
            if (log_file.peek() == std::ifstream::traits_type::eof())
                std::remove(RunTable::log_path(table.dir(), c).c_str());
            if (res.status == "ok")
                ++n_ok;
            else
                ++n_failed;
            cout << "run " << c << ": " << res.status;
            if (res.status != "ok")
                cout << " (exit " << res.exit_code << ")" << (res.message.empty() ? "" : ": " + res.message);
            cout << endl;
            finished.erase(c);
        }
    }

    cout << "Runs: " << plan.size() << ", " << n_ok + n_done << " ok (" << n_done << " from before), " << n_failed
         << " failed" << endl;
    if (n_failed)
    {
        cerr << "Error: " << n_failed << " run" << (n_failed > 1 ? "s" : "") << " failed (see " << table.dir()
             << "/runs.csv and logs/)" << endl;
        outcome.exit_code = 1;
    }
    return outcome;
}
