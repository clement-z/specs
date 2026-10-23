#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "parser/parse_tree.h"
#include "run_table.h"

// Run plan of a netlist with .step and .mc lines (F-02). Runs are numbered 0 .. size() - 1 over the
// nested loops: the .step lines in netlist order (the first one outermost), then the Monte-Carlo runs
// (innermost). Without .mc every run uses the master seed; Monte-Carlo run k uses mc_run_seed(master, k),
// at every step point (the same draws for every step value: common random numbers).
class RunPlan {
public:
    struct Point {
        uint64_t run = 0;
        std::vector<std::pair<std::string, Variable>> params; // stepped parameters
        bool mc = false;
        uint64_t mc_run = 0;
        uint64_t seed = 0;
    };

    RunPlan(const ParsePlan &plan, uint64_t master_seed);

    // Number of runs; 0 if it exceeds max_runs()
    uint64_t size() const { return m_size; }
    static uint64_t max_runs() { return 2147483647; } // the run column is an int32
    uint64_t master_seed() const { return m_master_seed; }
    const ParsePlan &plan() const { return m_plan; }
    Point point(uint64_t run) const;
    // "x=1 y=2.5 mc_run=3 seed=..." (for the listings)
    std::string describe(const Point &p) const;
    // The parameter columns of the run table (the stepped parameters)
    std::vector<std::string> param_names() const;

    // Seed of Monte-Carlo run k: the first output of the stream rng_for(master_seed, ".mc/<k>")
    static uint64_t mc_run_seed(uint64_t master_seed, uint64_t k);
    // A parameter value as text: an integer in decimal, a double in the shortest form that reads back
    // to the same double, a string as is
    static std::string format_value(const Variable &v);

private:
    ParsePlan m_plan;
    uint64_t m_master_seed;
    uint64_t m_size = 1;
};

// Runs the plan's points that the table does not hold yet, one process each (fork(), before any
// SystemC elaboration), at most `jobs` at a time, and commits them in run order. In the parent: returns
// child = false and the exit status (0 if every run is ok). In a run's process: returns child = true and
// the run to simulate, with stdout on /dev/null and stderr in the run's log file; that process
// simulates the run, writes RunTable::chunk_path(), and exits 0.
struct RunnerOutcome {
    bool child = false;
    uint64_t run = 0;
    int exit_code = 0;
};
RunnerOutcome run_plan(const RunPlan &plan, RunTable &table, unsigned jobs);

// Manifest of a run table (RunTable): the fields of the run manifest m (versions, master seed, command
// line, netlist files with their SHA-256; no trace file), then the plan: the stepped values, the number
// of Monte-Carlo runs, the number of runs and the drawn parameters
struct RunManifest;
std::string table_manifest_json(const RunManifest &m, const RunPlan &plan, const std::vector<std::string> &drawn);

// Message of a run: the last line of its log (stderr) that starts with "Error", else its last non-empty
// line, at most 500 characters
std::string last_log_line(const std::string &log_path);
