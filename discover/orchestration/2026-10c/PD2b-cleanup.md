# Phase D2b — remove the no-coalescing switch and the delta-write mode (branch `ws/cleanup2`)

Follow `COMMON.md` and `PD-common-f15.md`. Phase id: `PD2b`. Scope: "Decisions of 2026-10-23 (phase D)" in
`backlog/DECISIONS-2026-10-21.md`. One commit per item; the tree builds and passes at every commit.

1. **No-coalescing switch** (added in D1, ec3e0f3 and 51db5ea; extended in D2): remove `--no-coalescing`,
   `.options coalesce`, the config member, the FIFO queue path of `OpticalOutputPort` and its tests and golden
   cases (`regress/f15-coalesce-options`, `regress/f15-no-coalescing-cli`, their netlists), and every mention in
   `doc/numerics.md`, `doc/netlist.md`, `CHANGELOG.md` (`[unreleased]`: the D1 entry; it was never released, so
   edit it rather than adding a "removed" line). `.options coalesce=...` is then an unknown option: handle it
   exactly like any other unknown `.options` key today, and add a golden case showing that. Keep the event cap
   (`--max-events`, `.options maxevents`) and its cases; reword anything that cited no-coalescing as a use of it.
   Keep the `coalesced` counter of `--stats` (it counts the default merges).
   Predicted golden impact: `cli/help`, `cli/no-args` (help lines removed), stats JSON files whose `options`
   object loses `coalesce` (only that key), gtest list; nothing else. Strictly bit-identical outputs otherwise.
2. **Delta-write mode and generic devices:** remove `OpticalOutputPort::m_use_deltas` and its branches, and the
   unused devices `GenericTransmissionDevice`, `Generic2x2Coupler`, `GenericWaveguide` (files, build lists,
   includes, the commented-out lines in `src/tb/`), plus doc/stat wording about "devices that write deltas".
   Predicted golden impact: none (they are unreachable from netlists and testbenches): bit-identical, counters identical.
3. **Roadmap and backlog bookkeeping:** update `discover/scout/F-03-sparameter-device.md` and the F-03 line of
   `discover/scout/FEATURES.md`: the generic device it planned to refactor is removed (decision 2026-10-23), F-03
   is to be written from scratch as a state-holding, per-wavelength device; the former code is at cc1e52e.
   Record as a new backlog item (next free B-number, `backlog/` file + `backlog.md` row, severity by your
   judgement, status open; verify with a minimal repro first, do not fix): the PD2 report's finding that an MLPROBE
   at the start of a `.tran` seems to record only the last wavelength written to its net (two CW sources at 1.55
   and 1.56 um into a merger: 1560 nm reads 0 for the whole run).
4. Gate as in `PD-common-f15.md` (strict gcc golden, clang/asan before/after, counters before/after on the
   pre-existing cases minus the removed ones), then `backlog/sprint-2026-10c/PD2b-REPORT.md`.
