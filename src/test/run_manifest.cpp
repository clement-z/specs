// Run manifest unit tests (F-01): JSON format and key order, string escaping, null hash and null
// trace file, wiring to SPECSConfig and prepareSimulation(), the netlist file list of ParserState and
// the data files read while the circuit is built (SPAR, VLSRC, EVLSRC).
#include <gtest/gtest.h>
#include <systemc.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "parser/parse_tree.h"
#include "parser/parser_state.h"
#include "run_manifest.h"
#include "specs.h"
#include "test/utils.h"
#include "utils/sha256.h"

#include "parser.h"
#include "scanner.h"

namespace {
std::string tmp_path(const std::string &suffix)
{
    return testing::TempDir() + "specs_test_" + std::to_string(getpid()) + suffix;
}
std::string read_file(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}
size_t count(const std::string &haystack, const std::string &needle)
{
    size_t n = 0;
    for (size_t pos = haystack.find(needle); pos != std::string::npos; pos = haystack.find(needle, pos + 1))
        ++n;
    return n;
}
void write_text(const std::string &path, const std::string &text)
{
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f << text;
}
std::string sha256_of(const std::string &path)
{
    std::string hex;
    EXPECT_TRUE(spx::sha256_file_hex(path, hex)) << path;
    return hex;
}
int parse_netlist(ParseTree &tree, const std::string &text)
{
    ParserState state;
    state.current_filename = "test.cir";
    yyscan_t scanner;
    yylex_init_extra(&state, &scanner);
    YY_BUFFER_STATE buf = yy_scan_string(text.c_str(), scanner);
    yy_switch_to_buffer(buf, scanner);
    yyset_lineno(1, scanner);
    const int result = yyparse(scanner, &tree);
    yylex_destroy(scanner);
    return result;
}
}

TEST(RunManifest, JsonFormatAndKeyOrder)
{
    SPECS_TEST_HEADER()
    {
        const std::string abc = tmp_path("_abc.cir");
        const std::string missing = tmp_path("_missing.cir");
        {
            std::ofstream f(abc, std::ios::binary);
            f << "abc";
        }
        std::remove(missing.c_str());

        RunManifest m;
        m.specs_version = "v1.2-3.gabcdef0";
        m.systemc_version = "3.0.2-Accellera";
        m.seed = UINT64_MAX; // printed as an integer, not truncated
        m.command_line = { "specs", "-f", "a.cir" };
        m.netlist_files = { abc, missing };
        m.data_files = { missing, abc };
        m.trace_file = "traces/m.vcd";

        const std::string expected =
            "{\n"
            "  \"format\": \"specs-run-manifest\",\n"
            "  \"format_version\": 2,\n"
            "  \"specs_version\": \"v1.2-3.gabcdef0\",\n"
            "  \"systemc_version\": \"3.0.2-Accellera\",\n"
            "  \"seed\": 18446744073709551615,\n"
            "  \"command_line\": [\"specs\", \"-f\", \"a.cir\"],\n"
            "  \"netlist_files\": [\n"
            "    {\"path\": " + RunManifest::json_quote(abc) + ", \"sha256\": \"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\"},\n"
            "    {\"path\": " + RunManifest::json_quote(missing) + ", \"sha256\": null}\n"
            "  ],\n"
            "  \"data_files\": [\n"
            "    {\"path\": " + RunManifest::json_quote(missing) + ", \"sha256\": null},\n"
            "    {\"path\": " + RunManifest::json_quote(abc) + ", \"sha256\": \"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\"}\n"
            "  ],\n"
            "  \"trace_file\": \"traces/m.vcd\"\n"
            "}\n";
        EXPECT_EQ(m.to_json(), expected);
        // deterministic: same inputs, same bytes
        EXPECT_EQ(m.to_json(), m.to_json());
        std::remove(abc.c_str());
    }
    SPECS_TEST_FOOTER();
}

TEST(RunManifest, NullTraceFileAndNoNetlistFiles)
{
    SPECS_TEST_HEADER()
    {
        RunManifest m;
        m.command_line = { "specs", "-t", "wg" };
        const std::string json = m.to_json();
        // the data_files key is always there, right after netlist_files
        EXPECT_NE(json.find("\n  \"netlist_files\": [],\n  \"data_files\": [],\n"), std::string::npos) << json;
        EXPECT_NE(json.find("\n  \"trace_file\": null\n}\n"), std::string::npos) << json;
        EXPECT_NE(json.find("\n  \"seed\": 0,\n"), std::string::npos) << json;
    }
    SPECS_TEST_FOOTER();
}

