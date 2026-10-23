# PF03c report: SPAR per-path delay option, delay-removed interpolation (F-03, B-190, branch `ws/spar3`)

Prompt: `discover/orchestration/2026-10c/PF03c-spar-options.md`. Decisions: `backlog/DECISIONS-2026-10-21.md`, row
"SPAR follow-ups review" (2026-10-23). Base: 30f4fcd. Predictions, written before each re-record, and the
measurements in detail: `PF03c-progress.md`.

## Items

| # | Item | Outcome | Commit | Golden impact |
|---|---|---|---|---|
| 1 | Per-path delays as an option (`delay_per`) | done | 7e044eb (+ 55afed9: error case) | new cases `regress/spar-ring-tran-output`, `err/spar-bad-delay-per`; nothing else |
| 3 | Stale comments in two golden netlists | done | 842d0fe | `regress/spar-ring-tran`, `cir/spar-add-drop-ring`: run manifest netlist hash only |
| 2 | B-190: validate, then fix | validated, done | 4f323e4 | `out` at 1.55 um in the four ring cases (~1e-12; 4e-15 for the 11-point file); gtest case |
| 4 | Docs | done | e9c3d85 (item docs landed with items 1 and 2) | none |

Order: 1, 3, 2, 4 (2 waited for its validation, run by an Opus subagent while 1 and 3 were done).

### 1. `delay_per="path"|"output"`
- **Keyword**: `delay_per`, values `"path"` (default: PF03b's delayed copies, unchanged) and `"output"` (one delay per
  output). Named like the other SPAR keywords (one lowercase key, string values in quotes, case-insensitive); it
  reads as "delay per path / per output" next to `delay=`. A bad value is an error (`err/spar-bad-delay-per`).
- **`"output"`** restores 48e510f's semantics: per output and wavelength, the group delay of the path with the
  largest |S| into it (the first if equal, lit or not), used by every write to that output through the output
  port's pending writes; an input event recomputes each output it reaches from the latest inputs (absolute
  writes). Implemented in the same device: the copies are set at once (they equal the inputs), and the write
  carries the output's delay. `delay="none"` and `delay=<s>` keep their meaning in both modes (none: identical
  traces; fixed: same times and values, a write leaves one delta cycle earlier with `"output"`, which can swap
  rows of one time in a trace).
- **Warnings**: the B-014 rounding check is unchanged (once per device, on the delays actually computed: in
  `"output"` mode only the dominant paths'). Negative delay in `"output"` mode: reported once, when the output is
  first written, naming the dominant path and saying it is used for every path into that output ("the output acts
  as a connection without delay"), with the grid hint. `--vci` prints `delay_per = output` in that mode only (so
  default outputs are unchanged).
- **Against 48e510f** (a gcc build of it, no keyword, vs HEAD with `delay_per="output"`; `.tran`, both inputs lit with
  steps; `delay=auto`, `"none"`, `2e-12` each): the two-waveguides-into-a-coupler device of `test_sparam.py`
  (delays 1.4/4.2 ps) at two wavelengths and the 1 nm ring file of 48e510f at 1.5525 um: **traces byte-identical**
  (9 runs). On the ring at 1.551, 1.5505 and 1.55 um (intervals next to the 1 nm grid's null, where PF03b's mixed
  interpolation applies) the rows and times are the same and only fields differ (the interpolation).
- **Cost** (ring 0.05 nm file, both inputs lit, 2 x 200000 values 0.25 ps apart, gcc release, 7 runs, median user
  time): at 1.55 um (2 of 4 paths delayed) path 0.96 s, output 0.91 s; at 1.552 um (4 delayed) path 1.04 s, output
  0.94 s; peak RSS ~31 MB both; port writes equal (1150033); `pending_max` 1 (path) vs 14 / 5 (output). PF03b had
  measured 1.02 -> 1.63 s on its own stress case, whose stimulus is not recorded; mine repeats values more often.
- Tests: `test_sparam.py` per-output-tran (every write to o1 at input event + 1.40 ps, to o2 + 4.20 ps: the larger
  path into o2 is b's), negative-delay/per-output (1 nm golden file at 1.551 um, `in` alone: in->out and add->drop
  reported, both flagged as grid artefacts) and per-output-positive (0.05 nm ring: no warning).
- Golden: `regress/spar-ring-tran-output` (out and drop both at +1.721 ps, no warning, `--vci` +1 line), as
  predicted; counters equal to `regress/spar-ring-tran`'s but out's `last_emission_s` (21.721 ps).

### 3. Stale comments
`regress/spar-ring-tran` now says per-path delays (out follows in->out, clamped, one warning; drop its own delay);
`cir/spar-add-drop-ring` says `.dc` has no delays, so no negative-delay warning. Its `cases.json` note no longer
says the header predates that. Strict check before the re-record: `netlist_files[0].sha256` only, in both.

### 2. B-190
**Validation** (Opus subagent, stdlib prototype outside the repo, `/tmp/PF03c-proto/scripts/proto.py`, results
`/tmp/PF03c-proto/RESULTS.md`): the prototype is bit-identical to the C++ (27624 `.dc` evaluations, 8 files, linear
and PCHIP) and reproduces the golden outputs. The unbalanced MZI (two 50/50 couplers, arms 300/400 um, ng 4.3,
2 dB/cm, 1.54-1.56 um by 0.2 nm, 28 points per FSR) reproduces PF03b's numbers (0.0226 / 0.0160 / 0.0704 / 0.0790 /
LOO 0.114). References: SPECS exports at the midpoints (closed forms agree within 1.3e-12). Worst field error between
grid points, today -> the implemented fix, with the grid check (leave-one-out, LOO; its values were re-measured with
the C++ code):

