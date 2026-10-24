# PN progress: equal-time order by wavelength, then a per-time-step queue (branch `ws/timeorder`)

Heartbeat: `8b9a9f97` (PN-heartbeat, every 5 min, expires 48 h).
Base: `ws/timeorder` at ec05f36. Gates before any change: `pixi run test gcc` 17/17 OK; strict golden
(`--rtol 0 --atol 0 --print-slack 0`) 278 passed, 0 failed/new/missing; counters of the base build recorded
(`stats-record`, 277 cases, 161 with statistics) in `/tmp/PN/stats-base`; base binary kept in `/tmp/PN/base/`.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 0 | Base gates, base records (gcc stats; clang, asan-gcc outputs) | in progress | | |
| 1 | Equal-time order by wavelength (heap kept), prediction, re-record, pinning case, docs | todo | | |
| 2 | Per-time-step (bucket) queue, bit-identical to step 1, timing | todo | | |
| 3 | Docs (B-188, comments, CHANGELOG) | todo | | |
| 4 | Acceptance gates, report | todo | | |

## Step 1: prediction (written before the change)

Method, beyond PD2's list: a scratch build of the base (`PN-wip/instr`, not merged) counts, at every pop of a
pending entry, whether other entries of the port share its time ("tie") and whether the wavelength order
(smallest first) would have taken another one than the |E| order ("disagree"). Where no pop disagrees, the
emission sequence of every port is the same under both orders, so the run is bit-identical. Run on all 277
cases (`check --keep`): **11 cases disagree**, all other simulating cases have 0 disagreements (many have
ties: e.g. `b050-pdet-beat` 1 tie, `probe-latch-tran` 2, `spar-3port-taylor` 4, all agreeing).

| case | ties / disagree | predicted change |
|---|---|---|
| `tb/lambda` | 88 / 68 | VCD of multi-wavelength nets (testbench probes trace every wavelength: the net's last write of a time step): another wavelength at some time steps |
| `regress/b001-fd-multilambda-stale` | 4 / 3 | `.op` listing: a multi-wavelength net (u, m, o) shows another wavelength (its last write); per-wavelength values unchanged (feed-forward) |
| `regress/f15-stats-file-print` | 4 / 3 | same listing change as b001, plus device statistics (`pending_mean`, possibly processed/emitted counts) |
| `regress/b033-crossing-two-lambda` | 2 / 1 | `.op` listing: out1 or out2 shows another wavelength |
| `regress/b062-mlprobe-computed-wavelength` | 1 / 1 | CSV row order in the first time step (rows in order of first sample: by wavelength now); values unchanged |
| `regress/b184-mlprobe-tran-start-multilambda` | 1 / 1 | same as b062 |
| `regress/b186-mlprobe-dc-wavelength-sweep` | 4 / 4 | CSV row order within points; values unchanged (feed-forward) |
| `regress/b186-mlprobe-dc-power-sweep-wdm` | 63 / 17 | ring (zero-delay loop in `.dc`): MLPROBE and PROBE values at tolerance level (B-198 mechanism), CSV row order |
| `regress/f15-wdm-crow-mlprobe` | 58018 / 19594 | MLPROBE values per wavelength at tolerance level (PD2: up to ~1e-4 relative inside the rings) |
| `regress/probe-latch-dc` | 4 / 2 | probably none visible: the PROBE latch takes the smallest (or swept) wavelength whatever the arrival order, feed-forward values; if any, CSV/VCD of the latched probe only through the discard path |
| `regress/probe-latch-dc-swept-f` | 5 / 5 | as probe-latch-dc |

Also: `gtest/unit_tests` (the order test `PendingEntryOrderMatchesPairComparison` is replaced by a wavelength-order
test: the gtest list changes). `stats-compare`: changes only in the 11 cases above (pending_* and, in the
CROW and b186 power sweep, processed/emitted/suppressed counts).
