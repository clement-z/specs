// Run plans of .step/.mc (F-02): the .step/.mc lines, the numbering of the runs and their seeds, the run
// table (columns, index, resume and repair) and the runner (one process per run, failures recorded).
#include <gtest/gtest.h>

#include <csignal>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "parser/parse_tree.h"
#include "parser/parser_state.h"
#include "run_plan.h"
#include "run_table.h"
#include "test/utils.h"
#include "utils/rng.h"

#include "parser.h"
#include "scanner.h"

namespace fs = std::filesystem;

namespace {

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

std::string read_text(const fs::path &path)
{
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// A fresh directory for a table (removed first)
std::string table_dir(const std::string &name)
{
    const fs::path dir = fs::path(testing::TempDir()) / ("specs_test_" + std::to_string(getpid()) + "_" + name);
    fs::remove_all(dir);
    return dir.string();
}

RunChunk small_chunk(double value, const std::string &probe)
{
    RunChunk c;
    c.signal_names = { probe, "ROOT/P2" };
    c.time_s = { 0, 1e-12 };
    c.signal = { 1, 0 };
    c.wavelength_m = { 1.55e-6, 1.55e-6 };
    c.power_W = { value, 2 * value };
    c.field_re = { 0.1, 0.2 };
    c.field_im = { -0.1, 0 };
    c.draw_names = { "ROOT/.param/a#0" };
    c.draw_values = { value };
    c.drawn_params = { { "a", std::to_string(value) } };
    return c;
}

// Values of a 1-D .npy column written by the run table
template <typename T>
std::vector<T> read_column(const std::string &dir, const std::string &name)
{
    const std::string raw = read_text(fs::path(dir) / (name + ".npy"));
    std::vector<T> v((raw.size() - 128) / sizeof(T));
    if (!v.empty())
        std::memcpy(v.data(), raw.data() + 128, v.size() * sizeof(T));
    return v;
}

}

TEST(RunPlan, StepLines)
{
    SPECS_TEST_HEADER()
    {
        ParseTree tree;
        ASSERT_EQ(parse_netlist(tree, ".param n=1 x=0 y=0\n"
                                      ".step param n 1 3 1\n"
                                      ".STEP PARAM x 0 0.3 0.1\n"
                                      ".step param y list -1 2.5 {2*2}\n"
                                      ".mc 4 seed=9\n"), 0);
        ASSERT_EQ(tree.plan.steps.size(), 3u);
        const auto &n = tree.plan.steps[0];
        EXPECT_EQ(n.param, "n");
        ASSERT_EQ(n.values.size(), 3u);
        EXPECT_EQ(n.values[2].type, Variable::INTEGER); // integers stay integers
        EXPECT_EQ(n.values[2].inum, 3);
        // like .dc: accumulated, the stop value included (within 1e-9 of a step)
        const auto &x = tree.plan.steps[1];
        ASSERT_EQ(x.values.size(), 4u);
        EXPECT_EQ(x.values[3].as_double(), 0.1 + 0.1 + 0.1);
        const auto &y = tree.plan.steps[2];
        ASSERT_EQ(y.values.size(), 3u);
        EXPECT_EQ(y.values[0].as_integer(), -1);
        EXPECT_EQ(y.values[1].as_double(), 2.5);
        EXPECT_EQ(y.values[2].as_double(), 4);
        EXPECT_TRUE(tree.plan.mc);
        EXPECT_EQ(tree.plan.mc_runs, 4u);
        EXPECT_TRUE(tree.plan.mc_seed_given);
        EXPECT_EQ(tree.plan.mc_seed, 9u);
        EXPECT_EQ(n.where, "test.cir:2");
    }
    SPECS_TEST_FOOTER();
}

TEST(RunPlanDeathTest, InvalidStepAndMcLines)
{
    SPECS_TEST_HEADER()
    {
        const auto failed = testing::ExitedWithCode(1);
        ParseTree t1, t2, t3, t4, t5, t6, t7;
        EXPECT_EXIT(parse_netlist(t1, ".step x 1 2 1\n"), failed, "syntax error");
        EXPECT_EXIT(parse_netlist(t2, ".step para x 1 2 1\n"), failed, "expected .step param NAME");
        EXPECT_EXIT(parse_netlist(t3, ".step param x 2 1 1\n"), failed, "expected start < stop and step > 0");
        EXPECT_EXIT(parse_netlist(t4, ".step param x 1 2\n"), failed, "expected 3 values");
        EXPECT_EXIT(parse_netlist(t5, ".step param x 1 2 1\n.step param x list 3\n"), failed, "already stepped");
        EXPECT_EXIT(parse_netlist(t6, ".mc 0\n"), failed, "number of runs must be an integer from 1");
        EXPECT_EXIT(parse_netlist(t7, ".mc 2\n.mc 3\n"), failed, "one .mc line");
    }
    SPECS_TEST_FOOTER();
}

// runs numbered over the nested loops: the first .step outermost, .mc innermost
TEST(RunPlan, NumberingAndSeeds)
{
    SPECS_TEST_HEADER()
    {
        ParsePlan pp;
        pp.steps.push_back({ "a", { Variable(1), Variable(2) }, "" });
        pp.steps.push_back({ "b", { Variable(0.5), Variable(1.5), Variable(2.5) }, "" });
        pp.mc = true;
        pp.mc_runs = 2;
        const RunPlan plan(pp, 11);
        ASSERT_EQ(plan.size(), 12u);
        const RunPlan::Point p7 = plan.point(7); // a index 1, b index 0, mc 1
        EXPECT_EQ(p7.params[0].second.as_integer(), 2);
        EXPECT_EQ(p7.params[1].second.as_double(), 0.5);
        EXPECT_EQ(p7.mc_run, 1u);
        EXPECT_EQ(p7.seed, RunPlan::mc_run_seed(11, 1));
        // the same draws at every step point
        EXPECT_EQ(plan.point(1).seed, p7.seed);
        EXPECT_NE(plan.point(0).seed, p7.seed);
        spx::Rng rng = spx::rng_for(11, ".mc/1");
        EXPECT_EQ(RunPlan::mc_run_seed(11, 1), rng.next());
        EXPECT_EQ(plan.describe(p7), "a=2 b=0.5 mc_run=1 seed=" + std::to_string(p7.seed));
        EXPECT_EQ(plan.param_names(), (std::vector<std::string>{ "a", "b" }));

        // without .mc: the master seed
        ParsePlan steps_only;
        steps_only.steps.push_back({ "a", { Variable(1) }, "" });
        EXPECT_EQ(RunPlan(steps_only, 5).point(0).seed, 5u);

        // too many runs: size 0
        ParsePlan big;
        big.mc = true;
        big.mc_runs = 1000000000;
        big.steps.push_back({ "a", { Variable(1), Variable(2), Variable(3) }, "" });
        EXPECT_EQ(RunPlan(big, 0).size(), 0u);

        EXPECT_EQ(RunPlan::format_value(Variable(0.1)), "0.1");
        EXPECT_EQ(RunPlan::format_value(Variable(0.1 + 0.2)), "0.30000000000000004");
        EXPECT_EQ(RunPlan::format_value(Variable(int64_t(-3))), "-3");
    }
    SPECS_TEST_FOOTER();
}

TEST(RunTable, ChunkRoundTrip)
{
    SPECS_TEST_HEADER()
    {
        const std::string dir = table_dir("chunk");
        fs::create_directories(dir);
        const RunChunk c = small_chunk(1.5, "ROOT/P1");
        const std::string path = (fs::path(dir) / "c.chunk").string();
        ASSERT_TRUE(c.write(path));
        EXPECT_FALSE(fs::exists(path + ".tmp"));
        RunChunk r;
        ASSERT_TRUE(r.read(path));
        EXPECT_EQ(r.signal_names, c.signal_names);
        EXPECT_EQ(r.power_W, c.power_W);
        EXPECT_EQ(r.signal, c.signal);
        EXPECT_EQ(r.draw_names, c.draw_names);
        EXPECT_EQ(r.drawn_params, c.drawn_params);
        // a truncated chunk is rejected
        const std::string raw = read_text(path);
        std::ofstream(path, std::ios::binary | std::ios::trunc) << raw.substr(0, raw.size() - 3);
        EXPECT_FALSE(r.read(path));
        EXPECT_FALSE(r.read(path + ".missing"));
    }
    SPECS_TEST_FOOTER();
}

TEST(RunTable, CommitResumeAndRepair)
{
    SPECS_TEST_HEADER()
    {
        const std::string dir = table_dir("table");
        const std::string manifest = "{\"format\": \"test\"}\n";
        {
            RunTable t(dir, manifest, { "x" }, { "a" }, false);
            RunChunk c0 = small_chunk(1, "ROOT/P1");
            RunChunk c2 = small_chunk(3, "ROOT/P3"); // a new probe name
            ASSERT_TRUE(t.commit({ 0, "ok", 0, 42, "", { "0.5" }, "" }, &c0));
            ASSERT_TRUE(t.commit({ 1, "failed", 1, 42, "", { "1.5" }, "Error: x, \"y\"" }, nullptr));
            ASSERT_TRUE(t.commit({ 2, "ok", 0, 42, "", { "2.5" }, "" }, &c2));
            EXPECT_EQ(t.completed(), (std::set<uint64_t>{ 0, 2 }));
            EXPECT_EQ(t.rows(), 4u);
        }
        EXPECT_EQ(read_text(fs::path(dir) / "runs.csv"),
                  "run,status,exit_code,seed,mc_run,x,a,row_start,row_count,draw_start,draw_count,message\n"
                  "0,ok,0,42,,0.5,1.000000,0,2,0,1,\n"
                  "1,failed,1,42,,1.5,,2,0,1,0,\"Error: x, \"\"y\"\"\"\n"
                  "2,ok,0,42,,2.5,3.000000,2,2,1,1,\n");
        EXPECT_EQ(read_text(fs::path(dir) / "signal_names.txt"), "ROOT/P1\nROOT/P2\nROOT/P3\n");
        EXPECT_EQ(read_column<int32_t>(dir, "run"), (std::vector<int32_t>{ 0, 0, 2, 2 }));
        EXPECT_EQ(read_column<int32_t>(dir, "signal"), (std::vector<int32_t>{ 1, 0, 1, 2 }));
        EXPECT_EQ(read_column<double>(dir, "power_W"), (std::vector<double>{ 1, 2, 3, 6 }));
        EXPECT_EQ(read_column<int32_t>(dir, "draw_run"), (std::vector<int32_t>{ 0, 2 }));
        const std::string header = read_text(fs::path(dir) / "power_W.npy").substr(0, 128);
        EXPECT_EQ(header, RunTable::npy_header_fixed("<f8", 4));
        EXPECT_NE(header.find("'shape': (4,)"), std::string::npos);

        // an interrupted commit: rows past the index and a partial index line are dropped at open
        {
            std::ofstream(fs::path(dir) / "power_W.npy", std::ios::binary | std::ios::app) << std::string(16, 'x');
            std::ofstream(fs::path(dir) / "runs.csv", std::ios::binary | std::ios::app) << "3,ok,0,42,,3.5,,4,2";
        }
        {
            RunTable t(dir, manifest, { "x" }, { "a" }, false);
            EXPECT_EQ(t.completed(), (std::set<uint64_t>{ 0, 2 }));
            EXPECT_EQ(t.lines(), 3u);
            EXPECT_EQ(t.rows(), 4u);
            EXPECT_EQ(fs::file_size(fs::path(dir) / "power_W.npy"), 128u + 4 * 8);
            // run 1 again: its new line counts
            RunChunk c1 = small_chunk(2, "ROOT/P1");
            ASSERT_TRUE(t.commit({ 1, "ok", 0, 42, "", { "1.5" }, "" }, &c1));
            EXPECT_EQ(t.completed(), (std::set<uint64_t>{ 0, 1, 2 }));
        }
        EXPECT_EQ(read_column<int32_t>(dir, "run"), (std::vector<int32_t>{ 0, 0, 2, 2, 1, 1 }));
        RunTable again(dir, manifest, { "x" }, { "a" }, false);
        EXPECT_EQ(again.completed().size(), 3u);
        EXPECT_EQ(again.rows(), 6u);
    }
    SPECS_TEST_FOOTER();
}

TEST(RunTableDeathTest, OtherManifestOrDirectory)
{
    SPECS_TEST_HEADER()
    {
        const auto failed = testing::ExitedWithCode(1);
        const std::string dir = table_dir("other");
        {
            RunTable t(dir, "{\"a\": 1}\n", {}, {}, false);
            RunChunk c = small_chunk(1, "ROOT/P1");
            ASSERT_TRUE(t.commit({ 0, "ok", 0, 0, "", {}, "" }, &c));
        }
        EXPECT_EXIT(RunTable(dir, "{\"a\": 2}\n", {}, {}, false), failed, "was made by another netlist");
        // --overwrite: a new, empty table
        {
            RunTable t(dir, "{\"a\": 2}\n", {}, {}, true);
            EXPECT_EQ(t.lines(), 0u);
            EXPECT_EQ(t.rows(), 0u);
        }
        EXPECT_EQ(read_column<int32_t>(dir, "run").size(), 0u);
        // a creation cut short before the manifest (written last): only table files, made a table again
        const std::string cut = table_dir("cut");
        {
            RunTable t(cut, "{}\n", {}, {}, false);
        }
        fs::remove(fs::path(cut) / "table.manifest.json");
        {
            RunTable t(cut, "{}\n", {}, {}, false);
            EXPECT_EQ(t.lines(), 0u);
        }
        EXPECT_TRUE(fs::exists(fs::path(cut) / "table.manifest.json"));
        // a directory that is not a run table is never touched
        const std::string not_table = table_dir("not_table");
        fs::create_directories(not_table);
        std::ofstream(fs::path(not_table) / "keep.txt") << "data";
        EXPECT_EXIT(RunTable(not_table, "{}\n", {}, {}, true), failed, "is not a run table");
        EXPECT_TRUE(fs::exists(fs::path(not_table) / "keep.txt"));
    }
    SPECS_TEST_FOOTER();
}

// one process per run: a failing or crashing run is recorded, the others go on, commits in run order;
// a second call resumes with the failed runs only
TEST(RunPlan, RunnerRecordsFailuresAndResumes)
{
    SPECS_TEST_HEADER()
    {
        const std::string dir = table_dir("runner");
        ParsePlan pp;
        pp.steps.push_back({ "x", { Variable(0), Variable(1), Variable(2), Variable(3), Variable(4) }, "" });
        const RunPlan plan(pp, 0);
        for (int pass = 0; pass < 2; ++pass)
        {
            RunTable table(dir, "{}\n", plan.param_names(), {}, false);
            const RunnerOutcome outcome = run_plan(plan, table, 3);
            if (outcome.child)
            {
                // the process of one run (never back into the test)
                const uint64_t run = outcome.run;
                std::cerr << "message of run " << run << std::endl;
                if (pass == 0 && run == 1)
                    _exit(1);
                if (pass == 0 && run == 2)
                    raise(SIGKILL);
                if (pass == 0 && run == 3)
                    _exit(0); // no chunk: no result
                // later runs finish first: the commits still follow the run order
                usleep(static_cast<useconds_t>((4 - run) * 20000));
                _exit(small_chunk(run, "ROOT/P1").write(RunTable::chunk_path(dir, run)) ? 0 : 3);
            }
            EXPECT_EQ(outcome.exit_code, pass == 0 ? 1 : 0);
        }
        std::istringstream lines(read_text(fs::path(dir) / "runs.csv"));
        std::vector<std::string> status;
        std::string line;
        std::getline(lines, line);
        while (std::getline(lines, line))
        {
            const auto f = RunTable::csv_split(line);
            status.push_back(f[0] + ":" + f[1] + ":" + f[2]);
            if (f[0] == "1" && f[1] == "failed")
                EXPECT_EQ(f.back(), "message of run 1");
            if (f[1] == "crashed")
                EXPECT_EQ(f.back().rfind("killed by signal 9", 0), 0u) << f.back();
            if (f[0] == "3" && f[1] == "failed")
                EXPECT_EQ(f.back(), "the run wrote no result: message of run 3");
        }
        EXPECT_EQ(status, (std::vector<std::string>{ "0:ok:0", "1:failed:1", "2:crashed:137", "3:failed:0",
                                                     "4:ok:0", "1:ok:0", "2:ok:0", "3:ok:0" }));
        EXPECT_EQ(read_column<int32_t>(dir, "run"), (std::vector<int32_t>{ 0, 0, 4, 4, 1, 1, 2, 2, 3, 3 }));
        // the logs of the runs that wrote to stderr are kept
        EXPECT_EQ(read_text(RunTable::log_path(dir, 4)), "message of run 4\n");
        EXPECT_TRUE(fs::is_empty(fs::path(dir) / "chunks"));
    }
    SPECS_TEST_FOOTER();
}
