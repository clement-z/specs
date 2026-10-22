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
| 3 | pending events: time-only order, insertion order on ties, single `sc_event`, merge as a lookup [Q8] | **partly done**: time-only order **stopped** (beyond Q8: per-lambda values change at default tol); landed the bit-identical part: sc_event_queue replaced by own allocation-free emulation (method re-arm kept) | 67d1702; WIP `PD2-wip/q8-time-order` 935f362, `PD2-wip/single-event-thread-rearm` cd450ed | 3a gate: gcc strict 228/228, counters 227 same, clang + asan-gcc = s2, asan-clang 12/12; WDM CROW VCD/stdout/counters identical. Thread-rearm variant: golden identical but WDM CROW plain probes + counters change (process order). Bench C N=30: s3 1.465x, Q8 WIP 1.844x vs base |
| 4 | `OpticalSignal`: no virtual destructor, id behind a compile-time flag [Q2] | done | a935b7e | `-Dsignal_ids` (off); 127 cases re-recorded, every file identical up to the prefix (idcheck.py); counters 227 same; clang/asan-gcc same up to prefix; ids build = former listings; asan-clang 12/12 |
| 5 | bench after each step, report | done (PD2-REPORT.md) | | C-notrace N=30 (5 rounds, interleaved): s1 1.163x, s2 1.243x, s3 1.474x, s4 1.514x; A N=30 1.461x, D-wdm N=10 nwl=4 1.474x (s4 vs base) |

## Maintainer messages (2026-10-23, during the report)
Found: `--no-coalescing` on a 4-wavelength 3-ring CROW exhausts memory (bad_alloc at 3 GB within 0.5 ns; base build
too). Maintainer: "This blow up seems like a very logical finding. You will have an exponentially growing list of
signals" / "Expected behavior. And it has only physical explanation if you work with deltas, not with fields. I
think we can drop the deltaE idea too. ... Message the orchestrator for his guidance". Sent to the orchestrator
(9a0fb35) with the PD2 status; nothing changed for it. Coordinator guidance: do not remove no-coalescing / m_use_deltas in PD2 (separate item); write the report with the
Q8 evidence and options and the proposal verbatim; D2 merged without the Q8 reordering.
