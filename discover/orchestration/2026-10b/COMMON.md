# Common rules for every sprint 2026-10b phase

You run unattended in your own paseo worktree, on the phase branch named in your phase prompt, branched from
`sprint/2026-10b`. The coordinator merges your branch after an independent check. **Never `git push`.**

## Read first
`CLAUDE.md`, `discover/SPRINT-2026-10b.md` (board), **`backlog/DECISIONS-2026-10-21.md` (the maintainer's
decisions — they define what to build)**, the backlog files of your items (`backlog/B-NNN-*.md`), and for features the
scout briefs (`discover/scout/F-NN-*.md`). `tests/golden/README.md` for the regression harness.

## Scope
- Only the items of your phase, implemented exactly as decided. If an item needs a choice the decision does not
  cover, take the most conservative option, write the choice in your report, and keep going. If the choice would
  change simulation results beyond what the decision states, stop that item and report it instead.
- Anything else you notice: add it to your report under "Found, not done". Don't fix it.

## Build and test
- Prebuilt deps (SystemC 3.0.2, args 6.4.7): `meson setup <builddir> -Dcmake_prefix_path=$HOME/.local/opt/specs-deps-next`.
  Do **not** set `CC`/`CXX` (Meson then uses ccache automatically). Build dirs under `/tmp/<phase>-*`.
- Before and after each item: `meson compile`, `meson test`, and
  `python3 -I tests/golden/run_golden.py check --specs <b>/specs --unit-tests <b>/src/test/unit_tests --baseline tests/golden/baseline -j 4 --atol 0`
  (plus `--strict`/`--strict-order` once P1 makes them the default).
- Golden changes: a re-recorded case must be predicted by the decision for that item. Re-record only those cases
  (`record --only ... --note "B-NNN: why"`) in the same commit as the change. An unexpected output change means the
  change is wrong or out of scope: stop the item and report it.
- Add a regression test for every behaviour change (gtest with the fork macros, and/or a golden case).

## Commits and progress (this is what lets the work survive interruptions)
- One commit per item: `fix(B-NNN): …`, `feat(F-NN): …`, `test(golden): …`, `doc: …`, with cause, change, test and
  golden impact in the body.
- Keep `backlog/sprint-2026-10b/<Pn>-progress.md` current (item → status, commit, notes) and commit it after
  every item. On resume, read it first.
- Update each issue file (`status: fixed in <sha>` or `status: …`) and its row in `backlog.md`; update user docs
  (`README.md`, `doc/`, cheatsheet sources if any) and `CHANGELOG.md` `[unreleased]` for user-visible changes.

## Subagents
Use the Agent tool for independent strands (Opus for engine/parser/physics, Sonnet for mechanical work). Never use
`isolation: "worktree"` (it creates worktrees in the maintainer's main checkout): create your own with
`git worktree add /tmp/<phase>-wt/<name> -b <phase>-wip/<name> <your branch>`, cherry-pick from them, and review every
subagent diff yourself before it lands.

## Done
Write `backlog/sprint-2026-10b/<Pn>-REPORT.md`: items (outcome, commit, tests, golden impact), decisions you had to
take, "Found, not done", and the final `meson test` + golden results. Commit it (the coordinator treats it as the
completion marker), then stop and print a 10-line summary.
