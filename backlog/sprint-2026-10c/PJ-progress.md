# PJ — value-list source prints (progress)

Branch `ws/vlprints` (from `sprint/2026-10c` at 365d33b). Heartbeat: `6582c2bb` (PJ-heartbeat, every 5 min, 48 h).
Decision: "Follow-ups (2026-10-23)" in `backlog/DECISIONS-2026-10-21.md`; B-066.

| Item | Status | Commit | Notes |
|---|---|---|---|
| 0. Baseline (before) | done | — | `pixi run test gcc` 12/12 OK; golden strict (`--rtol 0 --atol 0 --print-slack 0`) 240/240 |
| 1. Prints behind `--vci`, stderr warning | done | baf872b | 6 gtests; the 37 predicted cases re-recorded in the same commit; proof OK (gcc, asan-clang, clang) |
| 2. Golden: 2 new cases | done | 2fa468a | `regress/b066-vlsrc-vci`, `regress/b066-evlsrc-past`; strict 242/242 |
| 3. Docs, CHANGELOG, B-066 | done | 7168885 | |
| Gate: asan-gcc before/after, `meson test` clang/asan-gcc | running | | before builds in `/tmp/PJ-wt/before` (354c5e5) |
| Report | pending | | |

## Predicted golden impact (written before the change)
stdout only, lines removed, nothing else: values, traces, stderr, exit codes, `.stats.json`, and every other file
are identical. Lines removed (computed from the baseline by `predict` regexes):
- VL/EVL emission lines `@<t>, <name> emitted: <value>` (CW lines have the form `<name> emitted: @ …` and stay);
- `<name> was enabled` of a VL/EVL source (a name that has an emission line in the same case);
- the value-file parse listing (`<t>: <P>W @<wl>nm` for VLSRC, `<t>: <V>V` for EVLSRC); no case has
  `Skipping … line` notes or `invalid time for signal emission`.

Differences from the counts in PG-REPORT (and the phase prompt):
- **"was enabled": 49 lines in 36 cases, not 77 in 41.** The other 27 lines come from photodetectors
  (`ROOT/PDET*` in `detector.cpp`), which the decision does not cover: they stay.
- **`regress/b052-evlsrc-positional` also loses its parse-time listing** (3 lines `1e-10: 1.5V` …): the EVLSource
  value-file reader is a copy of VLSource's (B-074), and the decision names both sources.
- **`opt/vci` does not change**: it runs with `--vci`, so it keeps its 5 emission lines and its "was enabled".
  So 37 cases change, not 38: 146 emission lines (VL 134 − 5 of opt/vci + EVL 17), 49 "was enabled", 6 parse lines,
  201 lines in all.

| Case | emission lines | "was enabled" | parse listing |
|---|---|---|---|
| `cir/add_drop_pyspecs` | 3 | 1 | 0 |
| `cir/subckt_tests` | 5 | 1 | 0 |
| `opt/tracefile-o` | 3 | 1 | 0 |
| `regress/b004-mode-fd-tran` | 3 | 1 | 0 |
| `regress/b006-empty-output` | 3 | 1 | 0 |
| `regress/b014-delay-rounding-warning` | 2 | 1 | 0 |
| `regress/b014-device-internal-rounds-to-zero` | 1 | 1 | 0 |
| `regress/b031-mzi-bidir` | 12 | 6 | 0 |
| `regress/b033-crossing-two-lambda-tran` | 6 | 2 | 0 |
| `regress/b049-pdet-drives-net` | 2 | 1 | 0 |
| `regress/b050-pdet-two-open-tran` | 4 | 2 | 0 |
| `regress/b052-evlsrc-positional` | 4 | 1 | 3 |
| `regress/b052-vlsrc-positional` | 4 | 1 | 3 |
| `regress/b062-mlprobe-unlisted-wavelength` | 3 | 1 | 0 |
| `regress/b153-zerotol-cli` | 6 | 2 | 0 |
| `regress/b153-zerotol-lossy-ring-tran` | 2 | 1 | 0 |
| `regress/b153-zerotol-open-loop` | 6 | 2 | 0 |
| `regress/f01-options-seed` | 4 | 1 | 0 |
| `regress/f01-options-seed-cli-priority` | 4 | 1 | 0 |
| `regress/f01-options-seed-int-literal` | 4 | 1 | 0 |
| `regress/f01-options-seed-int-max` | 4 | 1 | 0 |
| `regress/f01-options-seed-int64-max` | 4 | 1 | 0 |
| `regress/f01-options-seed-large` | 4 | 1 | 0 |
| `regress/f01-pdet-noise-default-seed` | 4 | 1 | 0 |
| `regress/f01-pdet-noise-seed` | 4 | 1 | 0 |
| `regress/f01-seed-large` | 4 | 1 | 0 |
| `regress/f01-seed-max` | 4 | 1 | 0 |
| `regress/f05-format-csv-npz` | 2 | 1 | 0 |
| `regress/f05-no-trace` | 2 | 1 | 0 |
| `regress/f05-save-tran` | 2 | 1 | 0 |
| `regress/f15-dispersive-waveguide` | 11 | 3 | 0 |
| `regress/f15-max-events-cli-priority` | 2 | 1 | 0 |
| `regress/f15-maxevents-options-tran` | 2 | 1 | 0 |
| `regress/f15-progress-quiet` | 3 | 1 | 0 |
| `regress/f15-stats-tran` | 6 | 2 | 0 |
| `regress/f15-wdm-crow-mlprobe` | 4 | 1 | 0 |
| `regress/probe-latch-tran` | 3 | 1 | 0 |

Totals: 146 + 49 + 6 = 201 lines, 37 cases.
