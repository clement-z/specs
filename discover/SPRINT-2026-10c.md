# Sprint 2026-10c — plan and status board (PREPARED, NOT LAUNCHED)

**Source of truth for this sprint.** Scope = `backlog/DECISIONS-2026-10-21.md`, sections "Follow-up decisions"
(d5890f1) and, once answered, "F-15 decisions" (to be added). Branch `sprint/2026-10c`, stacked on
`sprint/2026-10b` (local, nothing pushed). Coordinator: main session in `~/.paseo/worktrees/31cq0kv6/gigantic-donkey`.
Resume procedure, heartbeat and gate: as in `discover/SPRINT-2026-10b.md`, with the registry
`discover/orchestration/2026-10c/agents.json`.

Versions: **v0.2.0** = sprints 2026-10 + 2026-10b + phase A; **v0.3.0** = F-15 (phase D); v1.0.0 is reserved for the
journal publication. Merges into `dev`/`master`, tags and pushes only on an explicit maintainer request.

## Phases
| # | Phase | Items | Workspace / branch | Model | Done marker | Blocked on | State |
|---|---|---|---|---|---|---|---|
| A | Decided follow-ups (v0.2.0) | B-146 land; B-014 error option (b); 64-bit integer literals (overflow → parse error); zero tolerances allowed (≥ 0, warning when both 0) | `ws-followups` / `ws/followups` | Opus 5.5 xhigh | `backlog/sprint-2026-10c/PA-REPORT.md` | "go" | prepared |
| B | Sanitizer CI (gcc ASan/UBSan) green | reproduce locally with gcc (runtimes now installed), fix (suppression or code), keep the job blocking | coordinator | Opus 5.5 | CI run green on all jobs | "go" | prepared |
| C | v0.2.0 release prep | CHANGELOG `[0.2.0]`, version `0.2.0` in `meson.build`, `CITATION.cff` citing the ECOC paper (no DOI per version) | coordinator (or Sonnet) | Opus 5.5 | release-prep commit; **stop before tag/merge** | A, B | prepared |
| D1 | F-15 part 1: `--stats` + ablation switches | counters, JSON next to the trace, documented expert switches; no output change otherwise | `ws-stats` / `ws/stats` | Opus 5.5 xhigh | `PD1-REPORT.md` | C | prepared (answers recorded, Q8 provisional) |
| D2 | F-15 part 2: bit-identical refactors | squared-norm gate, per-λ S-parameter caches, single `sc_event` per port (heap order kept), `OpticalSignal` id behind a flag | `ws-engine2` / `ws/engine2` | Opus 5.5 xhigh | `PD2-REPORT.md` | D1 | prepared (answers recorded, Q8 provisional) |
| D3 | F-15 part 3: `SC_METHOD` conversion | output port + stateless device reactions | `ws-methods` / `ws/methods` | Opus 5.5 xhigh | `PD3-REPORT.md` | D2 | prepared (answers recorded, Q8 provisional) |
| D4 | F-15 measurement (v0.3.0 candidate) | before/after per-event cost on the CROW benchmark (`discover/scout/bench/`); ratios only (shared host) | coordinator | Opus 5.5 | numbers in the D reports | D3 | draft |
| E | Private validation runs (optional) | instructions kept outside the repository at the maintainer's request | coordinator | Opus 5.5 | outside the repo | A | prepared (outside repo) |

Why this order: A changes accepted inputs (zero tolerances) that D4 measures with; B and C make v0.2.0 releasable
before the engine changes start; inside F-15, `--stats` lands first so that D2 and D3 can be gated on event counts
as well as outputs.

## Gate before every merge (unchanged from 2026-10b)
Fresh gcc and clang builds, clang ASan+UBSan build (leak detection on, `tests/sanitizers/lsan.supp`), `meson test`,
golden `--atol 0` (strict order and coverage by default), audit that every changed baseline file maps to the item
that predicted it, diff scope limited to the phase. For D2/D3 additionally: outputs bit-identical
(`--rtol 0 --atol 0 --print-slack 0`) and `--stats` counters compared per case (criterion per the F-15 answers).

## Expected golden impact of phase A (predictions the gate checks)
| Item | Existing cases that change | New cases |
|---|---|---|
| B-146 | `cir-json/subckt_tests` (`flat.json` `writers` of 2 nets 0 → 1) | the 5 cases of `P2-ready/b146` |
| B-014 (b) | none (`regress/b031-mzi-bidir` keeps passing at the default timescale; its device-internal arms warn) | netlist waveguide rounding to 0 → error; device-internal rounding → warning only |
| 64-bit integers | `err/f01-options-seed-huge-integer` (new parse-error message); `err/f01-options-seed-int-max` becomes a passing case (the P4 guard is removed) | `.options seed=3000000000`; literal beyond 64 bits in a kwarg, a positional argument and a net name; a narrow consumer out of range |
| Zero tolerances | `err/b153-reltol-zero` becomes a passing case (warning only if abstol is also 0) | open loop at zero tolerance; lossy ring `.tran` at zero tolerance; both zero → one warning; negative values still errors |

## Pending maintainer input
- **F-15**: Q2 and Q5 confirmed; Q8 (equal-time order) pending the maintainer's review of the code excerpt and diagram.
- Unpushed local commits on `sprint/2026-10b` (bca9c50, d5890f1) and everything on `sprint/2026-10c`: push only on request.
- Housekeeping of the 2026-10b phase workspaces and `ws/*`, `P*-wip/*` branches.

## Log
- 2026-10-21 — sprint prepared (board, prompts, heartbeat registry); not launched ("no go, just prepare").
- 2026-10-21 — maintainer answers recorded (e6c443e): sanitizer runtimes installed, no DOI per version (ECOC paper stays the citation), 64-bit integer literals, F-15 Q1–Q8. Prompts updated; still no go.
- 2026-10-21 — Q2, Q5 confirmed; Q8 under review. Push of `sprint/2026-10b` and `sprint/2026-10c` and housekeeping of the 2026-10b workspaces authorised.
