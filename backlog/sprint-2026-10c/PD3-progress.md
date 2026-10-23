# PD3 progress (F-15 part 3: `SC_METHOD` conversion)

Branch `ws/methods` (from `sprint/2026-10c` 39f1bf9). Heartbeat: `acf69ae1` (PD3-heartbeat).
Scratch (not in the repo): `/tmp/pd3` (binaries per step, recordings, scripts).

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 0 | references: base build (39f1bf9) strict golden, event counters, clang/asan-gcc recordings; D1 tip (56e5808) build for the bench | done | | base `pixi run test gcc` 12/12, strict golden 228/228; stats `/tmp/pd3/stats/base`; recordings `/tmp/pd3/rec/base-{clang,asan-gcc}` (clean worktree `/tmp/pd3-wt/base`); binaries `/tmp/pd3/bin/{base,d1tip,...}` |
| 1+2 | `OpticalOutputPort::on_data_ready` and the stateless device reactions → `SC_METHOD` (one commit: the port alone changes tb/lambda and f15-stats-file-print) | done | bd92622 | strict golden 228/228 (gtest re-recorded: list only), counters 227 same; `fire_pending()` merged into `on_data_ready()` (s3 = s2 on golden, counters, extra CROWs); `--vci` order kept by a one-shot thread. Extra 4-λ CROWs (3 and 10 rings, MLPROBE + PROBE on every net): MLPROBE identical, plain probes of multi-λ nets differ (24/328, 73/712 signals), counters < 0.01 %; cause: SystemC triggers delta events in reverse notification order, so device methods now run before the port re-arm (was: re-arm method pass first) → equal-time tie order of timed events. Bench C-notrace N=30 (3 rounds): 2.51x vs D1 tip |
| 2b | golden case `regress/f15-wdm-crow-mlprobe` (MLPROBE of a 4-λ CROW, recorded with the base build) | done | e7afa5b, e98f79d | fails on the Q8 WIP (935f362), passes on base and s3 (gcc, clang, asan-gcc, strict); its counters: only pending_mean/max change (VLSource 1.75 → 1, COUPLER1) |
| 3 | `AGENTS.md` process style, docs, CHANGELOG | done (CHANGELOG pending the bench figure) | c1ec317 | clang release and asan-gcc: 227 pre-existing cases identical to base recordings (strict) |
| 4 | bench (`C-notrace N=30` vs D1 tip 56e5808), gate, report | todo | | |
