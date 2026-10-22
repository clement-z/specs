// Run manifest unit tests (F-01): JSON format and key order, string escaping, null hash and null
// trace file, wiring to SPECSConfig and prepareSimulation(), and the netlist file list of ParserState.
#include <gtest/gtest.h>
#include <systemc.h>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "parser/parser_state.h"
#include "run_manifest.h"
#include "specs.h"
#include "test/utils.h"

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
        m.trace_file = "traces/m.vcd";

        const std::string expected =
            "{\n"
            "  \"format\": \"specs-run-manifest\",\n"
            "  \"format_version\": 1,\n"
            "  \"specs_version\": \"v1.2-3.gabcdef0\",\n"
            "  \"systemc_version\": \"3.0.2-Accellera\",\n"
            "  \"seed\": 18446744073709551615,\n"
            "  \"command_line\": [\"specs\", \"-f\", \"a.cir\"],\n"
            "  \"netlist_files\": [\n"
            "    {\"path\": " + RunManifest::json_quote(abc) + ", \"sha256\": \"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\"},\n"
            "    {\"path\": " + RunManifest::json_quote(missing) + ", \"sha256\": null}\n"
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
        EXPECT_NE(json.find("\n  \"netlist_files\": [],\n"), std::string::npos) << json;
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

        RunManifest m(c, "traces/a.vcd");
        EXPECT_FALSE(m.specs_version.empty());
        EXPECT_EQ(m.systemc_version, sc_release());
        EXPECT_EQ(m.seed, 42u);
        EXPECT_EQ(m.command_line, c.command_line);
        EXPECT_EQ(m.netlist_files, c.netlist_files);
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
