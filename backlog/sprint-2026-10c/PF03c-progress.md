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

### Item 2 (B-190): validation (Opus subagent, prototype `/tmp/PF03c-proto/scripts/proto.py`, results `RESULTS.md`)
Prototype bit-identical to the C++ (27624 `.dc` evaluations, 8 files, linear and PCHIP). MZI (two 50/50 couplers,
arms 300/400 um, ng 4.3, att 2 dB/cm, 0.2 nm grid) reproduces PF03b's numbers (0.0226 / 0.0160 / LOO 0.114).
Variants: B (endpoint rule on the delay-removed S), C (re/im everywhere), D/E (extra tests for a null between two
points: chord midpoint / segment distance), each with mag/phase on the detrended phase or as today ("s"); tau0 from a
LS fit, a |S|^2-weighted fit, the end-point mean, the median of 2-point differences, the median of `tau_s`.
**Chosen: "Es"**, tau0 = median of the entry's `tau_s` (0 if it rotates by < 1e-3 rad over the largest step),
f_ref = first grid point; re/im intervals: today's endpoint test or the segment [S~_k, S~_k+1] passing within 0.07
max|S| of 0; re/im of S~ there, rotated back; magnitude/phase intervals exactly as today (linear and PCHIP).
Worst field error between points (all / LOO), today -> Es:
- MZI linear 0.0226 -> 0.0016, LOO 0.114 -> 0.0064 (no warning); PCHIP 0.016 -> 0.016, LOO 0.109 -> 0.043 (warns).
- Ring 0.05 nm linear 9.7e-4 -> 5.4e-4 (|S| >= 0.1: 4.0e-4 both), LOO 0.0039 -> 0.0021; PCHIP 6.6e-4 -> 5.65e-4,
  LOO 0.0037 -> 0.0019.
- Delay line, BDC, synthetic dispersive lines, asym, gain: bit-identical (linear and PCHIP).
- Under-resolved: ring k=0.4 0.21 -> 0.044 (lin), 0.19 -> 0.052 (PCHIP); 1 nm ring 0.54 both; all still warn
  with today's values (0.43, 0.71, 0.707522 for the golden 11-point file).
Not chosen: PCHIP on the detrended phase ("Er" for PCHIP: MZI 0.0035, ring 2.7e-4, but the synthetic dispersive
line 6e-7 -> 1.9e-4 and the BDC LOO +3 %: a degradation of resolved paths, so the conservative choice); B (MZI
LOO stays 0.114, a warning on a grid that interpolates within 0.0016); C (dispersive line 7.5e-4 -> 0.046 and a
false warning); fits and the mean as tau0 (the pi steps at nulls bias them to one arm's delay, 4.3 ps vs 5.02 ps).

### Item 2, written before the re-record (from the prototype, `golden.py`)
- `cir/spar-add-drop-ring`: one value, `out` at 1.55 um (interval within 1e-9 of a grid point, re/im today):
  S(out,in) changes by 1.02e-12; output field d re +4.0e-13, d im -9.4e-13. Every other point bit-identical.
- `regress/spar-ring-tran` and the new `regress/spar-ring-tran-output`: `out` field at 1.55 um changes by ~1e-12
  (d re -1.02e-12, d im +5.4e-14); times, drop, stderr/stdout unchanged.
- `regress/spar-grid-warning`: `out` by ~4e-15; grid warning text unchanged (0.707522 at 1.546e-06 m).
- gtest case: changes if a test is added (yes: new SParamFile tests).
- Every other case bit-identical (wg, touchstone, BDC, passivity, uni-reflection, taylor, err cases). Counters
  unchanged (changes far below the gates).
