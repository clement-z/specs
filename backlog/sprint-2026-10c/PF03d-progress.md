# PF03d progress: SPAR decisions of the second discussion (branch `ws/spar4`)

Prompt: `discover/orchestration/2026-10c/PF03d-spar-final.md`. Decisions: `backlog/DECISIONS-2026-10-21.md`, row
"SPAR second discussion"; details and expected values: `SPAR-discussion-2.md`. Base: fd239c8.

Heartbeat: `44fdc915` (PF03d-heartbeat, every 5 min, expires in 48 h).

Before builds: worktree `/tmp/PF03d-wt/base` (fd239c8, branch `PF03d-wip/base`): gcc, clang, asan-gcc.
Prototype (Opus subagent): `/tmp/PF03d-proto` (outside the repository, not durable).

## Items

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 1 | Remove `delay_per` | done | (this commit) | product code = reverse of 7e044eb's device/header/parser hunks (no later change to them); 2 golden cases removed; gcc 16 OK 1 skip, strict golden 275/275; asan-clang 16 OK 1 skip, no sanitizer message |
| 2 | Grid check on the actual intervals | done | (this commit) | gcc 16 OK 1 skip, strict golden 275/275 (no change, as predicted); asan-clang 16 OK, no sanitizer message; B-190 gtest numbers bit-identical (linear 0.00345946 / LOO 0.00892046, PCHIP 0.0127283 / 0.0348905) |
| 3 | PCHIP slopes per run | done | (this commit) | gcc 16 OK 1 skip, strict golden 275/275 (no change, as predicted); asan-clang 16 OK, no sanitizer message; B-190 gtest PCHIP 0.0127283 -> 0.00191221, LOO 0.0348905 -> 0.00553862 (linear bit-identical); test_sparam.py pchip between points 9.56e-5 where \|E\| >= 0.1 (same), 9.72e-5 anywhere (5.19e-4) |
| 4 | Pulse-pair tau0 | **stopped** (output change beyond the prediction) | side branch `PF03d-wip/item4` (5b41220, worktree `/tmp/PF03d-wt/item4`) | tau0 as expected; `regress/spar-grid-warning` warning text changes (0.707522 at 1.546 um -> 0.704106 at 1.554 um) and its `out` moves by 1.7e-13, not ~1.7e-14 (Log) |
| 5 | Docs and report | done | 7a4dc6d, report commit | item 2 regression test 81bbe0d; side branch rebased onto 7a4dc6d (5b41220) |

## Algorithms (written before the code, shared with the prototype)

Item 2, `leaveOneOutErrors()`: point j is interpolated in re/im parts when
`realImaginaryInterval(t, j - 1, j) || realImaginaryInterval(t, j, j + 1)` (was `(t, j - 1, j + 1)`).

Item 3, PCHIP magnitude/phase slopes per run: interval k = [k, k+1] is re/im when
`realImaginaryInterval(t, k, k + 1)` (after `removeDelay()`). A run is a maximal sequence of consecutive
magnitude/phase intervals; `dmag`/`dphase` at the points of a run = `pchipSlopes()` of the run's points alone (two
points: the secant); points of no run keep 0 (never used). `dre`/`dim` unchanged (whole grid). Leave-one-out:
the window j-2..j+2 without j; its intervals are [j-2, j-1] (re/im as on the grid), [j-1, j+1] (re/im by item 2's
rule), [j+1, j+2] (as on the grid); for a magnitude/phase point the slopes come from the run of the window that
contains [j-1, j+1]. Without re/im intervals: bit-identical to before.

Item 4, `removeDelay()` tau0 (m >= 2): steps h_k = f[k+1] - f[k]; median step (median of h, the mean of the two
middle values for an even count); interval k is used when h_k <= 3 * median step. z_k = S(k+1) conj S(k) (file
values re/im), Z = sum of z_k over used k in ascending k, mean step = (sum of used h_k) / count. tau0 =
arg(Z) / (2 pi mean step) (0 if Z == 0). Then two Newton steps on
g(tau) = sum_k |z_k| sin(arg z_k - 2 pi h_k tau) h_k, g'(tau) = -2 pi sum_k |z_k| cos(arg z_k - 2 pi h_k tau) h_k^2
(used k), tau -= g / g', a step taken only when g' < 0 and the result is finite. Then the 1e-3 rad cut over the
largest step of the whole grid and f_ref = first grid point, unchanged.

