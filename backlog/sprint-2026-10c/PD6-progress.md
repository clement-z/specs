# PD6 progress (PROBE always traces its wavelength)

Branch `ws/probewl` (from `sprint/2026-10c` a42e0f2). Heartbeat: `bb7f3ca7` (PD6-heartbeat).
Scratch (not in the repo): `/tmp/pd6` (logs, recordings, counters, scripts).
Decision: "PROBE wavelength (2026-10-23)" in `backlog/DECISIONS-2026-10-21.md`; prompt
`discover/orchestration/2026-10c/PD6-probe-wl.md`.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 0 | references (base a42e0f2) and prediction | done | | gcc `meson test` 12/12, strict golden 234/234; clang/asan-gcc recordings `/tmp/pd6/rec-before-*`; counters `/tmp/pd6/stats-before`; asan-clang test |
| 1 | PROBE and `PROBE{<net>}` always trace `wavelength`; false 4th argument / `wl=0` ignored, one warning per run | done | 769a17f | as predicted: re-recorded `regress/b153-zerotol-lossy-ring-tran`, `regress/probe-latch-tran` (+ wavelength signal: 1.55e-6 from 0; PROBE5 = PROBE4, 1.54e-6 from 2 ps; + warning first on stderr; netlist hash); new case `regress/probe-wl-ignored`, gtest `Probe.NetlistProbeAlwaysTracesTheWavelength`; strict gcc 235/235. Warning printed in `ParseTree::build_circuit()` (names collected in `ParseTreeCreationHelper`, no global) |
| 2 | docs (`doc/netlist.md`, element table, CHANGELOG), tests | done | 30a4a77 | tests are in 769a17f; cheatsheet is a PDF without source (not updated, as in PD5) |
| 3 | checks, report | done (PD6-REPORT.md) | | gcc test 12/12, strict golden 235/235; counters 233 same + 1 new; clang/asan-gcc before/after: only the 2 predicted cases differ, PD6 cases strict vs baseline; asan-clang 12/12 clean; heartbeat deleted |

## Predicted golden impact (written before any code change)

Scan (all tracked files: `tests/golden/netlists`, `circuits`, `circuits/legacy`, `backlog/repro`, inline
arguments in `tests/golden/cases.json`) for PROBE lines with a 4th positional argument or a `wl`/`wavelength`
keyword:
- `tests/golden/netlists/b153-zerotol-lossy-ring-tran.cir`: `probe1 out 0 1 0 0` (false 4th argument);
- `tests/golden/netlists/probe-latch-tran.cir`: `probe5 c wl=0`;
- `tests/golden/netlists/b148-probe-args.cir`: `probe1 a 1 1 1 1 1` (5 arguments: the existing "Too many
  input arguments" error, exit 1 before any warning; unchanged);
- `backlog/repro/a6-pyspecs-*.cir` (`wl=0`, `wl=1`): not golden cases (pyspecs repro).
No case passes PROBE arguments on the command line. The automatic probes already trace the wavelength
(traceall: all four quantities; `.save <net>`: all four), and `.save QTY(net)` is not changed.

Prediction:
- `regress/b153-zerotol-lossy-ring-tran`: VCD gains `ROOT/PROBE1.wavelength` (1.55e-6 from t = 0, its
  only value change); stderr gains the warning line ("... wl=0 ... ignored for ROOT/PROBE1"), printed at
  elaboration (`ParseTree::build_circuit()`), so before the zero-tolerance warning (`verifyConfig()`, from
  `runAnalysis()`), which stays the last line.
- `regress/probe-latch-tran`: VCD gains `ROOT/PROBE5.wavelength` (1.54e-6 from 2 ps, as `ROOT/PROBE4`);
  stderr gains the warning line for `ROOT/PROBE5`. CSV unchanged (the table rows always hold the wavelength).
- Both: stdout unchanged (the parse listing prints the arguments as written). The comments of the two
  netlists say what the probe traces; updating them changes the netlist hash in the run manifest
  (`traces/*.manifest.json`) of these two cases, nothing else.
- New golden case for the warning (positional and keyword forms, a true value, several probes in name
  order); gtest list re-recorded if unit tests are added.
- Everything else strictly bit-identical, event counters identical (the wavelength signal is a VCD trace,
  not an optical event).
