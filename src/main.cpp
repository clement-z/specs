#include <ctime>
#include <iomanip>
#include <sstream>
#include <chrono>

#include <optical_signal.h>
#include <time_monitor.h>
#include <tb/alltestbenches.h>
#include <utils/strutils.h>

#include <systemc.h>

#include <args.hxx>

#include <specs.h>
#include "optical_output_port.h"
#include "parser/parse_tree.h"

class OpticalOutputPort;

using namespace std;
using namespace std::chrono;
using namespace literals;

#include "../build/parser/parser.tab.h"
#include "../build/parser/parser.yy.h"
extern int yydebug;

int do_list_tests()
{
    cout << "Valid testbench identifiers:" << endl;
    if (tb_map.empty())
        cout << "\t # No test available #" << endl;
    for (const auto &test_pair : tb_map) {
        cout << "\t" << test_pair.first << endl;
    }
    return 0;
}

int do_test(const string &testname)
{
    // Try to find the test specified by testname from the tb_mab
    auto it = tb_map.find(testname);
    if (it == tb_map.end()) {
        // Not found
        cerr << "Test not found: " << testname << endl;
        return 1;
    }

    // Call the testbench function associated with testname
    auto start = high_resolution_clock::now();
    it->second();
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    std::cout << "Total runtime: " << duration.count()/1000.0 << " ms" << std::endl;
    return 0;
}

int do_circuit(const string &filename, bool is_dry_run = false, const string& json_filename = "")
{
    // Check whether the file exists
    FILE *f;
    if (filename == "-"s) {
        f = NULL;
    } else if (!(f = fopen(filename.c_str(), "r"))) {
        cerr << "Error: File not found \"" << filename << "\"" << endl;
        return 1;
    }

    cout << "╔═══════════════════╗" << endl;
    cout << "║  PARSING CIRCUIT  ║" << endl;
    cout << "╚═══════════════════╝" << endl;

    yyscan_t scanner;
    YY_BUFFER_STATE buf;

    yylex_init(&scanner);
    buf = yy_create_buffer(f, YY_BUF_SIZE, scanner);
    yy_switch_to_buffer(buf, scanner);

    ParseTree pt("ROOT");
    int parsing_result = yyparse(scanner, &pt);

    //yy_delete_buffer(buf, scanner);
    yylex_destroy(scanner);

    // Close the file
    if (f)
        fclose(f);

    // Return if unsuccessful
    if (parsing_result != 0) {
        return parsing_result;
    }

    pt.print();

    cout << "╔══════════════════════╗" << endl;
    cout << "║   BUILDING CIRCUIT   ║" << endl;
    cout << "╚══════════════════════╝" << endl;

    pt.build_circuit();

    if (!json_filename.empty())
    {
        ofstream outfile;
        outfile.open(json_filename, ios::out | ios::trunc);
        outfile << pt.to_json();
        outfile.close();
        cout << "Exported flattened circuit as JSON > " << json_filename << endl;
    }

    if (!is_dry_run)
    {
        cout << "╔══════════════════════╗" << endl;
        cout << "║      SIMULATION      ║" << endl;
        cout << "╚══════════════════════╝" << endl;

        specsGlobalConfig.runAnalysis();
    }
    else
    {
        cout << "╔══════════════════════╗" << endl;
        cout << "║  SIMULATION SKIPPED  ║" << endl;
        cout << "╚══════════════════════╝" << endl;
    }

    cout << "╔═══════════════════╗" << endl;
    cout << "║        DONE       ║" << endl;
    cout << "╚═══════════════════╝" << endl;

    return 0;
}

int raphael_main() { cout<< "Hello world" << endl; return 0; }

int tests_module_registry_access();