## Predictions

(written before each re-record)

- Item 1 (before the change): no output of any remaining case changes (the default path mode is the code that
  stays); `regress/spar-ring-tran-output` and `err/spar-bad-delay-per` disappear. Result: strict golden 275 passed
  (274 + gtest), 0 new, 0 missing.
- Item 2 (before the change): no golden output changes (summary: identical worst values with linear on every
  fixture and on the golden 1 nm file, 0.707522 at 1.546 um; only the PCHIP 1 nm value moves, no golden case uses
  PCHIP); all `SParamFile` gtests pass (the B-190 gtest's linear LOO 0.0089 may move in its last digits).
- Item 3 (before the change): no golden output changes (no golden case uses `interp=pchip`; the gtest case
  records test names and results only). Linear: bit-identical everywhere. PCHIP: bit-identical on entries without
  re/im intervals (delay lines, dispersive line, BDC, the B-190 gtest's delay line); the B-190 gtest's PCHIP
  worst value drops from 0.0127 (summary's reference MZI: 0.0165 -> 9.2e-4; ring through 6.3e-4 -> 2.1e-4);
  `test_sparam.py` PCHIP between-points bound (2e-4 where |E| >= 0.1) still holds with a changed value.
- Item 4 (prompt and summary): `out` of `cir/spar-add-drop-ring`, `regress/spar-ring-tran`,
  `regress/spar-grid-warning` move by about 1.7e-14; warning text unchanged (0.707522 at 1.546 um); tau0 ring
  through 0.74919 ps, drop 1.1236, 1 nm file 0.6235 / 1.1406, wg +1.40097; gtest numbers may move in the last
  digits.

## Log

- Item 3: the reference-MZI and ring numbers written in doc/sparam.md and the GRID_CHECK_THRESHOLD comment
  (9.2e-4, 2.1e-4, LOO 0.0059) are the summary's; to be confirmed with the C++ through the prototype's driver
  (item 5).

- Item 1: manifest bookkeeping = the two ids removed from the `cases` list; the `rerecords` history is kept as is
  (entries of past records stay true; no record was run for a removal), as the PD5 removal of
  `err/probe-wl-invalid` (1f44df4) did for its case list.
- Item 4, measured (C++, gcc -O3, `/tmp/PF03d-meas/tau0.cpp` and `gtestnum.cpp`): tau0 ring 0.05 nm through
  0.74919125801533026 ps (median 0.76199814204238836), drop 1.1235952555311075 (0.80631412032717187); 1 nm file
  through 0.623467229805865 (-0.014514455988249953), drop 1.1406498331338579 (0.98742985968978259); `wg`
  fixtures 1.400969199832238 (1.4009691998322635); BDC and asym fixtures unchanged (0 or same); B-190 gtest MZI
  4.99049 -> 4.99163 ps, worst values unchanged to 6 digits (linear 0.00345946 / LOO 0.00892046, PCHIP
  0.00191221 / 0.00553862). All as expected.
- Item 4, strict golden (gcc): exactly the three predicted cases fail, but:
  - `cir/spar-add-drop-ring` out at 1.55 um: field moves by 1.72e-14 (re 7.0e-15, im 1.57e-14), power
    5.194275354576741e-06 -> 5.194275354654886e-06: as predicted;
  - `regress/spar-ring-tran` out: 1.7e-14 (re), as predicted;
  - `regress/spar-grid-warning`: **the warning changes**, "at 1.546e-06 m ... 0.707522" -> "at 1.554e-06 m ...
    0.704106", and `out` moves by 1.7e-13 (re 1.22e-13, im 1.25e-13; tau0 of that file moves by 0.64 ps, against
    0.013 ps for the 0.05 nm file): both beyond the prediction.
  - Cause of the warning change: the interaction of items 2 and 4, which the discussion measured separately. A
    variant with the pulse-pair tau0 and the old doubled-interval LOO rule gives 0.707522 again (the
    discussion's measurement); with item 2's rule the worst point moves to the mirror point of the null.
  - Decision: item 4 stopped on `ws/spar4` (rule: an output change beyond the prediction); the complete change
    (code, docs, re-recorded baseline) is prepared on `PF03d-wip/item4` for the maintainer.
