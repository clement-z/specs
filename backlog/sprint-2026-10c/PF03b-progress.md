# PF03b progress (branch `ws/spar2`)

Prompt: `discover/orchestration/2026-10c/PF03b-spar-followups.md` (+ item 8 from the coordinator, 2026-10-23:
re-export `tests/sparam/data/add_drop_ring.sparam` on a 0.05 nm grid, own commit, after item 1).
Heartbeat: `94bb0c96` (PF03b-heartbeat), delete after the report is committed.
Base: 48e510f.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 1 | Per-path delayed copies | todo | | |
| 2 | Negative delay warning per path | todo | | |
| 3 | Grid-adequacy warning | todo | | |
| 4 | Mixed interpolation near nulls | todo | | |
| 5 | Data files in the run manifest | todo | | own commit |
| 6 | SiEPIC BDC fixture + golden | todo | | |
| 7 | Tests and docs | todo | | |
| 8 | Re-export ring file at 0.05 nm | todo | | own commit, after 1 |

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