int sc_main(int argc, char *argv[])
{
    // Define command line interface using args
    args::ArgumentParser parser("The Scalable Photonic Event-driven Circuit Simulator.", "");
    args::ValueFlag<string> file(
        parser, "circuit", "Simulate from circuit file", { 'f', "file" });
    args::ValueFlag<int> set_timescale(parser,
                          "set_timescale",
                          "Set the engine timescale (as log10(timestep/1s))"
                          "(0: seconds, -3: ms, -6:µs, ..., -15: fs)",
                          { 'R', "resolution" });
    args::ValueFlag<string> set_simulator_mode(parser,
                          "set_simulator_mode",
                          "Set simulator mode. Possible values:\n"
                          " - time-domain, event-driven, td (default)\n"
                          " - sampled-time\n"
                          " - frequency-domain, fd",
                          { 'm', "mode"});
    args::Flag run_manual_test(parser,
                          "run_manual_test",
                          "Run manual test function",
                          { 'T', "run-manual-test" });
#if YYDEBUG
    args::Flag debug_netlist_parser(parser,
                          "debug_netlist_parser",
                          "Run with netlist parser debug information",
                          { "debug-parser" });
#endif
    args::ValueFlag<string> set_tracefile(parser,
                          "set_tracefile",
                          "Set the default trace file",
                          { 'o', "output" });
    args::ValueFlag<string> export_json(parser,
                          "export_json",
                          "Export json of completed circuit to file",
                          { "json" });
    args::Flag set_dry_run(parser,
                          "dry_run",
                          "Do not run simulation directives",
                          { 'n', "dryrun" });
    // args::ValueFlag<double> set_reltol(parser,
    //                       "set_reltol",
    //                       "Set the value of the rel_tol parameter",
    //                       { "reltol" });
    args::ValueFlag<double> set_reltol(parser,
                          "set_reltol",
                          "Set the value of the relative tolerance parameter for field",
                          { "reltol" });
    args::ValueFlag<double> set_abstol(parser,
                          "set_abstol",
                          "Set the value of the absolute tolerance parameter for field",
                          { "abstol" });

    args::ValueFlag<size_t> set_nrings_crow(parser,
                          "set_nrings_crow",
                          "temporary",
                          { "nrings_crow" });

    args::Flag set_verbose_component_initialization(parser,
                          "set_verbose_component_initialization",
                          "Print components detail before starting simulation",
                          { "vci", "verbose_ci" });

    args::Flag list_tests(parser,
                          "list_tests",
                          "List available testbenches",
                          { 'p', "list-testbenches" });
    args::Flag add_time_monitor(parser,
                          "add_time_monitor",
                          "Monitor simulation status",
                          { "mon" });
    args::ValueFlag<string> test(
        parser,
        "test_name",
        "Run the specified testbench (use -p to list available testbenches)",
        { 't', "testbench" });
    args::HelpFlag help(parser, "help", "Display this help menu", { 'h', "help" });
    args::CompletionFlag completion(parser, { "complete" });
    try {
        parser.ParseCLI(argc, argv);
    } catch (const args::Completion &e) {
        cout << e.what();
        return 0;
    } catch (const args::Help &) {
        cout << parser;
        return 0;
    } catch (const args::ParseError &e) {
        cerr << e.what() << endl;
        cerr << parser;
        return 1;
    } catch (const args::ValidationError &e) {
        cerr << e.what() << endl;
        cerr << parser;
        return 1;
    }

    if (set_simulator_mode) {
        const string &s = set_simulator_mode.Get();
        if (strutils::iequals(s, "event-driven") || strutils::iequals(s, "time-domain") || strutils::iequals(s, "td"))
        {
            cout << "Optical ports working in event-driven mode by default" << endl;
            specsGlobalConfig.simulation_mode = OpticalOutputPortMode::EVENT_DRIVEN;
        }
        else if (strutils::iequals(s, "sampled-time"))
        {
            cout << "Optical ports working in sampled-time mode by default" << endl;
            specsGlobalConfig.simulation_mode = OpticalOutputPortMode::SAMPLED_TIME;
        }
        else if (strutils::iequals(s, "frequency-domain") || strutils::iequals(s, "fd"))
        {
            cout << "Simulation mode set to frequency domain" << endl;
            specsGlobalConfig.simulation_mode = OpticalOutputPortMode::FREQUENCY_DOMAIN;
        }
        else
        {
            cerr << "Unknown mode: '" << s << "'" << endl;
            return 1;
        }
    }
    if (set_reltol) {
        if (set_reltol.Get() > 0) {
            specsGlobalConfig.default_reltol = set_reltol.Get();
        } else {
            cerr << "Invalid abs_tol_phase value" << endl;
            return 1;
        }
    }
    if (set_abstol) {
        if (set_abstol.Get() > 0) {
            specsGlobalConfig.default_abstol = set_abstol.Get();
        } else {
            cerr << "Invalid abs_tol_power value" << endl;
            return 1;
        }
    }
    if (set_verbose_component_initialization) {
        specsGlobalConfig.verbose_component_initialization = set_verbose_component_initialization.Get();
    }
    if (set_nrings_crow) {
        #if BUILD_TB == 1
        //nrings_crow = set_nrings_crow.Get();
        #endif
    }
    if (set_timescale) {
        cout << set_timescale.Get() << endl;
        specsGlobalConfig.engine_timescale = SPECSConfig::EngineTimescale(set_timescale.Get());
    }
    if (run_manual_test) {
        return 0;
    }
#if YYDEBUG
    if (debug_netlist_parser) {
        yydebug = 1;
    }
#endif
    if (list_tests) {
        return do_list_tests();
    }
    if (set_tracefile) {
        cout << "Using trace file: " << set_tracefile.Get() << endl;
        // TODO: validate filename
        specsGlobalConfig.trace_filename = set_tracefile.Get();
    }

    shared_ptr<TimeMonitor> tm;
    if (add_time_monitor) {
        if (specsGlobalConfig.simulation_mode == OpticalOutputPortMode::FREQUENCY_DOMAIN)
            tm = make_shared<TimeMonitor>("TM", 0, 0.5);
        else
            tm = make_shared<TimeMonitor>("TM", 1e-14, 0.5);
    }

    if (file) {
        return do_circuit(file.Get(), set_dry_run.Get(), export_json.Get());
    }
    if (set_dry_run)
    {
        return 0;
    }
    if (test) {
        return do_test(test.Get());
    }

    // Print help if nothing else was done
    cout << parser;
    return 1;
}