TEST(RunManifest, JsonEscaping)
{
    SPECS_TEST_HEADER()
    {
        EXPECT_EQ(RunManifest::json_quote(""), "\"\"");
        EXPECT_EQ(RunManifest::json_quote("plain/path-1.cir"), "\"plain/path-1.cir\"");
        EXPECT_EQ(RunManifest::json_quote("a\"b"), "\"a\\\"b\"");
        EXPECT_EQ(RunManifest::json_quote("C:\\dir\\x.cir"), "\"C:\\\\dir\\\\x.cir\"");
        EXPECT_EQ(RunManifest::json_quote("\b\f\n\r\t"), "\"\\b\\f\\n\\r\\t\"");
        EXPECT_EQ(RunManifest::json_quote(std::string("\x01\x1f", 2)), "\"\\u0001\\u001f\"");
        EXPECT_EQ(RunManifest::json_quote(std::string(1, '\0')), "\"\\u0000\"");
        // not escaped: DEL, '/', and UTF-8 multibyte sequences
        EXPECT_EQ(RunManifest::json_quote("\x7f/\xc2\xb5m"), "\"\x7f/\xc2\xb5m\"");

        RunManifest m;
        m.command_line = { "specs", "--param", "name=\"x\"\\y\n" };
        m.netlist_files = { "dir with \"quotes\"\\and\ttab.cir" };
        m.trace_file = "traces/\"t\".vcd";
        const std::string json = m.to_json();
        EXPECT_NE(json.find("\"command_line\": [\"specs\", \"--param\", \"name=\\\"x\\\"\\\\y\\n\"],"), std::string::npos) << json;
        EXPECT_NE(json.find("{\"path\": \"dir with \\\"quotes\\\"\\\\and\\ttab.cir\", \"sha256\": null}"), std::string::npos) << json;
        EXPECT_NE(json.find("\"trace_file\": \"traces/\\\"t\\\".vcd\""), std::string::npos) << json;
        // no raw control character is left in the output (besides the line breaks between keys)
        for (char c : json)
            EXPECT_TRUE(c == '\n' || static_cast<unsigned char>(c) >= 0x20) << "control character " << int(c);
    }
    SPECS_TEST_FOOTER();
}

TEST(RunManifest, FromConfig)
{
    SPECS_TEST_HEADER()
    {
        SPECSConfig &c = specsGlobalConfig;
        c.seed = 42;
        c.command_line = { "specs", "-f", "a.cir" };
        c.netlist_files = { "a.cir", "b.cir" };
        c.data_files = { "s.sparam", "v.csv" };

        RunManifest m(c, "traces/a.vcd");
        EXPECT_FALSE(m.specs_version.empty());
        EXPECT_EQ(m.systemc_version, sc_release());
        EXPECT_EQ(m.seed, 42u);
        EXPECT_EQ(m.command_line, c.command_line);
        EXPECT_EQ(m.netlist_files, c.netlist_files);
        EXPECT_EQ(m.data_files, c.data_files);
        EXPECT_EQ(m.trace_file, "traces/a.vcd");
    }
    SPECS_TEST_FOOTER();
}

// prepareSimulation() writes <trace_filename>.manifest.json next to the default trace file it
// opens, and prints the same JSON once with --print-manifest (print_run_manifest)
TEST(RunManifest, WrittenAndPrintedByPrepareSimulation)
{
    SPECS_TEST_HEADER()
    {
        SPECSConfig &c = specsGlobalConfig;
        c.trace_filename = tmp_path("_prepare");
        c.command_line = { "specs", "--print-manifest", "-t", "none" };
        c.print_run_manifest = true;
        const std::string manifest_path = c.trace_filename + ".manifest.json";
        std::remove(manifest_path.c_str());

        testing::internal::CaptureStdout();
        c.prepareSimulation();
        const std::string out = testing::internal::GetCapturedStdout();

        const std::string written = read_file(manifest_path);
        EXPECT_EQ(written, RunManifest(c, c.trace_filename + ".vcd").to_json());
        EXPECT_NE(written.find("\"trace_file\": " + RunManifest::json_quote(c.trace_filename + ".vcd")), std::string::npos) << written;
        EXPECT_EQ(count(out, written), 1u) << out;
        EXPECT_FALSE(c.print_run_manifest);
        std::remove(manifest_path.c_str());
    }
    SPECS_TEST_FOOTER();
}

