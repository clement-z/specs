# GoogleTest unit tests. SystemC can only elaborate once per process, so every test body forks
# (SPECS_TEST_HEADER/FOOTER in utils.h). No gtest_main: SystemC's main() calls sc_main in
# main.cpp, which runs the tests. (The gtest wrap ignores main: true, so asking for gtest_main
# would give a different entry point depending on where gtest comes from.)
gtest_dep = dependency('gtest', main: false, required: get_option('tests'),
                       allow_fallback: true)

unit_tests_exe = disabler()
if gtest_dep.found()
  unit_tests_exe = executable('unit_tests',
    files(
      'devices/crossing.cpp',
      'devices/detector.cpp',
      'devices/models.cpp',
      'devices/pcm.cpp',
      'devices/probe.cpp',
      'devices/process_style.cpp',
      'devices/waveguide.cpp',
      'fixed_point.cpp',
      'general_utils.cpp',
      'integer_literals.cpp',
      'main.cpp',
      'optical_output_port.cpp',
      'parser_state.cpp',
      'rng.cpp',
      'run_manifest.cpp',
      'run_stats.cpp',
      'specs_config.cpp',
      'sysc_utils.cpp',
      'trace_table.cpp',
      'tran_start.cpp',
      'utils/sha256.cpp',
      'utils/transmission_cache.cpp',
    ),
    cpp_args: ['-pedantic'],
    # parser_dep: the netlist lexer and parser (integer_literals.cpp)
    dependencies: [gtest_dep, systemc_dep, parser_dep],
    link_with: spx_lib,
    implicit_include_directories: false,
  )

  # Kept serial (conservative): the golden suite also runs unit_tests. Trace files are per-process
  # since B-142 (testing::TempDir() + specs_test_<pid>), so the old shared /tmp path is no longer a reason.
  test('unit', unit_tests_exe,
    protocol: 'gtest',
    suite: 'unit',
    is_parallel: false,
  )
endif
