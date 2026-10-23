# PF03b progress (branch `ws/spar2`)

Prompt: `discover/orchestration/2026-10c/PF03b-spar-followups.md` (+ item 8 from the coordinator, 2026-10-23:
re-export `tests/sparam/data/add_drop_ring.sparam` on a 0.05 nm grid, own commit, after item 1).
Heartbeat: `94bb0c96` (PF03b-heartbeat), delete after the report is committed.
Base: 48e510f.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 1 | Per-path delayed copies | done | 7e9e03b | clang/asan-gcc: same 2 cases; asan-clang clean |
| 2 | Negative delay warning per path | todo | | |
| 3 | Grid-adequacy warning | done | 964b75a | threshold 0.02 (leave-one-out); no existing case changed; new case regress/spar-grid-warning |
| 4 | Mixed interpolation near nulls | done | 368bf61 | threshold 0.07 x max|S|; measurements in the report |
| 5 | Data files in the run manifest | done | 805dc8c | subagent (opus) in /tmp/PF03b-wt/manifest, reviewed, cherry-picked; ring manifests re-recorded for item 8's file |
| 6 | SiEPIC BDC fixture + golden | todo | | |
| 7 | Tests and docs | todo | | |
| 8 | Re-export ring file at 0.05 nm | done | 5e1b14f | predictions above; counters: only spar-ring-tran drop last_emission |

## Predictions (written before each re-record)

### Item 1 (per-path delayed copies), written before the re-record
Expected from the design (SPAR-discussion.md, "Notes for the follow-up item"), then checked by a first strict run
(the row order of `cir/spar-touchstone-tran` was seen in that run, not predicted beforehand):
- `.op`/`.dc` cases (`cir/spar-wg-dc`, `cir/spar-add-drop-ring`, `regress/spar-3port-taylor`, `regress/spar-passivity`,
  `regress/spar-uni-reflection`, the `err/spar-*` cases): unchanged (no delay outside event-driven ports; the
  zero-delay paths copy and write at once, as before).