TEST(RunManifestDeathTest, UnwritableFileExits)
{
    SPECS_TEST_HEADER()
    {
        EXPECT_EXIT(RunManifest::write_file(tmp_path("_no_such_dir/x.manifest.json"), "{}\n"),
                    testing::ExitedWithCode(1), "Error: cannot open run manifest file \".*_no_such_dir/x.manifest.json\"");
    }
    SPECS_TEST_FOOTER();
}

// The scanners record each netlist file once, in first-open order (-f files, then .include)
TEST(RunManifest, ParserStateRecordsInputFilesOnce)
{
    SPECS_TEST_HEADER()
    {
        ParserState s;
        s.record_input_file("ignored.cir"); // no list: not recorded, no crash

        std::vector<std::string> files;
        s.input_files = &files;
        s.record_input_file("main.cir");
        s.record_input_file("circuits/sub.cir");
        s.record_input_file("main.cir");
        s.record_input_file("circuits/sub.cir");
        s.record_input_file("./main.cir"); // the path string as used to open the file
        EXPECT_EQ(files, (std::vector<std::string>{ "main.cir", "circuits/sub.cir", "./main.cir" }));
    }
    SPECS_TEST_FOOTER();
}

// SPECSConfig::record_data_file(): each data file once, in first-open order, as the path string used
TEST(RunManifest, ConfigRecordsDataFilesOnce)
{
    SPECS_TEST_HEADER()
    {
        SPECSConfig &c = specsGlobalConfig;
        EXPECT_TRUE(c.data_files.empty());
        c.record_data_file("v.csv");
        c.record_data_file("dir/s.sparam");
        c.record_data_file("v.csv");
        c.record_data_file("./v.csv");
        EXPECT_EQ(c.data_files, (std::vector<std::string>{ "v.csv", "dir/s.sparam", "./v.csv" }));
    }
    SPECS_TEST_FOOTER();
}

// The data files read while the circuit is built (SPAR file=, VLSRC/EVLSRC value files, positional or
// values=) are in the manifest once each, with their SHA-256; ParseTree::data_files(), which the run
// table of a .step/.mc plan uses without building the circuit, gives the same paths
TEST(RunManifest, DataFilesOfSparAndValueListSources)
{
    SPECS_TEST_HEADER()
    {
        specsGlobalConfig.analysis_type = SPECSConfig::TRAN;
        specsGlobalConfig.engine_timescale = SPECSConfig::ONE_FS;
        specsGlobalConfig.applyEngineResolution();

        const std::string spar = tmp_path("_wg.taylor");
        const std::string values = tmp_path("_values.csv");
        const std::string evalues = tmp_path("_evalues.csv");
        write_text(spar, "specs-taylor 1\nports in out\ncenter 1.55e-6\nrange 1.5e-6 1.6e-6\nS out in amp 0.9\n");
        write_text(values, "first line\ntime,power,wavelength\n1e-12,1e-3,1.55e-6\n");
        write_text(evalues, "first line\ntime,value\n1e-12,1.5\n");

        // two elements read each file
        const std::string netlist =
            "vlsrc1 a \"" + values + "\"\n"
            "vlsrc2 b values=\"" + values + "\"\n"
            "evlsrc1 e \"" + evalues + "\"\n"
            "spar1 a c file=\"" + spar + "\"\n"
            "spar2 b d file=\"" + spar + "\"\n";
        ParseTree tree;
        ASSERT_EQ(parse_netlist(tree, netlist), 0);
        tree.build_circuit();

        const std::vector<std::string> expected = { values, evalues, spar };
        EXPECT_EQ(tree.data_files(), expected); // element order
        const std::vector<std::string> &recorded = specsGlobalConfig.data_files;
        EXPECT_EQ(recorded.size(), 3u);
        EXPECT_TRUE(std::is_permutation(recorded.begin(), recorded.end(), expected.begin(), expected.end()));

        const std::string json = RunManifest(specsGlobalConfig, "").to_json();
        const size_t data_key = json.find("\n  \"data_files\": [\n");
        ASSERT_NE(data_key, std::string::npos) << json;
        EXPECT_NE(json.find("\"format_version\": 2,"), std::string::npos) << json;
        for (const auto &f : expected)
        {
            const std::string entry = "{\"path\": " + RunManifest::json_quote(f) + ", \"sha256\": \"" + sha256_of(f) + "\"}";
            EXPECT_EQ(count(json, entry), 1u) << entry << "\n" << json;
            EXPECT_GT(json.find(entry), data_key) << json;
        }
        for (const auto &f : expected)
            std::remove(f.c_str());
    }
    SPECS_TEST_FOOTER();
}
