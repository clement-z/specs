# PK — fixes before v0.4.0 (progress)

Branch `ws/kfixes` (from `sprint/2026-10c` at 22714f2). Heartbeat: `690a2092` (PK-heartbeat, every 5 min, 48 h).
Decision: "Before v0.4.0 (2026-10-23)" in `backlog/DECISIONS-2026-10-21.md`.

| Item | Status | Commit | Notes |
|---|---|---|---|
| 0. Baseline (before) | done | — | `pixi run test gcc` 16 OK, run-table SKIP (77); strict golden 275/275. Before binaries (gcc, clang, asan-gcc) built from 22714f2 in `/tmp/PK-wt/before` |
| 1. B-189 negative/non-finite CWSRC power | done | 634c886 | new case `err/b189-cwsrc-negative-power` as predicted; strict 276/276; asan-clang: see below |
| 2. B-193 run-table test in CI and pixi | todo | | |
| 3. B-191 long netlist lines | todo | | |
| 4. B-192 value files resolved like `.include` | todo | | |
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
