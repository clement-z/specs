# Common rules for every sprint 2026-10c phase

You run unattended in your own paseo worktree, on the phase branch named in your phase prompt, branched from
`sprint/2026-10c`. The coordinator merges your branch after an independent check. **Never `git push`.**

## Heartbeat (set it up first, before any other work)
Usage limits can stop you (and your subagents) at any time. Create your own heartbeat so you resume on your own:
```
paseo heartbeat create --cron "*/5 * * * *" --expires-in 48h --name "<Pn>-heartbeat" \
  "[heartbeat <Pn>] If you are in the middle of work, ignore this and continue. If you were stopped (usage limit,
   error, cut-off subagents), resume: read backlog/sprint-2026-10c/<Pn>-progress.md, resume or relaunch the
   subagents that were cut off, and continue with the next item. If <Pn>-REPORT.md is already committed, delete
   this heartbeat (paseo heartbeat delete <id>) and stop."
```
Note its id in `<Pn>-progress.md`. Delete it right after committing your report. Keep heartbeat turns short
when nothing needs doing.

## Read first
`AGENTS.md` (agent instructions; `CLAUDE.md` imports it), `discover/SPRINT-2026-10c.md` (board), **`backlog/DECISIONS-2026-10-21.md` (the maintainer's
decisions — they define what to build)**, the backlog files of your items (`backlog/B-NNN-*.md`), and for features the
scout briefs (`discover/scout/F-NN-*.md`). `tests/golden/README.md` for the regression harness.

## Scope
- Only the items of your phase, implemented exactly as decided. If an item needs a choice the decision does not
  cover, take the most conservative option, write the choice in your report, and keep going. If the choice would
  change simulation results beyond what the decision states, stop that item and report it instead.
- Anything else you notice: add it to your report under "Found, not done". Don't fix it.

## Build and test
- Build through pixi (`AGENTS.md`, "pixi route"; if `pixi` is not on your PATH, your launch prompt says where it
  is): in your worktree, `pixi install --locked` if `.pixi/` does not exist, then
  `pixi run test gcc` (configure + build + `meson test` in `build-pixi/gcc`). SystemC and args come from the wraps.
- A clang sanitizer build is part of every item's check: `pixi run test asan-clang`; `meson test` there applies
  `tests/sanitizers/lsan.supp` (leak detection on). Never widen that file without a note in your report explaining
  which allocation it covers and why it is not a real leak.
- Before and after each item: `pixi run test gcc` and `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`
  (bit-identical to the baseline for non-native builds, B-183; strict order and coverage are the default).
- Golden changes: a re-recorded case must be predicted by the decision for that item. Re-record only those cases
  (`record --only ... --note "B-NNN: why"`) in the same commit as the change. An unexpected output change means the
  change is wrong or out of scope: stop the item and report it.
- Add a regression test for every behaviour change (gtest with the fork macros, and/or a golden case).

## Golden bookkeeping conflicts
Cherry-picks often conflict only in `tests/golden/cases.json` and `tests/golden/baseline/manifest.json`. Resolve
them as a union (both sides' cases, both sides' `rerecords` entries, deduplicated by (`recorded_at_utc`, `only`)),
never by dropping entries, then re-run the golden check.

## Files outside the repository
Read only what your prompt names. Never copy files, text or data from outside the repository into it.

## Commits and progress (this is what lets the work survive interruptions)
- One commit per item: `fix(B-NNN): …`, `feat(F-NN): …`, `test(golden): …`, `doc: …`, with cause, change, test and
  golden impact in the body.
- Keep `backlog/sprint-2026-10c/<Pn>-progress.md` current (item → status, commit, notes) and commit it after
  every item. On resume, read it first.
- Update each issue file (`status: fixed in <sha>` or `status: …`) and its row in `backlog.md`; update user docs
  (`README.md`, `doc/`, cheatsheet sources if any) and `CHANGELOG.md` `[unreleased]` for user-visible changes.

## Subagents
Choose the model per task (haiku for trivial lookups, sonnet for mechanical or documentation work, opus for
engine, parser, physics and review) and give every subagent an explicit effort, medium or high (maintainer request,
2026-10-21).
Use the Agent tool for independent strands (Opus for engine/parser/physics, Sonnet for mechanical work). Never use
`isolation: "worktree"` (it creates worktrees in the maintainer's main checkout): create your own with
`git worktree add /tmp/<phase>-wt/<name> -b <phase>-wip/<name> <your branch>`, cherry-pick from them, and review every
subagent diff yourself before it lands.

## Done
Write `backlog/sprint-2026-10c/<Pn>-REPORT.md`: items (outcome, commit, tests, golden impact), decisions you had to
take, "Found, not done", and the final `meson test` + golden results. Commit it (the coordinator treats it as the
completion marker), then stop and print a 10-line summary.
