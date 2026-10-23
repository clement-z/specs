#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

// Run table of a .step/.mc plan (F-02): one table for all the runs, in the directory <trace>.runs/.
//   table.manifest.json  what produced it (version, netlist files, command line, seed, plan); a resumed
//                        plan must have the same manifest
//   runs.csv             the run index, one line per finished run, appended: run, status (ok, failed,
//                        crashed), exit_code, seed, mc_run, the stepped parameters, the drawn top-level
//                        .param values, row_start, row_count, draw_start, draw_count, message. A run
//                        recorded twice (failed, then run again by a resumed plan): its last line counts
//   run.npy time_s.npy signal.npy wavelength_m.npy power_W.npy field_re.npy field_im.npy
//                        the probe tables of every run (the columns of specs --format npz, plus run),
//                        one row per sample; the rows of a run are row_start .. row_start + row_count - 1
//   signal_names.txt     the probe names, one per line (signal.npy indexes them)
//   draw_run.npy draw_name.npy draw_value.npy, draw_names.txt
//                        every random draw of every run (stream name, value)
//   logs/run<k>.log      stderr of run k, when not empty
// The .npy files are NumPy 1-D arrays (NPY 1.0) with a header of a fixed size, so that the row count in
// it can be rewritten after each append: np.load(path, mmap_mode="r") reads them without loading them.
// A run is committed by appending its rows, then its runs.csv line; at open, the columns are cut back
// to the rows that runs.csv accounts for (a crash between the two leaves extra rows).

// The results of one run, written by the run's process for the parent (a chunk file)
struct RunChunk {
    std::vector<std::string> signal_names;
    std::vector<double> time_s;
    std::vector<int32_t> signal; // index into signal_names
    std::vector<double> wavelength_m;
    std::vector<double> power_W;
    std::vector<double> field_re;
    std::vector<double> field_im;
    std::vector<std::string> draw_names; // one per draw
    std::vector<double> draw_values;
    std::vector<std::pair<std::string, std::string>> drawn_params; // name, value (as text)

    bool write(const std::string &path) const;
    bool read(const std::string &path);
};

class RunTable {
public:
    // The line of a finished run
    struct Result {
        uint64_t run = 0;
        std::string status; // ok, failed, crashed
        int exit_code = 0;
        uint64_t seed = 0;
        std::string mc_run; // "" without .mc
        std::vector<std::string> param_values; // in the order of param_columns
        std::string message;
    };

    // Creates the table in dir, or opens it to resume (same manifest), or replaces it (overwrite).
    // exit(1) with a message if it cannot
    RunTable(const std::string &dir, const std::string &manifest_json, const std::vector<std::string> &param_columns,
             const std::vector<std::string> &drawn_columns, bool overwrite);
    ~RunTable();
    RunTable(const RunTable &) = delete;
    RunTable &operator=(const RunTable &) = delete;

    // The runs whose last line says ok
    const std::set<uint64_t> &completed() const { return m_completed; }
    // Lines of runs.csv (all statuses)
    size_t lines() const { return m_lines; }
    uint64_t rows() const { return m_rows; }
    uint64_t draws() const { return m_draws; }

    // Appends the rows of the run (chunk: null for a run without results) and its runs.csv line.
    // Returns false with a message on stderr if a file cannot be written
    bool commit(const Result &result, const RunChunk *chunk);

    const std::string &dir() const { return m_dir; }
    static std::string chunk_path(const std::string &dir, uint64_t run);
    static std::string log_path(const std::string &dir, uint64_t run);

    // Fixed-size NPY 1.0 header (128 bytes) of a 1-D array of n elements of type descr
    static std::string npy_header_fixed(const std::string &descr, uint64_t n);
    // RFC 4180 field; splits a CSV line (no line breaks in fields)
    static std::string csv_field(const std::string &s);
    static std::vector<std::string> csv_split(const std::string &line);

private:
    struct Column {
        std::string name;
        std::string descr;
        size_t itemsize;
        int fd = -1;
    };

    std::string m_dir;
    std::vector<std::string> m_param_columns;
    std::vector<std::string> m_drawn_columns;
    int m_lock_fd = -1;
    int m_runs_fd = -1;
    size_t m_lines = 0;
    uint64_t m_rows = 0;
    uint64_t m_draws = 0;
    std::set<uint64_t> m_completed;

    std::vector<Column> m_data;  // run, time_s, signal, wavelength_m, power_W, field_re, field_im
    std::vector<Column> m_draw;  // draw_run, draw_name, draw_value
    std::vector<std::string> m_signal_names;
    std::map<std::string, int32_t> m_signal_index;
    std::vector<std::string> m_draw_names;
    std::map<std::string, int32_t> m_draw_index;
    int m_signal_names_fd = -1;
    int m_draw_names_fd = -1;

    std::string header_line() const;
    void open_or_create(const std::string &manifest_json, bool overwrite);
    void load_index();
    void open_columns(std::vector<Column> &columns, uint64_t n, bool create);
    int open_names(const std::string &file, std::vector<std::string> &names, std::map<std::string, int32_t> &index,
                   bool create);
    int32_t name_index(const std::string &name, std::vector<std::string> &names,
                       std::map<std::string, int32_t> &index, int fd, bool &ok);
    bool append(Column &c, const void *data, size_t n, uint64_t at);
    bool set_count(Column &c, uint64_t n);
};
