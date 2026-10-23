# PF06 progress (accuracy suite F-06 + activity map F-31, branch `ws/accuracy`)

Heartbeat: `fdb512fe` (PF06-heartbeat, every 5 min, 48 h). Delete after the report is committed.

Baseline before any change (cb3a5c3): `pixi run test gcc` 12/12 OK; golden strict 245/245;
`pixi run test asan-clang` 12/12 OK.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 0 | graphviz in pixi's `py` feature | done | (this commit) | `pixi lock`: additions only (py environment) |
| 1 | F-06: bench scripts moved to `tests/bench/`, generator (paper CROW, crow_tb preset) | todo | | |
| 2 | F-06: analytic oracles (add-drop, CROW transfer matrix) as a Meson suite | todo | | |
| 3 | F-06: bench with ratio assertions + local history | todo | | |
| 4 | F-31: `utils/activity_map.py` + test + docs | todo | | Sonnet subagent |
| 5 | Docs, CHANGELOG, report | todo | | |
