# Accuracy suite (F-06, README.md here): SPECS against closed forms (add-drop ring, CROW transfer
# matrices) with mutation checks, and the CROW generator against the legacy ac_crow testbench.
# Stdlib Python; about a second on a release build.
oracle_args = ['-I', files('test_oracles.py'), '--specs', specs_exe, '--repo', meson.project_source_root()]
if get_option('old_tb')
  oracle_args += ['--legacy-tb']
endif
test('oracles', find_program('python3'),
  args: oracle_args,
  depends: specs_exe,
  suite: 'oracle',
  timeout: 300,
)
