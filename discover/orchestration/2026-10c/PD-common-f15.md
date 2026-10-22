# F-15 — shared notes for phases D1–D3 (target v0.3.0)

Brief: `discover/scout/F-15-engine-performance-and-accounting.md` (written in sprint 2026-10, before Meson
subprojects and pixi: its build commands are outdated, its analysis is not). Decisions:
`backlog/DECISIONS-2026-10-21.md`, "Follow-up decisions" (F-15 approved: bit-identical refactors, `SC_METHOD`
conversion, single timed queue, `--stats`, documented expert ablation switches, no new globals) and the answers
in "Decisions of 2026-10-21, second round", cited as **[Q1]…[Q8]**. Read also "Decisions of 2026-10-22" (pixi, and
the second round: B-183, `native` off by default).

## Branches, builds, release
- Each phase branches from `sprint/2026-10c` (released as v0.2.0; dev and master hold the same tree). The
  coordinator merges each phase after its gate, then launches the next one. Changes go to `CHANGELOG.md`
  `[unreleased]` (v0.3.0); no tag, push or merge into `dev`/`master` without the maintainer.
- Build and test through pixi (`COMMON.md`, `AGENTS.md`). The default toolchain `gcc` (pixi gcc 15, portable,
  release) reproduces the golden baseline **bit for bit** (B-183); clang and the debug ASan builds differ from the
  baseline in 6 cases at one sweep point, so for them compare a build **before** your change with one **after**.

## Acceptance (all D phases)
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: 212/212 (plus new cases) with no re-record, except
  the cases a decision predicts for that commit ([Q2], [Q8]); each re-record in its own commit, with the proof
  asked for in the phase prompt.
- `pixi run test gcc`, `pixi run test asan-clang`: `meson test` all OK, no sanitizer message. clang release and
  asan-gcc: bit-identical before/after (`run_golden.py compare` of two recordings, strict tolerances).
- From D2 on, per-case `--stats` counters (collected with the tool D1 delivers) compared before/after as [Q1] says:
  identical in D2, any change explained in D3.
- Performance: `discover/scout/bench/bench_crow.py --specs build-pixi/gcc/specs` (stdlib only; run it with
  `pixi run python -I ...`). The host is shared and noisy: report **ratios** of user time, median of 3, same
  host and session. Target [Q7]: >= 1.5x on the bench's case `C-notrace N=30` by the end of D3.
- Nothing from outside the repository goes into it (`COMMON.md`, "Files outside the repository").
