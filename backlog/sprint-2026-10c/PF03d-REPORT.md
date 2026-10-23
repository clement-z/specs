# PF03d report: SPAR decisions of the second discussion (branch `ws/spar4`)

Prompt: `discover/orchestration/2026-10c/PF03d-spar-final.md`. Decisions: `backlog/DECISIONS-2026-10-21.md`, row
"SPAR second discussion"; details: `SPAR-discussion-2.md`. Base: fd239c8. Predictions (written before each change)
and measurements: `PF03d-progress.md`.

**Summary: items 1, 2, 3 and 5 are done on `ws/spar4`. Item 4 (pulse-pair tau0) is stopped.** It changes
`regress/spar-grid-warning` beyond the prediction: the warning text changes, and `out` moves ten times more than
predicted. The complete change is ready on the side branch **`PF03d-wip/item4`** (one commit, 5b41220, on top of
`ws/spar4`). It has the code, the docs and the re-recorded baseline, and every check passes. To merge it after the
maintainer accepts the change: `git cherry-pick 5b41220` (its subject says "NOT MERGED"; reword it when picking).

## Items

| # | Item | Outcome | Commit | Golden impact |
|---|---|---|---|---|
| 1 | Remove `delay_per` | done | c936414 | `regress/spar-ring-tran-output`, `err/spar-bad-delay-per` deleted; every other case bit-identical |
| 2 | Grid check on the actual intervals | done | 61d9316 (+ test 81bbe0d) | none |
| 3 | PCHIP slopes per run | done | 6d96f92 | none (no golden case uses `pchip`) |
| 4 | Pulse-pair tau0 | **stopped**: ready on `PF03d-wip/item4` (5b41220) | not on `ws/spar4` | 3 predicted cases; `regress/spar-grid-warning` beyond the prediction (below) |
| 5 | Docs and report | done | 7a4dc6d, this commit | none |

### 1. `delay_per` removed
- Product code: the device, header and parser hunks of 7e044eb, reversed (nothing changed them after 7e044eb).
  This removes `m_per_path_delays`, the per-output branches of `computeEntry()` and `onInput()`, the delay argument
  of `writeOutput()`, the `--vci` line and the per-output text of the negative-delay warning. `delay_per=` now gets
  the generic "Unknown keyword" error.
- Removed: the three `test_sparam.py` cases, `doc/sparam.md` "One delay per output", the `doc/netlist.md` row and
  `.tran` sentence, and the CHANGELOG sentence. The `tests/golden/README.md` counts are now 274 cases, err 100 and
  regress 125.
- `delay="auto"|"none"|<s>` is unchanged; "auto" was already documented as one delay per path.
- Golden: both cases deleted (cases.json, baseline directories, netlists, manifest case list). Strict check:
  275 passed, as predicted.

### 2. Grid check on the actual intervals
- `leaveOneOutErrors()` now uses `realImaginaryInterval(j-1, j) || realImaginaryInterval(j, j+1)`. The comment of
  `GRID_CHECK_THRESHOLD` and `doc/sparam.md` "Grid check" are updated.
- No golden change, as predicted. The B-190 gtest numbers are bit-identical.
- The prototype confirms the summary's values: the dispersive line 1 +- 4 ps goes from 0.957 to 0.0080; the ring
  k = 0.4 from 0.429 to 0.153.
- Regression test, inside the existing `SParamFile.GridCheckFlagsANullNarrowerThanTheGrid`: a 1 +- 4 ps dispersive
  line on 201 points over 1.5-1.6 um gives 0.0071 (linear and PCHIP). On the base code it gives 0.88 / 1.01 and the
  check fails.

### 3. PCHIP slopes per run
- New `pchipSlopesPerRun()`: `dmag`/`dphase` are computed per maximal run of magnitude/phase intervals.
- In the leave-one-out check, the window j-2..j+2 without j uses the same per-run slopes. Its [j-1, j+1] interval
  is re/im by item 2's rule.
