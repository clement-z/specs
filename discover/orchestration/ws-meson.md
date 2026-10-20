# ws-meson — implement the Meson migration

You are the **Meson migration implementer** for the SPECS sprint 2026-10 (Opus 5.5). You run in your own paseo
worktree on branch `ws/meson`, branched from `sprint/2026-10`. A coordinator merges your branch locally when you finish.
**Never `git push`.**

1. Read `CLAUDE.md`, `discover/ONBOARDING.md`, `discover/04-build-test-ci.md` (esp. §9) and `tests/golden/README.md`.
2. Implement **`discover/meson/PLAN.md`** end to end. It is the approved spec: Meson replaces CMake and the
   Makefile; CI goes to both GitLab and GitHub Actions. Where the plan says "choose and justify", decide and
   record the decision in `discover/meson/DECISIONS.md`.
3. Prove parity as described in PLAN.md §5 step 2. Record commands and results in `discover/meson/VALIDATION.md`.
   The golden-output check must pass. That is the acceptance gate.
4. Submodules: run `git submodule update --init --recursive` in your worktree if `thirdparty/*` are empty. Prebuilt
   SystemC + args are in `~/.local/opt/specs-deps` (read-only for you).
5. You may use subagents (Agent tool) for independent strands, e.g. a Sonnet subagent drafting the CI files while
   you do the build, or a fresh-eyes reviewer of your final diff. Keep the critical path (build + parity) yourself.
6. Commit in logical steps (PLAN.md §5). When done, print a 15-line summary: decisions, validation results,
   anything left undone or that a human should look at.
