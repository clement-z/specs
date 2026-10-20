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
| 1a | Meson migration (replace CMake + Make, GitLab CI + GitHub Actions) | paseo `ws-meson` | Opus 5.5 | pending |
| 1b | In-depth review → `backlog.md` + `backlog/` | paseo `ws-review` | Opus 5.5 xhigh orch. | pending |
| 1c | Feature scout → `discover/scout/` | paseo `ws-scout` | Fable 5.1 xhigh | pending |
| 2 | Easy fixes (unattended) | paseo `ws-fixes` | Opus 5.5 xhigh orch. | pending |
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
