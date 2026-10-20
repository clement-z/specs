# B-096: No tests for pyspecs although the golden baseline already contains 27 ready-made VCDs
severity: low | category: test-gap | effort: M | fix-class: needs-design | verdict: CONFIRMED
location: tests/ (no python test), pyspecs/specsparse.py
## Problem
B-076 and B-077 went unnoticed because nothing exercises `parse_vcd_dataframe`. The baseline VCDs in `tests/golden/baseline/cases/**/files/traces/*.vcd` cover FD (4 files), TD (19), probes, pdet (tb/detector) and MLPROBE-free circuits.
## Evidence
`grep -rn pyspecs tests` only matches circuit case names; `backlog/repro/a6-pyspecs-parse-vcds.py` parses all 27 files in a few seconds.
## Expected
A pytest module that parses the baseline VCDs and asserts type (FD for the four `analysis_type == b1` files), first column and shape (e.g. add_drop: 30 x 31), plus the B-078 repro netlists once built.
## Suggested fix
Add `tests/pyspecs/test_specsparse.py` (skip if pandas/pyDigitalWaveTools are missing) using the baseline VCDs as fixtures; hook into `meson test --suite python` or the golden harness. Needs a decision on test runner and CI environment.
## Golden impact
none.
## Related
B-076, B-077, B-078.

- Review ids: F-A6-21.
## Verification
Parsed all 27 baseline VCDs before/after the reference patch (23 identical TD frames, 4 FD).
