# Tests of the Python tools in utils/. The tool test needs only python3 (the render part is skipped when
# numpy, matplotlib or dot are missing).
test('activity-map', find_program('python3'),
  args: ['-I', files('test_activity_map.py'), '--specs', specs_exe, '--repo', meson.project_source_root()],
  suite: 'tools',
  depends: specs_exe,
)

# pyspecs/runtable.py (F-02): reads the run tables of two .step/.mc plans; skipped (exit 77) without numpy
# and pandas. The pixi default environment and the CI jobs that run meson test have them (B-193).
test('run-table', find_program('python3'),
  args: ['-I', files('test_run_table.py'), '--specs', specs_exe, '--repo', meson.project_source_root()],
  suite: 'tools',
  depends: specs_exe,
)
