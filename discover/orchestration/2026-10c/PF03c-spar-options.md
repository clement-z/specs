# Phase F03c — SPAR: per-path delay option, delay-removed interpolation (branch `ws/spar3`)

Follow `COMMON.md`. Phase id `PF03c`. Decisions: `backlog/DECISIONS-2026-10-21.md`, row "SPAR follow-ups review"
(2026-10-23). Read first `backlog/sprint-2026-10c/PF03b-REPORT.md` (what the previous phase built and measured),
`backlog/sprint-2026-10c/SPAR-discussion.md`, `doc/sparam.md`, `backlog/B-190-*.md`. Phase 2 of F-03 (broadband)
stays out of scope. Write your predictions (golden cases, counters) in `PF03c-progress.md` before each re-record.

## Scope
1. **Per-path delays as an option.** A `SPAR` keyword to enable or disable the per-path delayed copies of PF03b.
   Enabled is the default (current behaviour, outputs unchanged). Disabled restores the per-output delay of
   F-03 phase 1 (per output and wavelength, the group delay of the path with the largest |S| into that output;
   every write to that output uses it): look at the device before PF03b (48e510f) and keep its semantics. Choose
   the keyword's name and values to match the existing SPAR keywords (`delay=`, `interp=`, `passivity=`) and
   document it. `delay=none` and `delay=<s>` keep their meaning in both modes. Warnings (negative delay, rounding)
   adapted to the mode. Show that the disabled mode reproduces the pre-PF03b timing (e.g. against a build of
   48e510f on a data file where PF03b's interpolation change does not matter), and measure the cost of both modes
   on PF03b's stress case. Golden: one case per mode where they differ (the ring `.tran`).
2. **B-190: validate, then fix.** Delay-removed interpolation for tabulated entries: interpolate
   S(f) exp(-j 2 pi f tau0) with a delay tau0 per path taken out, then put the linear phase back. Choose tau0
   (e.g. the slope of a linear fit of the unwrapped phase over the file's band, or the mean group delay) and say why.
   **Validate first**, with the stdlib prototype approach of PF03b if useful: field error between grid points
   with and without, on the unbalanced MZI of PF03b (0.023 today), the resolved ring on 0.05 nm, a delay line
   (must stay exact, <= 4e-13 in the phase-convention tests), the SiEPIC BDC, and the under-resolved cases. If it
   does not clearly improve the MZI, or degrades a resolved case, **stop this item and report the numbers**. If it
   does: implement it together with the mixed real/imaginary rule (say how they combine, e.g. the rule applied to
   the delay-removed S), for `linear` and `pchip`; grid-point values unchanged; the grid-adequacy check uses the
   same interpolant (report its new numbers); the group delay keeps coming from the phase table.
3. **Stale comments** in the golden netlists `regress/spar-ring-tran` ("delay=auto per output ...") and
   `cir/spar-add-drop-ring` ("negative group delays ... are reported"): correct them. Their manifests change by the
   netlist hash only (re-record with a note).
4. Docs: `doc/sparam.md` (delay modes, interpolation), `doc/netlist.md` (the new keyword), `CHANGELOG.md`
   `[unreleased]`, the status of `backlog/B-190-*.md` and its row in `backlog.md`.

## Acceptance
- `pixi run test gcc`, `pixi run test asan-clang`: all OK, no sanitizer message.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: only the cases you predicted change, each as
  predicted.
- clang and asan-gcc: before/after `compare` (strict) shows the same set of changed cases.
- `stats-compare` (gcc) before/after: only predicted `SPAR` cases differ.
- Report `backlog/sprint-2026-10c/PF03c-REPORT.md` (items, measurements, decisions you took, golden and counter
  impact, found-not-done) committed on `ws/spar3`: that is the completion marker.
