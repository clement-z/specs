# S-parameter device SPAR (F-03): fixtures, phase convention per format, delays, ports, passivity and the
# round trip of circuits/add_drop.cir through utils/sparam_export.py (see README.md). Needs only specs.
test('sparam', find_program('python3'),
  args: ['-I', files('test_sparam.py'), '--specs', specs_exe],
  suite: 'sparam',
  timeout: 600,
)
