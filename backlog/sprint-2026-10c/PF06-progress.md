# PF06 progress (accuracy suite F-06 + activity map F-31, branch `ws/accuracy`)

Heartbeat: `fdb512fe` (PF06-heartbeat, every 5 min, 48 h). Delete after the report is committed.

Baseline before any change (cb3a5c3): `pixi run test gcc` 12/12 OK; golden strict 245/245;
`pixi run test asan-clang` 12/12 OK.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 0 | graphviz in pixi's `py` feature | done | af2324b | `pixi lock`: additions only (py environment) |
| 1 | F-06: bench scripts moved to `tests/bench/`, generator (paper CROW, crow_tb preset) | done | 095d46c | same netlists as the scout generator for the same options |
| 2 | F-06: analytic oracles (add-drop, CROW transfer matrix) as a Meson suite | done | 7d31851 | errors <= 7.2e-7 vs tol 1e-5; mutants miss by >= 0.15; injected C++ bugs (no dispersion slope; k read as kp) fail the suite (scratch build, not committed); crow_tb preset == ac_crow (same times, dP <= 7e-13 W) |
| 3 | F-06: bench with ratio assertions + local history | done | 480fbc9 | |
| 4 | F-31: `utils/activity_map.py` + test + docs | done | dc2933c, d032b2a | Sonnet subagent, reviewed; PNG looked at |
| 5 | Docs, CHANGELOG, report | done | eab8c08, report commit | final: gcc 15/15, golden 245/245 strict, asan-clang 15/15 |
