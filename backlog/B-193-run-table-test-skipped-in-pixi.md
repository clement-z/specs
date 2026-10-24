# B-193: The run-table test is always skipped in pixi builds
severity: low | category: test-gap | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in a8b4721 (sprint 2026-10c PK)
location: tests/tools/meson.build, tests/tools/test_run_table.py, pixi.toml

## Problem
Found in phases PF02 and PF03b: `tools - specs:run-table` exits 77 (skipped) in `pixi run test`: the test needs
numpy and pandas, which the default pixi environment (the Python Meson finds) does not have; they are in the `py`
environment only. The pyspecs run-table reader is therefore not tested by `meson test` or CI.

## Expected
The test runs in the pixi builds and in CI.

## Suggested fix
Run it with the `py` environment's Python when available, or add numpy and pandas to the default environment's
test dependencies; keep the skip for plain builds without them.

## Golden impact
None.
