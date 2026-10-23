# Phase F03b — SPAR follow-ups (branch `ws/spar2`)

Follow `COMMON.md`. Phase id `PF03b`. Decisions: `backlog/DECISIONS-2026-10-21.md`, rows "SPAR delays", "SPAR licence
and manifest" and "SPAR follow-up phase" (2026-10-23). Read first `backlog/sprint-2026-10c/SPAR-discussion.md`
(facts, measurements, the decided options and "Notes for the follow-up item"), `backlog/sprint-2026-10c/PF03-REPORT.md`,
`doc/sparam.md`, `doc/reproducibility.md` ("Run manifest"). Phase 2 of F-03 (broadband) stays out of scope.

These items change simulation results of `SPAR` cases; the maintainer signed them off. Write your predictions
(which golden cases change, how, and why; counters too) in `PF03b-progress.md` **before** each re-record.

## Scope
1. **Per-path delayed copies** (`.tran`). On an input event at t with value a_i, for each output k reached from
   i, an internal arrival at t + tau_ki (tau of that path, at that wavelength); on arrival the delayed copy
   m_ki = a_i, and output k gets `sum_i S_ki m_ki` with zero delay. Writes to one output are then in time order.
   Steady state unchanged; `.op`/`.dc` unchanged. The dominant-path selection and the per-output delay go away.
   `delay=none` and `delay=<s>` keep their meaning (`<s>` per path). Implementation choice (one internal timed
   queue per device, or internal delayed ports per path) is yours: say why, and measure the cost (events per
   input event, memory) on the ring case. Coalescing and emission gating stay at the output port.
2. **Negative group delay**: still clamped to 0, now per path. Warning (once per path, `.tran` only, when at
   least half a time step): the path (port names), the wavelength, the value; when the value sits next to a grid
   point flagged by item 3, say it is probably a grid artefact; say that a clamped path behaves like a zero-delay
   connection inside a feedback loop.
3. **Grid-adequacy warning** at load time (tabulated formats; not the Taylor form): leave-one-out check as
   proposed in the discussion file (interpolate each interior grid point from its neighbours with the device's
   own interpolant, compare with the stored value). One warning per device naming the worst path and wavelength
   and the error; threshold around 0.01-0.02 in field (choose, justify, document). A smooth delay line must pass.
4. **Mixed interpolation near transmission nulls**: magnitude/phase as now where |S| is large; real/imaginary
   parts where |S| is small, decided **per interval** between grid points so S stays continuous. Choose the
   threshold from measurements and report them: the field error between grid points at the ring's through null
   (PF03 report: 0.020 linear / 0.015 PCHIP on a 0.05 nm grid, 0.025 nm off the grid) before and after, and the
   error elsewhere. The phase-convention tests (delay lines, <= 4e-13) must keep their bounds. Say what you do
   for `interp=pchip`. The group delay keeps coming from the phase as now. Item 3 uses the same interpolant.
5. **Data files in the run manifest**: a list of every data file the simulation reads (S-parameter files, VLSRC
   value files, any other file read by a device or source: search the code), with the path used to open it and
   its SHA-256, like `netlist_files`. Decide whether the key is always present (then every recorded manifest
   changes by that key only) and whether `format_version` changes; record why. Do the manifest change in **its
   own commit**, with only that change in the re-recorded files, so it can be checked mechanically. The F-02
   run table's `table.manifest.json` must also see a changed data file (a resume against another data file must
   not pass silently). Docs: `doc/reproducibility.md`.
6. **SiEPIC BDC fixture.** The coordinator downloaded the file and its upstream licence for you (your launch
   prompt names the directory: `EBeam_1550_TE_BDC.sparam`, `LICENSE.md`, `UPSTREAM_SHA`): repository
   github.com/SiEPIC/SiEPIC_EBeam_PDK, path `Lumerical_EBeam_CML/EBeam/source_data/bdc_TE_source/`, commit in
   `UPSTREAM_SHA`, MIT, "Copyright (c) 2016-2020, Lukas Chrostowski and contributors". You may copy exactly these
   two files into the repository (an exception to "Files outside the repository", approved by the maintainer):
   the data file unchanged, next to a `NOTICE` (source URL, path, commit, copyright, the MIT text). Then the brief's
   golden case: a `.dc` sweep of the split ratio (both outputs) over the file's range, with a check that the
   split is about 0.5 near 1.55 um.
7. Tests and docs: a two-input transient against an explicit circuit with paths of different delays (ring or
   MZI) for item 1; loader tests for item 3 (a null narrower than the grid warns, a delay line passes) and item 4;
   `doc/sparam.md` ("Behaviour in the simulation", delay, interpolation, the new warnings), `doc/netlist.md` (SPAR
   `delay`), `CHANGELOG.md` `[unreleased]`, the F-03 brief's status block.

**Not in scope:** re-exporting `tests/sparam/data/add_drop_ring.sparam` on a finer grid (the maintainer has not
decided; the coordinator sends a message if it is approved while you run).

## Acceptance
- `pixi run test gcc`, `pixi run test asan-clang`: all OK, no sanitizer message.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: only the cases you predicted change, each as
  predicted; the manifest commit changes manifests only.
- clang and asan-gcc: before/after `compare` (strict) shows the same set of changed cases.
- `stats-compare` (gcc) before/after: only predicted `SPAR` cases differ.
- Report `backlog/sprint-2026-10c/PF03b-REPORT.md` (items, decisions you took, measurements, golden and counter
  impact, found-not-done) committed on `ws/spar2`: that is the completion marker.
