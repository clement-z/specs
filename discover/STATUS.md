# Sprint 2026-10 — status board

Coordinator: main session (Opus 5.5) in `~/.paseo/worktrees/31cq0kv6/gigantic-donkey`, branch `sprint/2026-10`
(based on local `dev` @ 252a7f4). Nothing is pushed; all merges are local into `sprint/2026-10`.

## Environment (shared by all worktrees)
- Deps prefix: `~/.local/opt/specs-deps` — SystemC 3.0.1 (C++20, Release, CMake config only, no `.pc`) + taywee/args.
- meson 1.11.2 at `~/.local/bin/meson`; ninja, gcc 11.5, clang, flex, bison, doxygen system-wide.
- Discovery skill: `~/.claude/skills/codebase-onboarding/` (from affaan-m/everything-claude-code, reviewed).

## Phases
| # | Phase | Where | Model | State |
|---|-------|-------|-------|-------|
| 0a | Sprint branch, deps, meson, skill | here | Opus 5.5 | done |
| 0b | Paper → `doc/papers/`, README citation | here | Opus 5.5 | done (26bf92c) |
| 0c | Discovery (5 probes) → `discover/0*.md` | here (subagents) | Opus 5.5 | done |
| 0d | Golden-output baseline → `tests/golden/` (45 cases) | here (probe 4) | Opus 5.5 | done, independently re-verified |
| 0e | Onboarding guide + CLAUDE.md | here | Opus 5.5 | done |
| 1a | Meson migration (replace CMake + Make, GitLab CI + GitHub Actions) | paseo `ws-meson` (agent 7eac146d, branch `ws/meson`) | Opus 5.5 high | done, re-verified, merged (b86a745) |
| 1b | In-depth review → `backlog.md` + `backlog/` | paseo `ws-review` (agent ca28f59e, branch `ws/review`) | Opus 5.5 xhigh orch. | done, merged |
| 1c | Feature scout → `discover/scout/` | paseo `ws-scout` (agent b2f5bd43, branch `ws/scout`) | Fable 5.1 xhigh | done, merged (7787aed) |
| 2 | Easy fixes (unattended) | paseo `ws-fixes` (branch `ws/fixes`) | Opus 5.5 xhigh orch. | running |
| 3 | Easy cleanup | paseo `ws-cleanup` | Opus high → Sonnet → Opus | pending |

## Rules for all workstreams
- Every behavioural change is validated with `tests/golden/run_golden.py check`.
- Physics/numerics changes are never "easy": tag `needs-human` in the backlog.
- One commit per backlog ID for fixes (`fix(B-NNN): ...`).
- No `git push`, ever.

## Log
- 2026-10-20 — sprint started; deps built; paper committed.
- 2026-10-20 — discovery done (5 reports, ~100 backlog seeds). Golden harness: 44 specs cases + gtest; fresh
  rebuild passes with `--atol 0`; a perturbed wrapper fails. Known: in-tree `unit_tests` doesn't compile
  (baseline gtest recorded with `discover/meson/unit-tests-minimal.patch`); `tb/mesh` segfaults (recorded as-is);
  detector traces read uninitialised memory at t=0 (4 signals excluded).
- 2026-10-20 — phase 1 launched: ws-meson, ws-review, ws-scout (all paseo `auto` mode, worktrees under `~/.paseo/worktrees/31cq0kv6/ws-*`).
- 2026-10-20 — all agents paused by the account session limit (~05:00–06:40 Paris); resumed via `paseo send`.
- 2026-10-20 — ws-scout merged: 30 ranked features, 15 briefs, bench evidence (paper plateau reproduced; `.dc` matches
  closed-form add-drop to 1e-6). Top 5: F-01 reproducible runs, F-02 sweeps/Monte-Carlo, F-03 S-parameter device,
  F-04 receiver chain, F-05 outputs/tracing. ws-review: B-001..B-122 committed, parser/tests areas + verification ongoing.
- 2026-10-20 — ws-meson merged. Coordinator re-verification on a fresh worktree: setup+compile OK (1m43 at -j4),
  `meson test` 10 ok + 1 expected-fail (`Waveguide.OperatingPoint`), golden 45/45 at `--atol 0` incl. gtest; only
  source edit is the approved `src/test/utils.h` patch. CI never ran on real runners (no push). Human-review items:
  `discover/meson/DECISIONS.md` entries tagged **[review]**.
- 2026-10-20 — ws-review merged: 178 issues (0 critical, 15 high), 70 easy-unattended; 85 independent second checks
  (61 confirmed, 24 corrected, 0 refuted). Reviewer rule needing maintainer sign-off: any fix that changes results of a
  netlist that runs today is `needs-human` even if mechanical (B-052, B-146, B-147, B-035, B-009). Phase 2 launched.
