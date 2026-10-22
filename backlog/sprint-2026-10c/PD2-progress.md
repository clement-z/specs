# PD2 progress (F-15 part 2: bit-identical engine refactors)

Branch `ws/engine2` (from `sprint/2026-10c` 56e5808). Heartbeat: `8e2ab409` (PD2-heartbeat).

Baseline before any change (pixi gcc 15 release, `build-pixi/gcc`): `pixi run test gcc` 12/12 OK;
`pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0` 227 passed, 0 failed/new/missing.
Event counters of the base: `stats-record` 226 cases, 134 with statistics (`/tmp/pd2/stats/base-gcc`).
Base binary for the bench: `/tmp/pd2/bin/base/specs`. Base recordings of clang, asan-gcc, asan-clang:
`/tmp/pd2/rec/base-<toolchain>` (helper `/tmp/pd2/scripts/record_builds.sh`; per-step gate `/tmp/pd2/scripts/gate.sh <label> [ref]`, bench
`/tmp/pd2/scripts/bench.py`). References after item 2 (with the new case): `/tmp/pd2/stats/s2b-gcc`, `/tmp/pd2/rec/s2b-*`;
binaries per step in `/tmp/pd2/bin/<step>/specs`.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 1 | tolerance gate on squared norms (+ unit tests: last = 0, NaN, inf) | done | e50d693 | guarded: abs() decides near thresholds/out of range; gcc strict 227/227, counters 226 same, clang + asan-gcc = base, asan-clang 12/12; gtest re-recorded; bench C N=30 1.159x vs base |
| 2 | per-wavelength S-parameter caches (waveguide, coupler, phase shifter) | done | 23bae2f | gcc strict 228/228 (new case regress/f15-dispersive-waveguide recorded with base build), counters same, clang + asan-gcc = base, asan-clang 12/12; ad hoc dispersive CROW/add-drop identical; bench C N=30 1.251x vs base (s1 1.164x) |
| 3 | pending events: time-only order, insertion order on ties, single `sc_event`, merge as a lookup [Q8] | **stopped (time-only order)**; bit-identical part in progress | WIP `PD2-wip/q8-time-order` 935f362 | golden: only the 4 predicted cases + pending_mean of b001 merger; BUT 4-lambda CROW (crow_gen 3 --nwl 4, MLPROBE on all nets): per-lambda values differ up to 8.4e-4 rel at default tol (identical at abstol=reltol=0): gate vs intermediate same-time values. Beyond Q8 prediction -> stopped, for the maintainer. Next: land only single sc_event + no sc_event_queue allocations with the |E| heap order kept (3a), if bit-identical incl. WDM CROW |
| 4 | `OpticalSignal`: no virtual destructor, id behind a compile-time flag [Q2] | todo | | predicted re-record: `[NNNN]` prefix of listings |
| 5 | bench after each step, report | todo | | |
