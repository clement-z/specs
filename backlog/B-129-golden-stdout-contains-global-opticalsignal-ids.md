# B-129: golden stdout contains global OpticalSignal IDs that are compared exactly, so any change in the number of OpticalSignal copies breaks 25 baselines
severity: med | category: test-gap | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: fixed in 23797a8, 1747633 (sprint 2026-10b P1)
location: tests/golden/run_golden.py:93-108 (`normalise_text`), :59-61 (`NUM_RE`), :201-204 (integers compared exactly), src/optical_signal.h:28,55-62,231
## Problem
`operator<<(OpticalSignal)` prints `[NNNN]` = `m_id`, a value taken from a static counter by `getNewId()`, which is called by every constructor and, notably, by the copy constructor (optical_signal.h:55-56). So the printed numbers depend on exactly how many `OpticalSignal` objects the program created/copied before. `normalise_text` does not touch them and `compare_text` treats them as integers that must match exactly. Any refactor that adds or removes a copy (`const auto&` instead of `auto`, an extra temporary, an elided move) shifts all later IDs and fails every case that prints signals with identical physical values. 955 ID tokens in 25 of the 44 stdout baselines are affected.
## Evidence
```
compare_text("\tOUT: [0015]  @ 1550 nm (1.000000 V.m^-1, ...)", "\tOUT: [0016]  @ 1550 nm (1.000000 V.m^-1, ...)")  -> difference reported
normalise_text keeps "[0015]" unchanged
grep over tests/golden/baseline/cases/*/*/stdout.txt: 955 tokens "[dddd]" in 25 cases
```
## Expected
The harness checks the value and the identity structure (which prints refer to the same signal), not the absolute counter.
## Suggested fix
In `normalise_text` (per stream), replace each `\[(\d{4,})\]` by `[#k]` where k is the rank of the ID's first appearance. Equality between prints is preserved, absolute offsets vanish. Verified: on all 44 baseline stdout files the canonical form is identical for the original and for a copy with every ID shifted by +7. Downside to accept: a change in the number of copies is no longer visible; the ID counter then deserves its own unit test if that matters. Re-record the baseline in the same commit as the harness change.
## Golden impact
stdout baselines of the 25 cases are rewritten once (IDs replaced by ranks); values unchanged.
## Related
B-126.

- Review ids: F-A5-09.
## Verification
Compared canonical forms of the baselines as above; read optical_signal.h.
