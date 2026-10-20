# GoogleTest unit tests. SystemC can only elaborate once per process, so every test body forks
# (SPECS_TEST_HEADER/FOOTER in utils.h). gtest's main() replaces SystemC's: sc_main in main.cpp
# is not used.
gtest_dep = dependency('gtest', main: true, required: get_option('tests'),
                       allow_fallback: true)

unit_tests_exe = disabler()
if gtest_dep.found()
  unit_tests_exe = executable('unit_tests',
    files(
      'devices/waveguide.cpp',
      'main.cpp',
    ),
    cpp_args: ['-pedantic'],
    dependencies: [gtest_dep, systemc_dep],
    link_with: spx_lib,
    implicit_include_directories: false,
  )

  # The tests write the fixed path /tmp/specs_test2.vcd, so they must not run concurrently
  # (with each other or with the golden suite, which also runs unit_tests).
  test('unit', unit_tests_exe,
    protocol: 'gtest',
    suite: 'unit',
    is_parallel: false,
  )
endif
