# PN progress: equal-time order by wavelength, then a per-time-step queue (branch `ws/timeorder`)

Heartbeat: `8b9a9f97` (PN-heartbeat, every 5 min, expires 48 h).
Base: `ws/timeorder` at ec05f36. Gates before any change: `pixi run test gcc` 17/17 OK; strict golden
(`--rtol 0 --atol 0 --print-slack 0`) 278 passed, 0 failed/new/missing; counters of the base build recorded
(`stats-record`, 277 cases, 161 with statistics) in `/tmp/PN/stats-base`; base binary kept in `/tmp/PN/base/`.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 0 | Base gates, base records (gcc stats; clang, asan-gcc outputs) | done | 560875c | base records in `/tmp/PN/rec-base-{clang,asan-gcc}` |
| 1 | Equal-time order by wavelength (heap kept), prediction, re-record, pinning case, docs | done | f1b94ea (prediction), 2674103 | 6 of the 11 predicted cases changed, none outside |
| 2 | Per-time-step (bucket) queue, bit-identical to step 1, timing | done | 94ca1f3 | strict golden unchanged, stats 278 same; faster at every channel count |
| 3 | Docs (B-188, comments, CHANGELOG) | done | 4ab8bf6 | B-188, B-015, B-198, backlog.md, CHANGELOG |
| 4 | Acceptance gates, report | done | (report commit) | `PN-REPORT.md`; heartbeat 8b9a9f97 deleted after it |

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

## Step 1: result (2674103)

Strict golden before the re-record: 7 failed, all predicted: `tb/lambda` (stdout: the OUT listing of the
testbench shows the wavelengths of a time step in the other order), `b001-fd-multilambda-stale` and
`f15-stats-file-print` (`.op` listing: m, o, u now show 1560 nm, the last write; the stats file and print:
`pending_mean` of the mergers 1.625 -> 1.5, of `ROOT/MERGERM` 1.75 -> 1.5), `b062-mlprobe-computed-wavelength`
and `b184-mlprobe-tran-start-multilambda` (the operating-point listing before the `.tran`: m shows 1560 nm
instead of 1550 nm; I had predicted a CSV row-order change for these two, which did not happen: the change is
the listing category of the prediction instead), `f15-wdm-crow-mlprobe` (MLPROBE at 1551.2 nm only, max
3.76e-8 sqrt(W) on a peak of 9.11e-4, 4.1e-5 of the peak; the 3 other channels identical), `gtest/unit_tests`
(test list). Predicted but unchanged: `b033-crossing-two-lambda`, both `b186-*`, both `probe-latch-dc*`.

Pinning case `regress/pn-wdm-crow-equal-time-order` (`crow_gen.py 3 --nwl 4 --pulses 1 --duration 4e-10`,
MLPROBEs on through, r2b_out, drop; default tolerances; VCD 1.3 MB, stored gzipped, 460 kB): against the base
build, 1551.2 nm moves by up to 9.0e-5 of its peak at through, 3.3e-5 in ring 2, 4.1e-5 at the drop; the other
channels identical. Recorded with `--repeat 3` (reproducible).

`stats-compare` base vs step 1: 274 same, 3 differ (b001 and f15-stats-file-print: `pending_mean`; the WDM
CROW: e.g. couplers emitted 44085 -> 41640, coalesced 18644 -> 21094, waveguides coalesced 2445 -> 0),
1 new (the pinning case). Gates after the re-record: `pixi run test gcc` 17/17, strict golden 279 passed,
`pixi run test asan-clang` 17/17 (no sanitizer message in the test log).

## Step 2: per-time-step queue (94ca1f3)

Design: the entries sorted by (time, wavelength) in a power-of-2 ring buffer, so each time step is one
contiguous run (bucket) in wavelength order; `write()` merges or adds in one call (append when later than the
last entry; direct merge into the last entry; else a binary search, then merge or insert shifting the shorter
side); `pop()` advances the head. No per-bucket allocation (a vector per bucket would cost an allocation or a
pool and an indirection per access). Same total order as step 1, so the same pops: bit-identical by
construction.

Checks: strict golden 279 passed (no re-record), `stats-compare` step 1 vs step 2 278 same; CROW 3 rings 4
channels with every net traced (61 MB VCD), 3 rings 64 channels traced (10.5 MB), 10 rings 16 channels: VCD,
stdout (paths aside) and counters identical between the step-1 build (`/tmp/PN-wt/s1`, 2674103) and step 2.
`pixi run test gcc` 17/17, `pixi run test asan-clang` 17/17 (no sanitizer message).

Timing (idle host, no build running; `tests/bench/bench.py`'s `run_once`, `--no-trace` and for C-notrace also
`-o`; user time of the child, median of 3, N = 30 of 5; step 1 and step 2 interleaved), ns per processed event:

| case | processed | step 1 | step 2 | ratio |
|---|---|---|---|---|
| N=3, 1 ch | 718030 | 119.8 | 115.6 | 0.965 |
| N=3, 4 ch | 2856510 | 116.2 | 102.6 | 0.883 |
| N=3, 16 ch | 11783441 | 132.1 | 106.8 | 0.808 |
| N=3, 64 ch | 45317350 | 157.6 | 117.7 | 0.747 |
| N=10, 1 ch | 8394360 | 128.9 | 122.9 | 0.954 |
| N=10, 4 ch | 33568262 | 146.2 | 130.7 | 0.894 |
| N=10, 16 ch | 134057535 | 188.9 | 139.6 | 0.739 |
| C-notrace N=30 (`--no-trace`) | 12289043 | 151.9 | 140.3 | 0.923 |
| C-notrace N=30 (`-o`) | 12289043 | 150.0 | 140.0 | 0.933 |

Single channel faster, not slower. Profile after (perf, N = 10): `emit_due_entry()` 16.5 % (1 ch), 19.8 % (16 ch);
PM measured 17 % / 34 % with the heap. B-015 delay line (8 x 10 cm, 20000 values 1 ps apart through a value
file; 1230 entries pending per port on average): 0.10 s -> 0.09 s user, VCD identical.

## Acceptance gates (tip 14862ed, code of 94ca1f3)
- `pixi run test gcc`, `clang`, `asan-gcc`, `asan-clang`: 17/17 each; no sanitizer message in the asan logs.
- Strict golden (gcc): 279 passed, 0 failed/new/missing.
- clang and asan-gcc, base vs tip (`compare --rtol 0 --atol 0 --print-slack 0`): 271 passed, 7 failed (the 6
  cases + gtest), 1 new (the pinning case): the same set as gcc.
- Scratch worktrees left in place (not merged): `/tmp/PN-wt/instr` (branch `PN-wip/instr`, uncommitted
  instrumentation of the base), `/tmp/PN-wt/s1` (branch `PN-wip/s1` at 2674103, the step-1 timing build).
