# Sprint 2026-10b — plan and status board

**Source of truth for this sprint.** Scope = `backlog/DECISIONS-2026-10-21.md` (nothing outside it). Branch
`sprint/2026-10b`, stacked on `sprint/2026-10` (PR #16). Coordinator: main session in
`~/.paseo/worktrees/31cq0kv6/gigantic-donkey`. If a session is interrupted, resume from this file (§ Resume).

## Phases (sequential; each = one paseo workspace branched from the current `sprint/2026-10b`)
| # | Phase | Items | Workspace / branch | Model | Done marker (committed on its branch) | State |
|---|---|---|---|---|---|---|
| P1 | Golden harness hardening | B-126, B-127, B-128, B-129 | `ws-harness` / `ws/harness` (agent 6e49bae6) | Opus 5.5 high | `backlog/sprint-2026-10b/P1-REPORT.md` | done, merged (d214196) |
| P2 | Netlist/engine correctness | B-144/145/147, B-146, B-052, B-001, B-002/004, B-058, B-051, B-060 (validation), B-037 (docs) | `ws-engine` / `ws/engine` (agent 062bddfe) | Opus 5.5 xhigh orch. | `backlog/sprint-2026-10b/P2-REPORT.md` | done, merged (cdacdca); B-146 awaits sign-off |
| P3 | Devices/physics | B-032/033/159 (crossing), B-049/050 (PDET), B-048 (probes, IC-proof), B-021/B-014/B-124 (tolerance docs + delay warnings) | `ws-devices` / `ws/devices` (agent a8c73097) | Opus 5.5 xhigh orch. | `backlog/sprint-2026-10b/P3-REPORT.md` | running |
| P4 | F-01 reproducible runs (fixes B-016) | seeds, name-ordered iteration, manifest | `ws-repro` / `ws/repro` | Opus 5.5 xhigh | `backlog/sprint-2026-10b/P4-REPORT.md` | pending |
| P5 | F-28 + F-12 | `.op`/`.dc` convergence; netlist pack (SPICE semantics) | `ws-netlist` / `ws/netlist` | Opus 5.5 xhigh orch. | `backlog/sprint-2026-10b/P5-REPORT.md` | pending |
| P6 | F-05 outputs/tracing | `.save`, `--no-trace`, CSV/`.npz`, trace-all fallback | `ws-outputs` / `ws/outputs` | Opus 5.5 high | `backlog/sprint-2026-10b/P6-REPORT.md` | pending |
| P7 | CI: sanitizer job blocking | after P4 removes the ordering noise | coordinator | Opus 5.5 | CI green with blocking sanitizer job | pending |
| P8 | Final docs + verification + push | cheatsheet/README/CHANGELOG, full re-verification, push branch | coordinator | Opus 5.5 | pushed `origin/sprint/2026-10b` | pending |

Why sequential: P2–P6 all re-record golden outputs and touch `parse_element.cpp`/`specs.cpp`; one phase at a time
keeps every baseline change attributable and avoids manifest/merge conflicts. P1 goes first because it changes the
comparison rules everything else is checked against.

## Rules for every phase (copied into each prompt)
1. Only items listed for the phase, implemented as decided in `DECISIONS-2026-10-21.md`. Anything else → note it, don't do it.
2. One commit per item (`fix(B-NNN): …` / `feat(F-NN): …`), regression test first where feasible.
3. Golden: every re-recorded case must be predicted by the decision for that item and cited in the commit
   (`--note "B-NNN"`); an unexpected output change stops that item (→ report, `needs-human`).
4. Build with ccache (no `CC`/`CXX` env; deps `-Dcmake_prefix_path=$HOME/.local/opt/specs-deps-next`).
5. Commit progress at least after every item; keep `backlog/sprint-2026-10b/Pn-progress.md` current so an
   interruption loses nothing. Never push. Implementer worktrees under `/tmp`, never `isolation: worktree`.
6. Update the issue files (`status:`), `backlog.md`, docs and `CHANGELOG.md` for user-visible changes.

## Coordinator gate before merging a phase
Fresh gcc and clang builds; `meson test`; golden `--atol 0 --strict` against the phase's baseline; audit that
every changed baseline file maps to an approved item; diff scope limited to the phase's items; then
`git merge --no-ff` into `sprint/2026-10b` and update this board.

## Heartbeats (survive usage limits)
- Coordinator: paseo heartbeat `924bc5b5` (every 5 min, expires after 96 h) runs `discover/orchestration/heartbeat.py`,
  which resumes limit-stopped phase agents itself and reports DONE/ATTENTION lines to the coordinator.
  Registry of watched agents: `discover/orchestration/2026-10b/agents.json` (update when launching a phase).
- Each phase orchestrator creates its own 5-min heartbeat (rule in `COMMON.md`) and deletes it after its report.
- Delete the coordinator heartbeat in P8 (`paseo heartbeat delete 924bc5b5`).

## Resume (after a session limit or a new session)
1. Read this board and `backlog/DECISIONS-2026-10-21.md`.
2. `paseo ls` → running/idle phase agent; check its branch for the done marker and `Pn-progress.md`.
3. If the agent stopped mid-phase: `paseo send --no-wait <id> "<resume, re-launch subagents, commit progress>"`.
4. Re-arm the watcher: `python3 discover/orchestration/watch_agents.py <repo> <agent>:<branch>:<done marker>`.

## Decided open point
- Sanitizer job blocking (P7): **option a** — LSan suppression file limited to leaks at `exit()` on error paths and
  allocations owned by SystemC thread stacks; leak detection stays on otherwise.

## Pending maintainer sign-offs
- **B-146** (P2): fix ready on `P2-ready/b146` @ 8129020; the only unpredicted golden change is `cir-json/subckt_tests` `flat.json`: `writers` of `ROOT/out` and `ROOT/X2/N00001` 0 → 1 (flat-equivalent count; JSON export only, no simulation change). If approved: cherry-pick onto `sprint/2026-10b` and set B-146's status.

## Log
- 2026-10-21 — decisions recorded (c4597b9); plan written; sanitizer option a chosen; awaiting "go".
- 2026-10-21 — go; P1 launched (agent 6e49bae6, ws/harness).
- 2026-10-21 — coordinator heartbeat 924bc5b5 (5 min) + heartbeat.py; phase agents set their own heartbeats.
- 2026-10-21 — P1 merged (d214196): gate = no C++ change, 30 baseline files ID-only, gcc+clang meson test 12/12, golden 81/81 `--atol 0`, swapped line fails. P2 launched (agent 062bddfe). Note: phase agents' own heartbeats are denied by their permission classifier; the coordinator heartbeat resumes them.
- 2026-10-21 — P2 merged (cdacdca): gate = scope OK, gcc+clang meson test 12/12, golden 115/115 `--atol 0`, old baseline + predicted edits == new baseline. B-146 parked for sign-off. P3 launched (agent a8c73097).
