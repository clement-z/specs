#include "run_manifest.h"
#include "specs.h"
#include "utils/sha256.h"

#include <systemc.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
// Generated header (version_dep): only this file of the core includes it
#include <utils/version.h>

RunManifest::RunManifest(const SPECSConfig &config, const std::string &trace_file_name)
    : specs_version(spx::version::full)
    , systemc_version(sc_release())
    , seed(config.seed)
    , command_line(config.command_line)
    , netlist_files(config.netlist_files)
    , data_files(config.data_files)
    , trace_file(trace_file_name)
{
}

std::string RunManifest::to_json() const
{
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"format\": \"specs-run-manifest\",\n";
    ss << "  \"format_version\": 2,\n";
    ss << "  \"specs_version\": " << json_quote(specs_version) << ",\n";
    ss << "  \"systemc_version\": " << json_quote(systemc_version) << ",\n";
    ss << "  \"seed\": " << seed << ",\n";

    ss << "  \"command_line\": [";
    for (size_t i = 0; i < command_line.size(); ++i)
        ss << (i ? ", " : "") << json_quote(command_line[i]);
    ss << "],\n";

    ss << "  \"netlist_files\": " << json_file_list(netlist_files) << ",\n";
    ss << "  \"data_files\": " << json_file_list(data_files) << ",\n";

    ss << "  \"trace_file\": " << (trace_file.empty() ? "null" : json_quote(trace_file)) << "\n";
    ss << "}\n";
    return ss.str();
}

std::string RunManifest::json_file_list(const std::vector<std::string> &paths)
{
    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < paths.size(); ++i)
    {
        std::string hex;
        ss << (i ? "," : "") << "\n    {\"path\": " << json_quote(paths[i]) << ", \"sha256\": ";
        if (spx::sha256_file_hex(paths[i], hex))
            ss << json_quote(hex);
        else
            ss << "null";
        ss << "}";
    }
    ss << (paths.empty() ? "" : "\n  ") << "]";
    return ss.str();
}

void RunManifest::write_file(const std::string &filename, const std::string &json)
{
    std::ofstream outfile;
    outfile.open(filename, std::ios::out | std::ios::trunc);
    if (!outfile)
    {
        std::cerr << "Error: cannot open run manifest file \"" << filename << "\"" << std::endl;
        exit(1);
    }
    outfile << json;
    outfile.close();
    if (!outfile)
    {
        std::cerr << "Error: cannot write run manifest file \"" << filename << "\"" << std::endl;
        exit(1);
    }
}

std::string RunManifest::json_quote(const std::string &s)
{
    std::string out = "\"";
    for (unsigned char c : s)
    {
        switch (c)
        {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\b':
                out += "\\b";
                break;
            case '\f':
                out += "\\f";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (c < 0x20)
                {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                }
                else
                {
                    // other bytes as they are (UTF-8 is passed through)
                    out += static_cast<char>(c);
                }
                break;
        }
    }
    out += '"';
    return out;
}
