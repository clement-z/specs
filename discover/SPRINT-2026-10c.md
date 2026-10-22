# Sprint 2026-10c — plan and status board (A and B launched 2026-10-21)

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
| A | Decided follow-ups (v0.2.0) | B-146 land; B-014 error option (b); 64-bit integer literals (overflow → parse error); zero tolerances allowed (≥ 0, warning when both 0) | `ws-followups` / `ws/followups` (agent f8d8912f) | Opus 5.5 xhigh | `backlog/sprint-2026-10c/PA-REPORT.md` | — | **done**, merged (591b877) |
| B | Sanitizer CI (gcc ASan/UBSan) green | reproduce locally with gcc (runtimes now installed), fix (suppression or code), keep the job blocking | coordinator | Opus 5.5 | CI run green on all jobs | — | **done**: 8af20cc (B-181); CI run 39624029371 green on all jobs |
| C | v0.2.0 release prep | CHANGELOG `[0.2.0]`, version `0.2.0` in `meson.build`, `CITATION.cff` citing the ECOC paper (no DOI per version) | coordinator (or Sonnet) | Opus 5.5 | release-prep commit; **stop before tag/merge** | A, B | done (59c0362); tag/merge/push await the maintainer |
| C2 | SystemC as a Meson subproject (v0.2.0) | `thirdparty/` → `subprojects/`; SystemC 3.0.2 and args 6.4.7 from wraps (submodules removed); subproject by default, installed SystemC on request; CI, `install.sh`, docs, CHANGELOG (decisions, third round) | `ws-subprojects` / `ws/subprojects` | Opus 5.5 high | `backlog/sprint-2026-10c/PC2-REPORT.md` | — | **done**, merged (4212520) |
| F | pixi development environment (after v0.2.0) | `pixi.toml` + `pixi.lock` (compilers, build tools, pyspecs and docs features, tasks), shared ccache/wrap cache across worktrees, README/CLAUDE.md, CI pixi job (Ubuntu blocking, macOS non-blocking) (decisions of 2026-10-22) | `ws-pixi` / `ws/pixi` | Opus 5.5 high (+ Sonnet workers) | `backlog/sprint-2026-10c/PF-REPORT.md` | — | launched |
| D1 | F-15 part 1: `--stats` + ablation switches | counters, JSON next to the trace, documented expert switches; no output change otherwise | `ws-stats` / `ws/stats` | Opus 5.5 xhigh | `PD1-REPORT.md` | C | prepared (F-15 answers confirmed) |
| D2 | F-15 part 2: bit-identical refactors | squared-norm gate, per-λ S-parameter caches, time-only ordered pending events + single `sc_event` per port, `OpticalSignal` id behind a flag | `ws-engine2` / `ws/engine2` | Opus 5.5 xhigh | `PD2-REPORT.md` | D1 | prepared (F-15 answers confirmed) |
| D3 | F-15 part 3: `SC_METHOD` conversion | output port + stateless device reactions | `ws-methods` / `ws/methods` | Opus 5.5 xhigh | `PD3-REPORT.md` | D2 | prepared (F-15 answers confirmed) |
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

