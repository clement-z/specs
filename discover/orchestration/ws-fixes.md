# ws-fixes — implement the easy backlog fixes, unattended

You are the **fix orchestrator** for the SPECS sprint 2026-10 (Opus 5.5, xhigh). You run in your own paseo
worktree on branch `ws/fixes`, branched from `sprint/2026-10` after the Meson migration and the review backlog were
merged. A coordinator merges your branch locally. **Never `git push`.**

## Read first
`CLAUDE.md` (build/test commands are Meson now), `backlog/README.md`, `backlog.md`, `backlog/SUMMARY.md`,
`tests/golden/README.md`, `discover/meson/DECISIONS.md`.

## Scope
- Candidates: backlog items with `fix-class: easy-unattended`. Take `CONFIRMED` items first. A `PLAUSIBLE` item
  is in scope only once you have reproduced it; then upgrade its verdict in the issue file.
- **Out of scope, always**: anything `needs-human` or `needs-design`; anything that changes simulation results for
  *valid* inputs (physics, numerics, tolerances, event semantics), even if it looks easy. If an "easy" item
  turns out to need a decision, stop, mark it `needs-design` with your notes, and move on.
- Leave pure `style/maintainability` items to the later cleanup phase unless they are a one-line part of a fix.

## Process per item (one commit per backlog ID)
1. Work in **batches of independent items** (different files). Fan them out to implementer subagents (Agent tool,
   `isolation: "worktree"`): **Sonnet** for XS/S mechanical fixes, **Opus** for anything touching the engine,
   the parser grammar or memory/UB.
2. The implementer must:
   a. reproduce the problem (repro netlist / test / command from the issue file) and record the before-output;
   b. add a regression test where feasible: a gtest in `src/test/` (fork-per-test macros), and/or a new golden case
      in `tests/golden/cases.json` for CLI/netlist-level behaviour (record it *after* the fix);
   c. make the minimal fix in the local style (never reformat untouched lines);
   d. `meson compile`, `meson test` (all suites), and `run_golden.py check --atol 0` against the baseline;
   e. if golden outputs change: the change must be exactly what the issue's "Golden impact" predicts (crash →
      clean error, etc.). Then re-record the affected cases with `--only` and `--note "B-NNN"` and include the
      baseline update in the same commit. **Unexpected golden changes ⇒ abandon the item** and report back.
3. You review each returned diff yourself (or with a fresh Opus reviewer subagent for engine/parser changes):
   correctness, minimality, test quality, style. Then cherry-pick into `ws/fixes` as
   `fix(B-NNN): <title>`, with a body explaining the cause, the fix and the test.
4. After each batch: full `meson test` + golden check on `ws/fixes` HEAD. Fix or revert any cross-item interaction.
5. Update the issue file (`status: fixed in <sha>` / `status: skipped — <reason>` / `status: reclassified`) and the
   `backlog.md` index (add a `status` column if missing) in the same or a follow-up commit.

## Done when
Every `easy-unattended` item is fixed, skipped with a reason, or reclassified. Write `backlog/FIXES-REPORT.md`
(table: ID, outcome, commit, tests added, golden impact) and commit it. Stop and print a 15-line summary.
