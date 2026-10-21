# P1 — golden harness hardening (branch `ws/harness`)

Follow `discover/orchestration/2026-10b/COMMON.md`. Phase id: `P1`. Model: Opus 5.5 (high); this phase is small,
do it yourself (subagents optional for a fresh-eyes review).

Items (all approved): **B-129, B-126, B-127, B-128**, in that order (B-129 first: later phases rely on stable output
across OpticalSignal copy counts).
- **B-129**: normalise printed `[NNNN]` OpticalSignal IDs per stream to first-appearance ranks, so equality between
  prints is preserved and absolute offsets vanish.
- **B-126**: strict line order by default. Per-case `unordered_streams` only for cases whose order depends on heap
  layout today (B-016, fixed later in P4: list them in the report so P4 can remove the exceptions).
- **B-127**: NEW and MISSING cases and a skipped gtest fail `check` by default (`--allow-new` for the record
  workflow); `--only` matching nothing exits non-zero.
- **B-128**: no print slack for numbers with fewer significant digits than the threshold in the backlog item
  (choose the threshold from data: every case must still pass on a clean build, and a 1-digit change in a tolerance
  or config printout must fail).

Acceptance:
- On a fresh build of this branch, `check` passes with the new defaults for gcc **and** clang, and the gtest case runs.
- Re-recording is allowed only where the normalisation changes the stored form (IDs); stored values must not change.
- Demonstrate each new failure mode with a perturbation (reordered lines, missing case, skipped gtest, empty
  `--only`, a 1-digit tolerance change) and record the commands and results in the report.
- Update `tests/golden/README.md`, the `meson test` golden registration if flags change, the CI files if they call
  `run_golden.py` directly, and `CLAUDE.md` commands.
