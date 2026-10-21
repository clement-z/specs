# Sprint 2026-10b — plan and status board

**Source of truth for this sprint.** Scope = `backlog/DECISIONS-2026-10-21.md` (nothing outside it). Branch
`sprint/2026-10b`, stacked on `sprint/2026-10` (PR #16). Coordinator: main session in
`~/.paseo/worktrees/31cq0kv6/gigantic-donkey`. If a session is interrupted, resume from this file (§ Resume).

## Phases (sequential; each = one paseo workspace branched from the current `sprint/2026-10b`)
| # | Phase | Items | Workspace / branch | Model | Done marker (committed on its branch) | State |
|---|---|---|---|---|---|---|
| P1 | Golden harness hardening | B-126, B-127, B-128, B-129 | `ws-harness` / `ws/harness` | Opus 5.5 high | `backlog/sprint-2026-10b/P1-REPORT.md` | pending |
| P2 | Netlist/engine correctness | B-144/145/147, B-146, B-052, B-001, B-002/004, B-058, B-051, B-060 (validation), B-037 (docs) | `ws-engine` / `ws/engine` | Opus 5.5 xhigh orch. | `backlog/sprint-2026-10b/P2-REPORT.md` | pending |
| P3 | Devices/physics | B-032/033/159 (crossing), B-049/050 (PDET), B-048 (probes, IC-proof), B-021/B-014/B-124 (tolerance docs + delay warnings) | `ws-devices` / `ws/devices` | Opus 5.5 xhigh orch. | `backlog/sprint-2026-10b/P3-REPORT.md` | pending |
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

## Resume (after a session limit or a new session)
1. Read this board and `backlog/DECISIONS-2026-10-21.md`.
2. `paseo ls` → running/idle phase agent; check its branch for the done marker and `Pn-progress.md`.
3. If the agent stopped mid-phase: `paseo send --no-wait <id> "<resume, re-launch subagents, commit progress>"`.
4. Re-arm the watcher: `python3 discover/orchestration/watch_agents.py <repo> <agent>:<branch>:<done marker>`.

## Open point
- Sanitizer job blocking (P7): with leak detection on, it cannot become clean (leaks at `exit()` on error paths;
  SystemC never unwinds thread stacks). Awaiting maintainer choice (suppression file vs no leak check).

## Log
- 2026-10-21 — decisions recorded (c4597b9); plan written; awaiting "go".
