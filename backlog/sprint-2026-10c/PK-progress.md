# PK — fixes before v0.4.0 (progress)

Branch `ws/kfixes` (from `sprint/2026-10c` at 22714f2). Heartbeat: `690a2092` (PK-heartbeat, every 5 min, 48 h).
Decision: "Before v0.4.0 (2026-10-23)" in `backlog/DECISIONS-2026-10-21.md`.

| Item | Status | Commit | Notes |
|---|---|---|---|
| 0. Baseline (before) | done | — | `pixi run test gcc` 16 OK, run-table SKIP (77); strict golden 275/275. Before binaries (gcc, clang, asan-gcc) built from 22714f2 in `/tmp/PK-wt/before` |
| 1. B-189 negative/non-finite CWSRC power | done | 634c886 | new case `err/b189-cwsrc-negative-power` as predicted; strict 276/276; asan-clang 17/17 OK, no sanitizer message |
| 2. B-193 run-table test in CI and pixi | done | a8b4721 | numpy+pandas in the pixi default env (lock: additions only) and pip in CI; run-table OK in gcc and asan-clang; golden unchanged |
| 3. B-191 long netlist lines | done | 8fc95cb | REJECT removed (twin start conditions); new case `regress/b191-long-line-vlsrc`; strict 277/277; 35-netlist differential check identical; asan-clang 17/17 OK, no sanitizer message |
| 4. B-192 value files resolved like `.include` | done | dd822d7 | new case `regress/b192-value-files-next-to-netlist` + gtest (gtest case re-recorded); strict 278/278; asan-clang: pending |
| 5. Remove legacy CROW testbench | todo | | |
| Gate: clang, asan-gcc before/after; stats-compare | todo | | |
| Report | todo | | |

## Predictions
### Item 1 (B-189), written before recording
- New case `err/b189-cwsrc-negative-power`: stderr `Error (ROOT/CWSRC1): power must be >= 0 and finite (got -1 W)`,
  exit 1, no trace files. No existing case changes (no golden netlist has a negative or non-finite CWSRC power:
  the only negative power, `circuits/subckt_tests.cir`, is a VLSRC value, B-063). Counters: unchanged.
- B-063 (VLSRC negative power) not included: its fix has an open design question (`circuits/subckt_tests.cir`
  feeds `-2` W and `cir/subckt_tests`/`opt/vci` bake the NaN in; edit the example or clamp, B-135).

### Item 3 (B-191), written before recording
- Approach: flex's REJECT removed (it also came from the variable trailing contexts `/{EOL}` and `/{WS}*=`).
  The line rule records the line, gives it back (`yyless(0)`) and the line is scanned again in a twin start
  condition without that rule; the subcircuit-body REJECTs become appends; trailing contexts fixed-length.
- Existing cases: none changes (strict 276/276 checked with the new scanner before recording). Differential
  check of the before/after binaries on 35 edge netlists (CRLF, `.title` forms, continuation lines, no-break
  spaces, nested subcircuits, last line without EOL, includes, unmatched quotes): identical stdout, stderr and
  exit codes, except a 30 KB comment line (failed before, parses now).
- New case `regress/b191-long-line-vlsrc`: exit 0, VCD with 2000 value changes of `PROBE{ROOT/out}`
  (1 mW / 2 mW alternating every 10 ps), no stderr. Counters: unchanged for the existing cases.

### Item 4 (B-192), written before recording
- Existing cases: none changes. The 8 VLSRC/EVLSRC value-file references of the golden netlists (b052-*,
  b066) are working-directory paths with no file of that name next to their netlist (checked by script), and
  the param-named ones (`cir/paul_3ring_puf`, `err/b064-*`) name files that exist nowhere: same error text.
  Strict check with the change: 277/277 before recording.
- New case `regress/b192-value-files-next-to-netlist`: exit 0 (was exit 1, `Error: Cannot open the file
  "vlsrc-values.csv".`); manifest `data_files` = `tests/golden/netlists/b192-value-files/vlsrc-values.csv`,
  `.../lib/values.csv`, `.../evlsrc-values.csv` (element order); VCD with the three sources' values.
- `gtest/unit_tests`: one new test, `ParserStateDeathTest.ValueFilesResolvedAsIncludes` (re-record of that case
  only if the harness requires it for a new test name).
