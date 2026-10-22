# PD1 progress (F-15 part 1: `--stats`, ablation switches, `--progress`, counter tool)

Branch `ws/stats` (from `sprint/2026-10c` 4f177fb). Heartbeat: `1c34605e` (PD1-heartbeat).

Baseline before any change: `pixi run test gcc` 12/12 OK; `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`
212 passed, 0 failed/new/missing.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 1 | per-port counters + `--stats[=summary\|ports\|devices]`, `--stats-file`, `--print-stats` | done | ec3e0f3 | 212 old cases bit-identical; cli/help, cli/no-args, gtest re-recorded; 5 new cases; asan-clang 12/12 OK |
| 2 | ablation switches: no coalescing (FIFO ties), event cap (`.options` + CLI, `doc/numerics.md`) | done | ec3e0f3 (engine), 51db5ea | 9 new cases, no existing case changed |
| 3 | `--progress` (stage callback, stderr) | done | ec3e0f3 (code, gtest), 51db5ea (golden cases) | |
| 4 | counter collection tool for D2/D3 (run_golden.py) + test | done | a53004c | `stats-record`/`stats-compare`; --stats for -t; dangling-net SIGSEGV fixed; gcc twice: 225 same; asan-clang vs gcc: 225 same |
| 5 | CROW bench, stats off: ratio before/after | in progress | | driver /tmp/pd1-scripts/bench_ab.py; base builds clang/asan-gcc in /tmp/PD1-wt/base for the before/after checks |
| 6 | report | todo | | |

Base worktree for before/after comparisons: `/tmp/PD1-wt/base` (branch `PD1-wip/base` = 4f177fb), built with pixi gcc.
