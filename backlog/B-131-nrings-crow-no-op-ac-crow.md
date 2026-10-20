# B-131: `--nrings_crow` is a no-op; `ac_crow` always simulates 3 rings
severity: low | category: cli | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 449d63c
location: src/main.cpp:244-247, src/main.cpp:341-345 (assignment commented out), src/tb/crow_tb.cpp:33,171, src/tb/crow_tb.h:30
## Problem
The option is parsed but its only effect is commented out (`//nrings_crow = set_nrings_crow.Get();`). `crow_tb_run()` constructs `CROW("crow", nrings_crow)` with the global initialised to 3. The help text is just "temporary". The testbench also does not use the ECOC paper's benchmark parameters: the paper uses N up to 512, 1 mW pulses at 500 MHz, 10 pulses, waveguides (33.2 +- 5.0) um from a normal distribution, power cross-coupling 0.15; `crow_tb` uses 1 W, 4 pulses, a fixed 775.775 um ring, coupling_through 0.8 (cross 0.2), neff 1, ng 2.
## Evidence
```
$ specs -t ac_crow > a.txt;  specs -t ac_crow --nrings_crow 7 > b.txt        -> identical stdout, byte-identical VCD
```
With the line enabled (scratch build): N=1,2,3,5,7 print 1,2,3,5,7 `wg_top_*` names and produce different traces (pthrough: 1028 records for N=3, 2180 for N=5); N=64 runs in 0.5 s. Without validation `--nrings_crow -1` is parsed as 18446744073709551615 and the program prints ring names forever; `--nrings_crow 0` passes silently in Release (the `assert(N > 0)` in CROW::init is compiled out).
## Expected
`--nrings_crow N` sets the number of rings; invalid N is rejected.
## Suggested fix
src/main.cpp:341-345:
```cpp
if (set_nrings_crow) {
    #if BUILD_TB == 1
    if (set_nrings_crow.Get() < 1 || set_nrings_crow.Get() > 1024)
    {
        cerr << "Invalid --nrings_crow value (expected 1..1024)" << endl;
        return 1;
    }
    nrings_crow = set_nrings_crow.Get();
    #endif
}
```
and change the help text "temporary" to "Number of rings of the CROW testbench (-t ac_crow), default 3". Do not touch the paper-parameter mismatch here (a documentation note or a separate benchmark netlist generator is a design decision).

Correction from the second check: **leave the help string ("temporary") unchanged** in this issue; then the golden impact really is none. Changing the help text fails `cli/help` and `cli/no-args` (the string is in their stdout baseline); if a maintainer wants the new text, re-record those two cases in the same commit (`run_golden.py record ... --only '^cli/(help|no-args)$'`). The help-text rewording belongs to B-025 anyway.
## Golden impact
None if the help text is left unchanged (verified: `--nrings_crow 3` output equals today's default; 0, -1 and 1025 are rejected with exit 1).
## Related
B-141 (crow_tb dead code).

- Discovery seeds: S5.08 (see `backlog/README.md` for seed numbering).
- Review ids: F-A5-11.
## Verification
Built the one-line change in /tmp/ws-review-A5/repo and ran the matrix above. Upper bound 1024 is arbitrary (paper max is 512); safe because the flag was inert before.

**Independent second check (V6, of F-A5-11): ADJUST.** Method: applied the suggested src/main.cpp change plus help-text change on the scratch copy, rebuilt specs, ran the --nrings_crow matrix and the golden check

Functionally correct: N=1/3/7 give 1/3/7 `wg_top_*` names (old binary: `--nrings_crow 7` output identical to default); `--nrings_crow 3` output equals the old default; 0, -1 (parses as 2^64-1), 1025 are rejected with exit 1 and the message; 1024 runs (35 s in default td mode, under the 60 s limit); the BUILD_TB macro is defined in this build, so the `#if` branch is active. The upper bound 1024 is an arbitrary but harmless constant (a bound is needed because `-1` wraps to SIZE_MAX). WRONG CLAIM: "Golden impact: none". The help text is part of the golden stdout: `cli/help` and `cli/no-args` FAIL (`--nrings_crow=[set_nrings_crow]   temporary` is in tests/golden/baseline/cases/cli/{help,no-args}/stdout.txt:28, and the longer text wraps and shifts the following lines; golden check: 42 passed, 2 failed). Fix the finding text to either (a) leave the help string "temporary" untouched, then golden impact is really none, or (b) change the help text AND re-record those two cases in the same commit (`run_golden.py record --specs ... --out tests/golden/baseline --only '^cli/(help|no-args)$' --note "B-xxx help text for --nrings_crow"`; `record --only` merges into the existing baseline). Everything else in the finding stands.

**Orchestrator:** Help-text change removed from the easy fix (it fails cli/help and cli/no-args).