| device | linear | LOO linear | PCHIP | LOO PCHIP |
|---|---|---|---|---|
| unbalanced MZI, 0.2 nm | 0.0226 -> **0.0016** | 0.114 W -> **0.0064** | 0.016 -> 0.016 | 0.109 W -> 0.043 W |
| ring k=0.85, 0.05 nm | 9.7e-4 -> 5.4e-4 (4.0e-4 where \|S\| >= 0.1, same) | 0.0039 -> 0.0021 | 6.6e-4 -> 5.65e-4 | 0.0037 -> 0.0019 |
| delay line (`wg.*` fixtures) | 3.96e-13, bit-identical | 3.9e-13 | 4.38e-13, bit-identical | 4.2e-13 |
| SiEPIC BDC (no reference) | bit-identical | 0.0024 (same) | bit-identical | 0.0022 (same) |
| smooth dispersive lines (synthetic) | bit-identical | same | bit-identical | same |
| ring k=0.4, 0.05 nm (under-resolved) | 0.21 -> 0.044 | 0.43 W (same) | 0.19 -> 0.052 | 0.39 W (same) |
| ring k=0.85, 1 nm; golden 11-point file | 0.54 (same) | 0.71 W (same) | 0.52-0.54 (same) | 0.71 W (same) |

(W: above 0.02, a warning.) It clearly improves the MZI (14x, and its grid no longer warns) and degrades no
resolved case, with either interpolant.

**Implementation** (`SParamData::removeDelay()`, `realImaginaryInterval()`, `interpolate()`, `leaveOneOutErrors()`):
- tau0 per entry = **median of the finite-difference group delays at the grid points** (`tau_s`, already
  computed); 0 when it turns S by less than 1e-3 rad over the largest grid step; f_ref = the first grid point.
  Why the median: the unwrapped phase of a path with nulls has a pi step at every null, all taken the same way, so
  a least-squares fit (weighted or not) and the end-to-end slope see a staircase and return one arm's delay
  (4.28-4.42 ps instead of the MZI's mean 5.02 ps; MZI 0.0033 instead of 0.0016, and with PCHIP on the detrended
  phase the ring worse than today, 9.1e-4); the median only sees the one or two differences next to each null. On the BDC all estimates
  agree within 0.6 fs. The cut keeps nearly flat phases exact (without it the gtest
  `MixedInterpolationNearANull`, S linear in f, fails: tau0 = -1.09 fs gives 8.6e-6).
- **How it combines with the mixed rule**: re/im intervals use the real and imaginary parts of the delay-removed
  S~ (linear, or PCHIP slopes of those), the delay put back after; an interval is re/im when |S| at either end is
  below 0.07 max|S| (today's test, unchanged since |S~| = |S|) **or when the segment between S~ at its two ends
  passes within 0.07 max|S| of 0** (a null between two points whose |S| is larger). Magnitude/phase intervals are
  exactly today's code, for linear and PCHIP. Grid-point values unchanged (the file's magnitude and phase); group
  delays still from the phase table; the grid check uses the same interpolant (and the same tau0).
- Tests: gtest `SParamFile.DelayRemovedInterpolationOfARotatingPathWithNulls` (a two-delay MZI entry with 28 points
  per FSR: linear 0.022 -> 0.0035 between points, LOO 0.094 -> 0.0089, under the threshold; PCHIP 0.018 -> 0.0127;
  a delay line in the same file stays exact; prototype values identical). `test_sparam.py` between-points (ring):
  linear 9.35e-4 -> 5.21e-4 anywhere, 3.84e-4 where |E| >= 0.1 (same); PCHIP unchanged; phase-convention tests
  unchanged (3.62e-13 `.dc` between points, 2.84e-15 `.tran`).
