# PL progress — B-186: MLPROBE records each `.dc` point (branch `ws/mlprobedc`)

Heartbeat: `a034057d` (PL-heartbeat, every 5 min, expires in 48 h).

| Step | Status | Commit | Notes |
| --- | --- | --- | --- |
| Baseline builds (gcc, clang, asan-gcc, asan-clang) | done | 57fffc3 | all 16 OK; before binaries in /tmp/PL-before/{gcc,clang,asan-gcc}, asan-clang in the worktree /tmp/PL-wt/before (the first asan-clang build ran while the source was being edited) |
| B-186 implementation + golden cases + docs | done, checks running | 314727a | repro output as predicted (CSV and VCD); gcc golden strict: 275 pass, 2 new |
| Report | todo | | |

## Findings before the change
- No existing golden case runs a `.dc` with an MLPROBE: the 15 cases whose netlists contain an MLPROBE all run
  a `.tran` (scan of `tests/golden/cases.json` and the netlists they include, 2026-10-24).
- `.op` (scope item 4): `runOPAnalysis()` disables PROBEs and MLPROBEs alike for the operating point (the same
  code also runs the OP before a `.tran`); a `.op` run of the repro netlist writes an empty CSV (no PROBE row
  either). Not the switch that disables MLPROBE in `.dc` (`runDCAnalysis()`): a separate question, left as is.

## Predictions (before re-recording)
- Existing golden cases: **none changes** (strict, `--rtol 0 --atol 0`), on gcc, clang, asan-gcc, asan-clang.
- `stats-compare` (gcc) before/after: **identical for every case** (no case has an MLPROBE in a `.dc`).
- New case `regress/b186-mlprobe-dc-wavelength-sweep` (the repro, 2 points: 1.548, 1.549 um; MERGER halves
  the power; WG1 10 um, negligible loss):
  - point 1 (t = 0): MLPROBE1 1.548 um 0.5 mW, 1.549 um 0, 1.55 um 1 mW;
  - point 2 (t = 1 tick): 1.548 um 0 (absent at this point: fresh steady state), 1.549 um 0.5 mW, 1.55 um 1 mW;
  - PROBE1 unchanged from today's behaviour: the swept wavelength at each point (0.5 mW at 1.548, then 1.549 um);
  - CSV: 3 MLPROBE1 rows per point (one per listed wavelength, the absent one with power 0 at its listed
    wavelength), in wavelength order, after PROBE1's row.
- New case: a power sweep of a two-channel WDM circuit (two CW sources merged, a ring filter): MLPROBE rows
  for both channels at every point, the swept channel's power proportional to the swept power, the other
  channel constant; PROBE traces the smallest wavelength (no WL sweep).
