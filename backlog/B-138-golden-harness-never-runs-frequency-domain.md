# B-138: golden harness never runs the frequency-domain mode of the `ac_*` testbenches
severity: low | category: test-gap | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 6fa8c9e
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

**Independent second check (V6, of F-A5-18): CONFIRMED.** Method: ran `-t ac_add_drop -m fd` vs default, added the case to a scratch cases.json, `record --only 'tb/ac_add_drop-fd' --repeat 3` into a baseline copy, then full `check`

Default mode prints 18 lines, `-m fd` prints 522 lines (501 sweep points), 0.25 s; the FD branch is not covered by any existing case (opt/mode-fd runs a netlist, not the testbench). The new case is deterministic (3 recordings identical; only `Total runtime` differs and is normalised), record merges into the existing manifest (`--only` path), and `check` afterwards is clean for the case. Baseline grows by ~128 kB (case dir with one freqsweep_tb.vcd). The cases.json insertion is mechanical (json.dumps(indent=1) round-trips the file byte-for-byte, so the 10-line diff is exactly the new entry).