- Golden: `out` at 1.55 um (a sweep point ~1e-9 of an interval from the through null's grid point, re/im today) in
  `cir/spar-add-drop-ring` (power 5.194275359220189e-06 -> 5.194275354576741e-06), `regress/spar-ring-tran` and
  `regress/spar-ring-tran-output` (field ~1e-12), `regress/spar-grid-warning` (4e-15; warning text unchanged,
  0.707522), and the gtest case; all as predicted from the prototype. Every other case bit-identical; counters
  unchanged.

### 4. Docs
`doc/sparam.md` (Interpolation: the delay removal and the segment test, with numbers; Grid check: new numbers;
"One delay per output"), `doc/netlist.md` (`delay_per` row, `interp` row, `.tran` sentence), `CHANGELOG.md`
`[unreleased]` (SPAR entry: `delay_per`, delay-removed interpolation), `tests/sparam/README.md` (grid-check numbers),
`tests/golden/README.md` (counts), F-03 brief status block, `backlog/B-190-*.md` (validation, fix, `fixed in
4f323e4`) and its row in `backlog.md`, comments of `MIXED_INTERP_THRESHOLD` / `GRID_CHECK_THRESHOLD`.

## Decisions I had to take (not covered by the decisions file)
1. Keyword name and values: `delay_per="path"|"output"`.
2. `"output"` mode: the negative-delay warning is per (dominant) path, at the first write of the output, and says
   that every path into that output is undelayed; `--vci` prints `delay_per` only in that mode (default stdout
   unchanged).
3. B-190 tau0 = median of `tau_s` with a 1e-3 rad cut, f_ref = first grid point (above).
4. **The segment test** added to the near-null criterion. Without it (the endpoint rule alone on S~, variant B),
   the MZI between points is the same 0.0016 but its grid check stays at 0.114 and warns on a grid that
   interpolates within 0.0016: the check would no longer describe the interpolant. On every grid tested the
   segment test selects the same intervals as today on the actual grid (28/400 for the MZI, 46/2400 for the ring,
   0 for the BDC and delay lines), except the under-resolved k = 0.4 ring (where it helps, 0.21 -> 0.044); it acts
   mainly in the leave-one-out check.
5. **PCHIP keeps today's magnitude/phase** (variant "Es"). PCHIP on the detrended phase ("Er") would give the MZI
   0.0035 (instead of 0.016, LOO 0.0078 instead of 0.043) and the ring 2.7e-4, but degrades smooth dispersive paths
   (synthetic line with 5 +- 1.5 ps of group delay: 6.0e-7 -> 1.9e-4, still below linear's 7.5e-4) and moves the
   BDC by up to 2.75e-4 (LOO +3 %). The brief says to stop rather than degrade a resolved case, so the
   conservative choice; Er for PCHIP is a maintainer option (below). Linear (the default) is the same in both.
6. Re-measured stress case for item 1 (PF03b's was not recorded).

## Found, not done
- **PCHIP on the detrended phase** (decision 5): better on rotating paths with nulls, slightly worse on smooth
  dispersive ones; needs a maintainer choice (needs-human).
- With the delay removal the threshold 0.07 is no longer critical (the same results from 0.03 to 0.1); without
  it, today's rule was 0.01 above a cliff (at 0.06 the 0.05 nm ring would warn, 0.053). No change made.
- PF03b documented the BDC's grid check as 0.0026 and the ring with PCHIP as 2.0e-3 between points; the C++ gives
  0.0024 (linear) / 0.0022 (PCHIP) and 6.6e-4 (the subagent reproduces 2.0e-3 with a threshold of 0.05).
  Corrected in `doc/sparam.md` and `tests/sparam/README.md` (BDC).
- The group delays near the MZI's nulls are as unresolved as before (the grid check tests S, not the delay; known
  from PF03b, now also true of a grid that no longer warns).
- The B-190 prototype and the item-1 comparison scripts are outside the repository (`/tmp/PF03c-proto`,
  `/tmp/PF03c-item1`, `/tmp/PF03c-loo`); `/tmp` is not durable. A committed check of the MZI case exists as the gtest.
- Still open from before: B-191 (long netlist lines), B-192 (data file path resolution), B-193 (run-table test
  skipped in pixi: the one skip below).

## Final results (HEAD of `ws/spar3`)
- `pixi run test gcc`: 16 OK, 0 fail, 1 skipped (`tools - specs:run-table`, B-193).
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: **277 passed** (275 + 2 new cases), 0 failed.
- `pixi run test asan-clang`: 16 OK, 1 skipped (same), no sanitizer message; run after items 1, 2 and at the end;
  `lsan.supp` unchanged.
- Strict `compare`, before (30f4fcd) vs after (e9c3d85), gcc, clang and asan-gcc: the **same set** in all three:
  3 changed (`cir/spar-add-drop-ring`, `regress/spar-ring-tran`, `regress/spar-grid-warning`) and 1 new
  (`regress/spar-ring-tran-output`); `err/spar-bad-delay-per` was added afterwards (55afed9).
- `stats-compare` (gcc) before vs after item 2: 274 same, 0 differ, 1 new (`regress/spar-ring-tran-output`).
- Worktrees outside the repo: `/tmp/PF03c-wt/base` (30f4fcd: clang, asan-gcc builds), `/tmp/PF03c-wt/old` (48e510f,
  gcc build).
