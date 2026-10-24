#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "optical_signal.h"

// Table export of the probed signals (F-05, specs --format csv|npz): the samples that PROBE and MLPROBE
// trace in the VCD, one row per time step, probe and wavelength: the last sample of that wavelength in
// that time step (the delta cycles of a time step, e.g. the iterations of a .op/.dc fixed point, are
// not physical time; the VCD also only shows the end of a time step). Rows in time order; within a time
// step, in the order of the first sample of each (probe, wavelength). Columns:
//   time_s        simulation time (s)
//   signal        name of the probe (CSV); in the .npz, an index into the array signal_names
//   wavelength_m  wavelength of the sample (m)
//   power_W       |E|^2 (W)
//   field_re      Re(E) (sqrt(W))
//   field_im      Im(E) (sqrt(W))
// CSV: a header line, then one line per row; numbers in the shortest form that reads back to the same
// double. NPZ: a NumPy .npz, i.e. a zip archive (stored, not compressed) of one .npy array per column
// (signal: int32) plus signal_names (unicode strings, every probe, in registration order), written
// without any dependency. The CSV is written as the simulation runs; the .npz is kept in memory and
// written by close().
class TraceTable {
public:
    // csv_filename, npz_filename: the files to write (empty: not written). Both are opened here;
    // exit(1) with a message if one cannot be
    TraceTable(const std::string &csv_filename, const std::string &npz_filename);
    ~TraceTable();

    // Index of a signal (probe), registered at the first call with that name
    uint32_t signal_index(const std::string &name);
    // One sample of a signal (time_s: the current simulation time, never decreasing)
    void append(double time_s, uint32_t signal, const OpticalSignal &s);
    // A zero row (power and field 0) at wavelength_m, which need not be a registered wavelength: a listed
    // wavelength that the net of an MLPROBE does not carry at a .dc point (B-186). Appended once per time
    // step, signal and wavelength: no later sample replaces it
    void appendZero(double time_s, uint32_t signal, double wavelength_m);
    // Drops the sample of (signal, wavelength id) of the current time step, if any: a PROBE that
    // latches a smaller wavelength within the time step of its latch (PD5)
    void discard(double time_s, uint32_t signal, uint32_t wavelength_id);
    // Writes the .npz and closes the files; the samples appended afterwards are dropped. Returns
    // false (with a message on stderr) if a file could not be written
    bool close();

    const std::vector<std::string> &signal_names() const { return m_signal_names; }
    size_t rows() const { return m_rows; }

    // Keep the columns in memory without a .npz file (call before the first sample): the run of a
    // .step/.mc plan hands them to the run table (F-02)
    void keepColumns() { m_keep_columns = true; }
    const std::vector<double> &time_s() const { return m_time_s; }
    const std::vector<int32_t> &signal() const { return m_signal; }
    const std::vector<double> &wavelength_m() const { return m_wavelength_m; }
    const std::vector<double> &power_W() const { return m_power_W; }
    const std::vector<double> &field_re() const { return m_field_re; }
    const std::vector<double> &field_im() const { return m_field_im; }

    // Helpers of the .npz writer, public for the unit tests
    // CRC-32 of the zip format (polynomial 0xEDB88320), continued from crc
    static uint32_t crc32(const void *data, size_t size, uint32_t crc = 0);
    // NPY 1.0 preamble (magic, version, header) of a 1-D array of n elements of type descr, padded so
    // that the data starts at a multiple of 64 bytes
    static std::string npy_header(const std::string &descr, size_t n);

private:
    std::string m_csv_filename;
    std::string m_npz_filename;
    std::ofstream m_csv;
    std::ofstream m_npz;
    bool m_closed = false;
    bool m_keep_columns = false;
    size_t m_rows = 0;

    std::vector<std::string> m_signal_names;
    std::vector<std::string> m_csv_fields; // the names as CSV fields
    std::string m_line;                    // CSV line buffer

    // Rows of the current time step, one per (signal, wavelength id), written when the time advances
    struct Row {
        double time_s;
        uint32_t signal;
        double wavelength_m;
        double power_W;
        double field_re;
        double field_im;
        bool dropped = false; // discard(): not written
    };
    double m_pending_time_s = -1;
    std::vector<Row> m_pending;
    std::unordered_map<uint64_t, size_t> m_pending_index; // (signal << 32 | wavelength id) -> m_pending

    // .npz columns
    std::vector<double> m_time_s;
    std::vector<int32_t> m_signal;
    std::vector<double> m_wavelength_m;
    std::vector<double> m_power_W;
    std::vector<double> m_field_re;
    std::vector<double> m_field_im;

    void writePending();
    bool writeNpz();
};
