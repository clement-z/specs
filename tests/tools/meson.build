# Tests of the Python tools in utils/. The tool test needs only python3 (the render part is skipped when
# numpy, matplotlib or dot are missing).
test('activity-map', find_program('python3'),
  args: ['-I', files('test_activity_map.py'), '--specs', specs_exe, '--repo', meson.project_source_root()],
  suite: 'tools',
  depends: specs_exe,
)