- `regress/spar-ring-tran` (1.55 um, only `in` lit, 1 nm file): `out` was written with the delay of the dominant
  path into it (the unlit add->out, +1.324 ps); now with the delay of in->out, whose finite-difference group delay at
  the null is -0.6775 ps, clamped to 0: `out` steps at 1 ps and 20 ps instead of 2.324 / 21.324 ps (same fields).
  `drop` unchanged (in->drop was already the dominant path, +1.324 ps). stderr gains the negative-delay warning
  (old text, once per device: the in->out delay is now computed; before, only the dominant path's was).
  Counters: only `ports.ROOT/SPAR1.p2_out_delayed_writer.last_emission_s` 21.324 -> 20 ps.
- `cir/spar-touchstone-tran` (one path per output): same times and same fields, bit for bit. The SPAR output is now
  written one delta cycle later (the arrival fires at t + tau, then the output is written without delay), so in
  the CSV the rows of MLPROBE1 (WG) and MLPROBE2 (SPAR) at the same time swap (4 pairs of rows). Counters unchanged.
- Every other case: unchanged, counters unchanged.

### Item 8 (ring file re-exported on a 0.05 nm grid), written before the re-record
Facts measured first: the export reproduces the committed 1 nm file byte for byte; the 0.05 nm export has 601
points from 1.535e-06 to 1.5649999999999386e-06 m (SPECS's `.dc` sweep accumulates its wavelengths, so they differ
in the last bits from those of a 1 nm sweep). Of the 31 wavelengths of the 1 nm sweep (the grid of the old file
and the sweep of `cir/spar-add-drop-ring`), only the first (1.535 um) is bit-identical to a point of the new grid;
the other 30 are 2e-14 to 4e-14 (relative) away from the nearest point, i.e. ~1e-9 of an interval.
Only two golden cases use the file (`cir/spar-add-drop-ring`, `regress/spar-ring-tran`); their netlists stay unchanged.
- `cir/spar-add-drop-ring`: **not bit-identical**. At 1.535 um the value is the file's value, computed by the same
  `.dc` of the same circuit at the same wavelength: identical. At the other 30 points the device interpolates
  between two grid points ~1e-9 of an interval from one of them, whose value was computed ~5e-20 m away:
  |dE| <= |dS/dlambda| * 6e-20 m, about 1e-11 away from resonance and up to ~1e-10 near the through null
  (slope up to ~1e9 /m there). Traces change in the last digits (well below the netlist's reltol 1e-6); the
  `.dc` default tolerances of run_golden pass. stderr unchanged (empty); stdout unchanged.
- `regress/spar-ring-tran` (1.55 um, 1 fs step): the source wavelength is also ~2e-14 off a grid point, so the
  fields change in the last digits as above. Delays: in->drop goes from 1.324 ps (finite differences on the 1 nm
  grid, 23 % low) to ~1.72 ps (exact 1.72 ps, discussion table): `drop` steps at ~2.72 ps and ~21.72 ps instead of
  2.324 / 21.324 ps. in->out stays negative (FD -37 ps on the 0.05 nm grid at the null; -0.68 ps before): clamped
  to 0, `out` still steps at 1 ps and 20 ps; the warning's value changes from -6.775e-13 s to about -3.7e-11 s.
  stdout (`--vci`) unchanged (range printed with 6 digits). Counters: drop's `last_emission_s` 21.324 -> ~21.72 ps.
- No other case changes.
Result of the strict check (before the re-record): as predicted (drop at 2.721 / 21.721 ps, warning -3.73047e-11 s,
out at 1 / 20 ps, field changes <= 4.3e-11 absolute in both cases). One statement of the prediction was wrong:
the default tolerances (rtol 1e-9, atol 1e-12) do not pass at one point, `out` at 1.55 um in
`cir/spar-add-drop-ring`, where |E| = 0.002: the 4.3e-11 change is 2e-8 relative there.

### Item 4 (mixed interpolation, threshold 0.07 x the entry's largest |S|), written before the strict run
Values at grid points are unchanged bit for bit (magnitude and phase there, as before); between grid points only
intervals with min(|S_k|, |S_k+1|) < 0.07 max|S| change; group delays unchanged (from the phase table).
- Delay-line and constant fixtures (`cir/spar-wg-dc`, `cir/spar-touchstone-tran`, `regress/spar-passivity`,
  `regress/spar-uni-reflection`) and the Taylor form (`regress/spar-3port-taylor`): unchanged (constant |S|).
- `cir/spar-add-drop-ring`: only the sweep point at 1.55 um changes, on `out` (S(out,in) = 0.00228 there; its
  neighbours on the 0.05 nm grid are below 0.07): the point is ~1e-9 of an interval off the grid point, so the
  two interpolants differ by ~1e-9 x |dS| ~ 1e-11 or less. The other through nulls (1545.90, 1554.10, ... nm)
  are >= 0.1 nm from the 1 nm sweep points, where |S| >= 0.1: unchanged. `drop`: unchanged (S(drop,in) has no null).
- `regress/spar-ring-tran`: `out` at 1.55 um changes by ~1e-11 or less (same reason); times unchanged; drop unchanged.
- Counters: unchanged everywhere (same events, values change by far less than the gates).
- Every other case unchanged.
Result of the strict check: as predicted, only `out` at 1.55 um in the two ring cases; the change is up to 4.2e-11
(I had estimated ~1e-11 or less). The new values are closer to the exact ones (the 1 nm file's grid value there).
The gtest case also changes (one new test, SParamFile.MixedInterpolationNearANull).

### Item 5 (data files in the run manifest)
Decisions: key `data_files` always present (`[]`), right after `netlist_files`; `format_version` 1 -> 2 in the run
manifest and the run table manifest. Every recorded manifest changes in those two places only (160 cases); the
non-empty lists: the 8 SPAR cases and b052-vlsrc/evlsrc-positional, b066-vlsrc-vci. Checked mechanically.

### Item 3 (grid-adequacy warning, threshold 0.02)
Prediction (from the prototype's leave-one-out values of every committed fixture: <= 0.0039): no existing case
changes. (The strict run was made right after the code change, before this note was written: 273 passed.)
Re-recorded: the gtest case (new test SParamFile.GridCheckFlagsANullNarrowerThanTheGrid) and the new case
`regress/spar-grid-warning` (1 nm ring export, 11 points: expected warning on the worst used path, from the
prototype on the 31-point file: ~0.7 at the through null near 1.55 um or 1.546 um; .tran: out at +0 with the old
negative-delay message, drop at the 1 nm grid's in->drop delay).