- `dre`/`dim` still use the whole grid.
- Linear is unchanged, and an entry without re/im intervals is bit-identical.
- gtest MZI with PCHIP: 0.0127 → 0.0019 between points, LOO 0.035 → 0.0055. The bounds are tightened to 0.003 and
  0.01.
- `test_sparam.py` PCHIP ring: 9.56e-5 where |E| >= 0.1 (unchanged), 9.72e-5 anywhere (was 5.19e-4).
- Prototype: ring through 2.10e-4, LOO 0.00185; the dispersive line, the delay line and the BDC transmissions are
  bit-identical. These match the summary.
- No golden change, as predicted.

### 4. Pulse-pair tau0: stopped
Implemented as written in `PF03d-progress.md` "Algorithms":
- closed form with the mean step of the intervals used, leaving out intervals whose step exceeds 3 times the median
  step (`DELAY_REMOVAL_MAX_STEP_RATIO`);
- two Newton steps, each taken only when g' < 0 and the result is finite;
- the 1e-3 rad cut and f_ref unchanged.

tau0 matches every expected value:

| entry | expected (ps) | measured (ps) | was (median, ps) |
|---|---|---|---|
| ring 0.05 nm, through | 0.74919 | 0.749191 | 0.762 |
| ring 0.05 nm, drop | 1.1236 | 1.123595 | 0.806 |
| 1 nm file, through | 0.6235 | 0.623467 | -0.0145 |
| 1 nm file, drop | 1.1406 | 1.140650 | 0.987 |
| `wg` fixtures | +1.40097 | +1.400969 | |
| gtest MZI | | 4.99163 | 4.99049 |

The gtest MZI's asserted numbers are unchanged to 6 digits.

Golden result, strict, gcc. The set of changed cases is the predicted one, but not every change is:
- `cir/spar-add-drop-ring`, `regress/spar-ring-tran`: `out` at 1.55 um moves by 1.72e-14, **as predicted**.
- `regress/spar-grid-warning`, **beyond the prediction**:
  - The grid warning becomes "at 1.554e-06 m ... 0.704106" instead of "at 1.546e-06 m ... 0.707522" (predicted
    unchanged).
  - `out` moves by 1.76e-13 instead of about 1.7e-14. On that file tau0 moves by 0.64 ps, against 0.013 ps on the
    0.05 nm ring.
- **Cause:** items 2 and 4 together. The discussion measured each one separately. Item 4 alone (with the old
  doubled-interval rule) gives 0.707522 again: checked with a C++ variant and with the prototype. With both, the LOO
  at 1.546 and 1.550 um drops to about 0.37, so the mirror point of the null (1.554 um) becomes the worst.
- **Also:** the prototype finds that, with item 4, PCHIP on the ring's through path at its null is a little worse,
  2.1e-4 → 3.0e-4 between points. Where |S| >= 0.1 it stays 9.7e-5. The side branch's `doc/sparam.md` gives these
  numbers.
- **Side branch `PF03d-wip/item4`, 5b41220.** Everything was checked again after the rebase:
  - code and comments, `doc/sparam.md`, CHANGELOG, B-190 "Fix" paragraph, `tests/sparam/README.md`;
  - the three cases re-recorded, with a manifest note;
  - gcc and asan-clang: 16 OK, 1 skipped, no sanitizer message; strict golden 275 passed against its own
    baseline;
  - strict compare against `ws/spar4` with gcc, clang and asan-gcc: the same 3 cases in all three;
  - `stats-compare` (gcc): 274 same.
- **Maintainer decision needed:** accept the new warning value and the 1.8e-13 move (then cherry-pick), or ask
  for a different change.

### Validation (prototype)
An Opus subagent rebuilt the stdlib prototype in `/tmp/PF03d-proto`, outside the repository and not durable.
- It matches the base C++ bit for bit: 969344 values (tau0, LOO, gridCheck, S at the grid points and at 7 interior
  points per interval, linear and PCHIP), 30 files (fixtures, BDC, ring files, reference devices, gtest data). It
  also reproduces the three baseline traces.
