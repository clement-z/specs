# PD1 progress (F-15 part 1: `--stats`, ablation switches, `--progress`, counter tool)

Branch `ws/stats` (from `sprint/2026-10c` 4f177fb). Heartbeat: `1c34605e` (PD1-heartbeat).

Baseline before any change: `pixi run test gcc` 12/12 OK; `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`
212 passed, 0 failed/new/missing.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 1 | per-port counters + `--stats[=summary\|ports\|devices]`, `--stats-file`, `--print-stats` | in progress | | |
| 2 | ablation switches: no coalescing (FIFO ties), event cap (`.options` + CLI, `doc/numerics.md`) | todo | | |
| 3 | `--progress` (stage callback, stderr) | todo | | |
| 4 | counter collection tool for D2/D3 (run_golden.py) + test | todo | | |
| 5 | CROW bench, stats off: ratio before/after | todo | | |
| 6 | report | todo | | |
