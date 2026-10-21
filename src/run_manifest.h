#pragma once

#include <cstdint>
#include <string>
#include <vector>

class SPECSConfig;

// Run manifest (F-01): what is needed to reproduce a run, i.e. the SPECS and SystemC versions,
// the seed, the command line and the SHA-256 of every netlist file read by the parser.
// SPECSConfig::prepareSimulation() writes it next to the default trace file, as
// <trace_filename>.manifest.json, and prints it to stdout with --print-manifest.
// The JSON is byte-reproducible for the same build and inputs: no dates, wall times, hostnames
// or absolute working directory.
struct RunManifest {
    std::string specs_version;
    std::string systemc_version;
    uint64_t seed = 0;
    std::vector<std::string> command_line;   // argv, as received
    std::vector<std::string> netlist_files;  // paths as opened by the parser, in first-open order
    std::string trace_file;                  // empty: no trace file is written ("trace_file": null)

    RunManifest() {}

    // Manifest of a run configured by config (seed, command line, netlist files), with the SPECS
    // and SystemC versions of this build
    RunManifest(const SPECSConfig &config, const std::string &trace_file_name);

    // One JSON object, one key per line, keys in a fixed order. The netlist files are hashed when
    // this is called ("sha256": null for a file that cannot be read).
    std::string to_json() const;

    // Write json (a to_json() result) to filename; error message and exit(1) if it cannot be written
    static void write_file(const std::string &filename, const std::string &json);

    // JSON string literal of s, quotes included: escapes '"', '\\' and the control characters
    static std::string json_quote(const std::string &s);
};
