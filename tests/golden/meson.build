# Golden-output regression check against the committed baseline (see README.md).
# The tb/* cases need the legacy testbenches; without unit_tests the gtest case is SKIPPED.
if not get_option('old_tb')
  warning('golden: not registered, the tb/* cases need -Dold_tb=true')
  subdir_done()
endif

golden_args = ['-I', files('run_golden.py'), 'check',
               '--specs', specs_exe,
               '--baseline', meson.current_source_dir() / 'baseline',
               '-j', '4']
if is_disabler(unit_tests_exe)
  golden_depends = []
else
  golden_args += ['--unit-tests', unit_tests_exe]
  golden_depends = [unit_tests_exe]
endif

test('golden', find_program('python3'),
  args: golden_args,
  depends: golden_depends,
  suite: 'golden',
  timeout: 600,
  is_parallel: false,  # also runs unit_tests (fixed /tmp path)
)
