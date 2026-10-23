# PF03c progress (branch `ws/spar3`)

Prompt: `discover/orchestration/2026-10c/PF03c-spar-options.md`. Decisions: `backlog/DECISIONS-2026-10-21.md`, row
"SPAR follow-ups review" (2026-10-23).
Heartbeat: `2aab3687` (PF03c-heartbeat).
Base: 30f4fcd. Before-records (outside the repo): `/tmp/PF03c-before/gcc` (golden outputs), `/tmp/PF03c-before/stats-gcc`
(counters); base builds of clang and asan-gcc in the worktree `/tmp/PF03c-wt/base`; a gcc build of 48e510f
(pre-PF03b) in `/tmp/PF03c-wt/old`. Base check: `pixi run test gcc` 16 OK, 1 skipped (run-table, B-193);
strict golden 275 passed.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 1 | Per-path delays as an option | done | 7e044eb | `delay_per="path"|"output"`; vs 48e510f: same timing (below); stress cost below; asan-clang clean |
| 2 | B-190: validate, then fix | validation running (Opus subagent, prototype in /tmp/PF03c-proto) | | |
| 3 | Stale comments in two golden netlists | done | (this commit) | manifests: netlist hash only |
| 4 | Docs | todo | | |

## Predictions (written before each re-record)

### Item 1 (delay_per="path"|"output"), written before recording
- Default (`delay_per="path"`): every existing case unchanged, bit for bit, counters unchanged (checked: strict golden
  275 passed with the item built).
- New case `regress/spar-ring-tran-output` (the netlist of `regress/spar-ring-tran` with `delay_per="output"`, 0.05 nm
  file, only `in` lit at 1.55 um): the largest path into `out` is add->out (|S| ~0.997, vs 0.0023 for in->out), into
  `drop` in->drop; both have the drop path's delay, +1.721 ps. So `out` and `drop` both step at 2.721 ps and 21.721 ps
  (fields as in `regress/spar-ring-tran`: out 0.0023-ish, drop ~0.997); no negative-delay warning (the dominant paths'
  delays are positive); stdout (`--vci`) as `regress/spar-ring-tran` plus one line `delay_per = output` after
  `delay = auto`. Counters: as `regress/spar-ring-tran` except `out`'s `last_emission_s` 21.721 ps (instead of 20 ps).
Result: as predicted (out and drop at 2.721 / 21.721 ps, same fields as the per-path case, stderr empty, stdout +1 line;
counters: only out's `last_emission_s`). gcc 16 OK 1 skipped; strict golden 276 passed; asan-clang 16 OK 1 skipped, no
sanitizer message; stats-compare vs base: 274 same, 1 new.

Item 1 measurements (scripts and runs in /tmp/PF03c-item1):
- Against a gcc build of 48e510f (no keyword) vs HEAD with `delay_per="output"`, `.tran`, two lit inputs with steps,
  each with `delay=auto`, `"none"`, `2e-12`: the skew device of test_sparam.py (two waveguides into a coupler, delays
  1.4 / 4.2 ps; exported on 0.5 nm) at 1.55 and 1.5503 um, and the 1 nm ring file of 48e510f at 1.5525 um: traces
  **byte-identical** (9 runs). The ring at 1.551, 1.5505, 1.55 um (intervals next to the 1 nm grid's through null,
  where PF03b's mixed interpolation applies): the same rows at the same times, fields differ (4.7e-13, 0.094, 3.7e-13:
  the interpolation, not the timing). stderr differs: the grid warning (PF03b) and the negative-delay text.
- Stress (ring 0.05 nm file, `in` and `add` lit, 2 x 200000 values 0.25 ps apart, 4 power levels, timescale -15,
  gcc release, 7 runs each, user time median): at 1.55 um (2 of the 4 paths delayed) path 0.96 s, output 0.91 s; at
  1.552 um (4 paths delayed) path 1.04 s, output 0.94 s. Peak RSS ~31 MB both. Port writes 1150033 both; emitted
  1070976 / 1070825 (1.55 um), 1125048 both (1.552 um); pending_max 1 (path) vs 14 / 5 (output). PF03b measured
  1.02 -> 1.63 s on its own stress case (not recorded); my stimulus repeats values more often.

### Item 3 (stale netlist comments), written before the re-record
- `regress/spar-ring-tran`, `cir/spar-add-drop-ring`: only the netlist's SHA-256 in `traces/delete_me.manifest.json`
  (the run manifest records path and hash), hence that file's hash in `case.json`. Traces, stdout, stderr unchanged.
  No other case. Counters unchanged.
Result of the strict check: as predicted (the two cases, `netlist_files[0].sha256` only).
