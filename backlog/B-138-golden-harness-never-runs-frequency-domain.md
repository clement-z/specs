# B-138: golden harness never runs the frequency-domain mode of the `ac_*` testbenches
severity: low | category: test-gap | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: tests/golden/cases.json:315-330 (tb/ac_add_drop, tb/ac_crow), src/tb/freqsweep_tb.cpp:45-53 (usage comment), src/tb/crow_tb.cpp:21-30
## Problem
Both testbenches are documented as `-t ac_add_drop -m fd`, but cases.json only runs them in the default event-driven mode, which exercises the pulse branch (`run_1` else-branch / `run_td`). The sweep branches (`run_1` FD branch, `crow_tb::run_fd`) are never executed by the golden suite although they are the main purpose of the testbenches.
## Evidence
`specs -t ac_add_drop -m fd` takes 0.13 s (522 lines, 501 sweep points); `specs -t ac_crow -m fd` takes 14.4 s and prints 40040 lines (too heavy). `-m td` and `-m sampled-time` give the same output class as the default.
## Expected
A golden case for the FD sweep.
## Suggested fix
Add to cases.json: `{"id": "tb/ac_add_drop-fd", "args": ["-t", "ac_add_drop", "-m", "fd"], "note": "frequency sweep branch of freqsweep_tb"}` and record it. Do not add ac_crow FD (14 s, 40k lines) unless it is reduced (B-131 plus a smaller sweep).
## Golden impact
one new case (NEW until recorded).
## Related
B-137, B-141 (crow_tb sticky stream state affects only the FD output).

- Review ids: F-A5-18.
## Verification
Ran both in FD mode; the case would be deterministic (same Release binary reproduces it).