## Log
- 2026-10-21 — sprint prepared (board, prompts, heartbeat registry); not launched ("no go, just prepare").
- 2026-10-21 — maintainer answers recorded (e6c443e): sanitizer runtimes installed, no DOI per version (ECOC paper stays the citation), 64-bit integer literals, F-15 Q1–Q8. Prompts updated; still no go.
- 2026-10-21 — Q2, Q5 confirmed; Q8 under review. Push of `sprint/2026-10b` and `sprint/2026-10c` and housekeeping of the 2026-10b workspaces authorised.
- 2026-10-21 — pushed `sprint/2026-10b` (d5890f1) and `sprint/2026-10c`; 2026-10b workspaces archived, `ws/*` and `P*-wip/*` branches and `/tmp` scratch removed (`P*-ready/*` kept for phase A).
- 2026-10-21 — Q8 decided: order by time only (same-time order unspecified, deterministic). All F-15 answers confirmed. Waiting for "go" on A/B.
- 2026-10-21 — backlog B-179 recorded (PUF circuit ring coupling to align before benchmark use; needs-human, decision pending); integer-clamp entry of phase A renumbered B-180.
- 2026-10-21 — **go for A and B**. A launched (agent f8d8912f, `ws/followups`); B started by the coordinator; coordinator heartbeat ac1c0a21 (5 min, 96 h, registry 2026-10c). C and D not approved yet.
- 2026-10-21 — B: reproduced with gcc 11 ASan+UBSan; one cause in all 146 failing golden cases: `Variable` copied an indeterminate bool (UB, parse_tree.h:277). Fixed in 8af20cc (B-181, member initialisers; outputs bit-identical); gcc ASan meson test 12/12 locally; pushed, CI run pending. Phase A agent told to merge sprint/2026-10c before widening `inum`.
- 2026-10-21 — B done: GitHub CI run 39624029371 on 8af20cc green on all three jobs (gcc, clang, blocking gcc ASan/UBSan).
- 2026-10-21 — A merged (591b877): gate = gcc, clang, gcc ASan, clang ASan meson test 12/12 (no sanitizer message), golden 212/212 `--atol 0`; existing-case changes as predicted + `err/b153-abstol-negative` message (`>= 0`). Backlog B-182 recorded (subckt-body parse error line numbers). Phase A agent archived, `ws/followups`, `PA-wip/*`, superseded `P*-ready/*` deleted; heartbeat ac1c0a21 deleted (no agent running). Next: C (v0.2.0 prep) waits for the maintainer.
- 2026-10-21 — GitHub CI run 39631546461 on aefd5a3 (A merged): green on all three jobs (gcc, clang, blocking gcc ASan/UBSan).
- 2026-10-21 — **go for C**. C done by the coordinator (59c0362): version 0.2.0, CHANGELOG `[0.2.0] - 2026-10-21` (New / Changed / Fixed / Removed), `CITATION.cff` (validated, CFF 1.2.0; ECOC 2023 as preferred-citation, no DOI), README pointer. Gate: gcc, clang, gcc ASan, clang ASan meson test 12/12 (no sanitizer message), golden 212/212 `--atol 0` (ASan builds with the LSan suppressions, as `meson test` applies them). Not pushed; commit to tag: 59c0362 (`v0.2.0`). D not approved yet.
- 2026-10-21 — maintainer: hold the push; SystemC as a Meson subproject by default, `thirdparty/` → `subprojects/`, wraps instead of submodules (decisions, third round; lands in v0.2.0 before the tag). CLAUDE.md fix c022c4d (local paths) to be superseded by C2. Phase C2 launched: agent 899f3d3e (`ws/subprojects`, Opus high); coordinator heartbeat 4632a24d (5 min, 96 h).
- 2026-10-22 — C2 merged (4212520): gate in a fresh clone without any prefix (SystemC 3.0.2 and args from the wraps): gcc, clang, gcc ASan, clang ASan meson test 12/12, no sanitizer message, golden 212/212 `--atol 0` bit-identical, `specs` linked to the subproject's `libsystemc.so.3.0`; diff reviewed against the prompt (agent checks: Meson 1.3.0, offline, opt-in installed SystemC, ABI rejection, install + rpath, CI YAML walked through). Agent 899f3d3e archived, heartbeat 4632a24d deleted. Release candidate: tip of `sprint/2026-10c`; push, tag `v0.2.0` and merges await the maintainer.
- 2026-10-22 — v0.2.0 release candidate = 6c7514a (C2 merged + board); the tag goes there, before phase F. Maintainer: go for phase F (pixi dev environment, decisions of 2026-10-22: compilers in pixi, CI job next to the apt jobs, after v0.2.0). Agent 36b603a0 (`ws/pixi`, Opus high + Sonnet workers); coordinator heartbeat d0cec0b3 (5 min, 96 h).
