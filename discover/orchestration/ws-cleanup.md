# ws-cleanup — easy, behaviour-neutral code cleanup

You are the **cleanup orchestrator** for the SPECS sprint 2026-10 (Opus 5.5, high). You run in your own paseo
worktree on branch `ws/cleanup`, branched from `sprint/2026-10` after the fixes were merged. A coordinator merges
your branch locally. **Never `git push`.**

Pipeline: **you (Opus, high) review → Sonnet subagents implement and test → you (or a fresh Opus subagent) review
each result.**

## Read first
`CLAUDE.md`, `backlog.md` (especially `style/maintainability` items still open, and `backlog/FIXES-REPORT.md`),
`discover/0*.md` sections on duplication and dead code, `tests/golden/README.md`.

## 1. Review: find cleanups that are *easy* and *provably behaviour-neutral*
Good candidates: dead code (unreferenced functions such as `raphael_main`/`do_circuit` in `main.cpp`, the dead
`Ring` device, `#if 0` blocks), large commented-out code, unused variables/parameters/includes (use the compiler
warnings from a clean `meson compile` as a list), duplicate files (`utils/draw_graph_from_json.py` vs
`pyspecs/`), stale comments and TODOs that are now done, obvious typos in messages/comments, trivially duplicated
helpers within one file.
**Not** in scope: reformatting, renaming public/netlist-visible names, changing class hierarchies, deduplicating
the big Uni/Bi copy-paste (that is a design task: log it in the backlog as `needs-design` if not already there),
anything touching physics/numerics, anything whose neutrality you cannot argue in one sentence.
Write the plan as `backlog/cleanup/PLAN.md`: batches of independent items (C-01, C-02, …), each with files,
the exact change, a neutrality argument, and the test to run.

## 2. Implement: Sonnet subagents (Agent tool, `model: sonnet`, `isolation: "worktree"`)
One batch per subagent. Each must: make only the planned change, in the local style, with no reformatting of
untouched lines; then `meson compile` (warnings must not increase; list before/after counts), `meson test` (all
suites), and `python3 -I tests/golden/run_golden.py check --atol 0 --strict` against the baseline. **No baseline
re-recording is allowed in this phase**: any golden difference means the change is not neutral, so revert it.
They return their diff and test evidence.

## 3. Review: Opus
Review every returned diff for neutrality, scope creep and style before cherry-picking into `ws/cleanup` as
`cleanup(C-NN): <summary>` (reference backlog IDs when applicable). Reject or trim anything doubtful.
After all batches: full rebuild from a clean build dir with gcc and clang, `meson test`, strict golden check.

## Done when
Write `backlog/cleanup/REPORT.md` (what was done, what was rejected and why, warning-count delta, LOC delta), mark
the consumed backlog items as fixed, commit, stop, and print a 15-line summary.