- It matches my C++ bit for bit at stage 2+3 (`ws/spar4`) and at stage 2+3+4 (`PF03d-wip/item4`).
- Trap found: GCC compiles `std::polar` to `sincos()` in some functions and to `sin`/`cos` in others, and these
  differ in the last bit. The prototype therefore calls the same libm function as each call site.

## Decisions I had to take
1. **Manifest bookkeeping of the deleted cases.** I removed the two ids from the manifest's `cases` list and kept
   the `rerecords` history as it is: its entries record past `record` runs, and no run took place for a removal.
   PD5 did the same for `err/probe-wl-invalid` (1f44df4).
2. **Item 3 details** (written in the progress file before the code):
   - `dre`/`dim` keep whole-grid slopes ("today's scheme otherwise");
   - points outside any run get slope 0, which no interval uses;
   - in the leave-one-out window, the two outer intervals are classified as on the grid.
3. **Item 4 details:**
   - two Newton steps, each guarded (g' < 0, finite result);
   - the mean step is taken over the intervals used;
   - the median step of an even count is the mean of the two middle values;
   - the skip ratio is exactly 3, as a new constant;
   - the 1e-3 rad cut keeps the largest step of the whole grid.
4. **Item 4 stopped rather than merged.** The prompt predicted "warning text unchanged" and about 1.7e-14. The rule
   for an output change beyond the prediction is to stop the item and report it. The side branch makes the decision
   a single cherry-pick.
5. **Regression tests go inside existing gtests** (the item 2 dispersive line; tighter item 3 bounds) rather than
   new test names. The golden gtest case records the test names, and the prompt predicts no golden change for items
   2 and 3.

## Found, not done
- **The summary's MZI numbers for item 3 are not reproducible exactly.** The reference MZI's effective index is not
  given; with neff 2.3 the prototype gets 7.1e-4 / 8.3e-4 between points and LOO 0.0061 / 0.0058, against 9.2e-4 /
  0.0059. The right magnitude, but `doc/sparam.md` and the `GRID_CHECK_THRESHOLD` comment quote the summary's
  values (9.2e-4, 0.0059).
- **Skip rule (item 4) on alternating grids.** When the short steps are the majority and the interval count is odd,
  the median step is a short one, so every long step is skipped (84 of 167 intervals on 0.2/1.0 nm steps). It is
  still exact for a delay line, but which intervals are used depends on the grid's parity.
- **BDC reflection and isolation entries** (|S| about 1e-3, unused by the golden case) with item 4: tau0 moves by up
  to 0.19 ps and S between points by up to 3.3e-4, tens of percent of those entries. A bidirectional BDC would see
  this.
- Two-band test of the skip rule (60 nm gap, 1.4 ps line): the prototype gets 1.40097 ps with the rule and 0.634
  without (the summary's 0.40 used another gap).
- The manifest's `rerecords` history still names the two deleted cases (decision 1).
- Still open from before: B-191, B-192, B-193 (the run-table test is skipped in pixi: the one skip below), and B-194
  (out of scope).

## Final results (`ws/spar4`, HEAD = this commit)
- `pixi run test gcc`: 16 OK, 0 fail, 1 skipped (`tools - specs:run-table`, B-193).
- `pixi run test asan-clang`: 16 OK, 1 skipped, no sanitizer message. Run after every item; `lsan.supp` unchanged.
- clang, asan-gcc: 16 OK, 1 skipped each.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: **275 passed** (274 cases + gtest), 0 failed. The
  only changes are the two deleted cases.
- Strict `compare`, before (fd239c8) against after (7a4dc6d), with gcc, clang and asan-gcc: the **same set** in all
  three: 275 passed, nothing changed; the two deleted cases are no longer in the case list.
- `stats-compare` (gcc), before against after: 274 same, 0 differ. The two deleted cases are absent.
- Worktrees outside the repository: `/tmp/PF03d-wt/base` (fd239c8, branch `PF03d-wip/base`, before builds) and
  `/tmp/PF03d-wt/item4` (branch `PF03d-wip/item4`). Remove them after the merge with `git worktree remove`.
