#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "optical_output_port.h"

// Run statistics (F-15, specs --stats): the activity counters of the optical output ports
// (OpticalOutputPortStats), summed over the run. Sections:
//  - SUMMARY: run totals (always written)
//  - PORTS: one row per output port (port, net, writer, wavelengths, counters)
//  - DEVICES: one row per device type (the type of the module that owns the port)
// SPECSConfig writes the JSON next to the default trace file, as <trace_filename>.stats.json
// (or to --stats-file), and prints it with --print-stats. The JSON is reproducible for the same
// build and inputs except "wall_time_s".
struct RunStats {
    enum Section {
        SUMMARY = 1,
        PORTS = 2,
        DEVICES = 4,
    };

    // Counters of a group of ports (the run, a device type)
    struct Totals {
        uint64_t output_ports = 0;
        uint64_t scheduled = 0;
        uint64_t coalesced = 0;
        uint64_t processed = 0;
        uint64_t emitted = 0;
        uint64_t suppressed_abstol = 0;
        uint64_t suppressed_reltol = 0;
        uint64_t suppressed_snap = 0;
        uint64_t pending_max = 0;
        uint64_t pending_sum = 0;
        uint64_t pending_end = 0; // entries still pending at the end of the run
        bool has_emitted = false;
        double last_emission_s = 0;

        void add(const OpticalOutputPortStats &s, uint64_t pending);
    };

    struct PortRow {
        std::string port;
        std::string net; // "" if unbound
        std::string writer; // the module that owns the port ("" if none)
        std::vector<double> wavelengths_m;
        Totals counters;
    };

    struct DeviceRow {
        std::string type;
        uint64_t instances = 0;
        Totals counters;
    };

    unsigned sections = SUMMARY;
    std::string analysis; // "OP", "DC", "TRAN"
    double abstol = 0;
    double reltol = 0;
    double snap_factor = 0;
    uint64_t max_events = 0;
    bool event_cap_reached = false;
    double end_time_s = 0; // simulated time at the end of the run
    double wall_time_s = 0;
    Totals totals;
    std::vector<PortRow> ports; // in name order
    std::vector<DeviceRow> devices; // in type order

    RunStats() {}

    // Sums the counters of oops (in that order; sc_get_all_module_by_type() gives name order)
    void collect(const std::vector<OpticalOutputPort *> &oops);

    // One JSON object, keys in a fixed order, rows on one line each
    std::string to_json() const;

    // Sections from the value of --stats: a comma-separated list of summary, ports, devices
    // (summary is always included). Returns false if a name is unknown
    static bool parse_sections(const std::string &value, unsigned &sections);

    // Type name of a module, as written in the C++ sources (demangled)
    static std::string type_name(const sc_object &obj);

    // Shortest decimal representation that reads back as the same double
    static std::string json_number(double v);
};
