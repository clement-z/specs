# Accuracy suite (F-06, README.md here): SPECS against closed forms (add-drop ring, CROW transfer
# matrices) with mutation checks. Stdlib Python; about a second on a release build.
oracle_args = ['-I', files('test_oracles.py'), '--specs', specs_exe, '--repo', meson.project_source_root()]
test('oracles', find_program('python3'),
  args: oracle_args,
  depends: specs_exe,
  suite: 'oracle',
  timeout: 300,
)

# Benchmark ratios on the paper-style CROW (event-count ratios always; the user-time ratio only on
# optimised builds without sanitizers). Absolute times are not asserted: bench.py records them in a
# local history file. A few seconds on a release build.
bench_args = ['-I', files('test_bench.py'), '--specs', specs_exe]
if get_option('buildtype') in ['plain', 'debug'] or get_option('b_sanitize') != 'none'
  bench_args += ['--no-timing']
endif
test('bench-ratios', find_program('python3'),
  args: bench_args,
  depends: specs_exe,
  suite: 'bench',
  timeout: 600,
  is_parallel: false,  # the timing ratio wants a quiet machine
)
