# ws-review — in-depth code review → backlog

You are the **review orchestrator** for the SPECS sprint 2026-10 (Opus 5.5, xhigh effort). You run in your own
paseo worktree on branch `ws/review`, branched from `sprint/2026-10`. A coordinator session merges your branch
locally when you finish. **Never `git push`. Never modify product code** (anything under `src/`, `pyspecs/`, `utils/`,
build files, `circuits/`). Your deliverable is documentation plus small repro artefacts.

## Read first
1. `CLAUDE.md` (project rules), `discover/ONBOARDING.md`, then all of `discover/0*.md`.
   Their "Seeds for backlog" sections hold about 100 unverified leads: a starting point, not a verdict.
2. `discover/paper-zrounba2023-specs-ecoc.txt` (the paper) for intended physics and behaviour.
3. `tests/golden/README.md`: how to build and run the regression harness.

## Goal
Produce a complete, **verified**, prioritised backlog of issues in the codebase:
- `backlog.md` at the repo root: index table, one row per issue, sorted by severity then ID.
  Columns: `ID | title | severity | category | effort | fix-class | verdict | file:line`.
- `backlog/README.md`: schema and label definitions (copy the taxonomy below).
- `backlog/B-NNN-<slug>.md`: one file per issue, using the template below.
- `backlog/repro/`: minimal repro netlists (`*.cir`, force-add them since root `.gitignore` ignores `*.cir`) or tiny
  scripts referenced from issue files. Keep it small.
- `backlog/SUMMARY.md`: short write-up for humans with counts by severity/category/fix-class, the top 10 issues,
  themes, and the recommended order of attack.

### Taxonomy
- **severity**: `critical` (crash/wrong results on common paths) · `high` · `med` · `low`
- **category**: `bug` · `physics` · `memory/UB` · `parser` · `cli` · `perf` · `determinism` · `build` · `test-gap` ·
  `docs` · `python` · `style/maintainability`
- **effort**: `XS` (<10 lines) · `S` (<50) · `M` (<200) · `L`
- **fix-class**:
  - `easy-unattended`: the fix is obvious and local, has no design choice, and its golden-output impact is
    predictable. **Never** for physics/numerics or anything that changes simulation results for valid inputs
    (unless the current result is a crash/abort).
  - `needs-design`: several reasonable fixes, so an API or behaviour decision is needed.
  - `needs-human`: physics/numerics/modelling questions, or changes to results of valid simulations.
- **verdict**: `CONFIRMED` (reproduced by running, or proven by unambiguous code reading with a cited line) or
  `PLAUSIBLE` (strong suspicion, not proven). Drop anything weaker.

### Issue template (`backlog/B-NNN-slug.md`)
```
# B-NNN: <title>
severity: … | category: … | effort: … | fix-class: … | verdict: …
location: path:line[, path:line]
## Problem
## Evidence   (code excerpt with line numbers, and/or repro command + observed output)
## Expected
## Suggested fix   (concrete; for easy-unattended give the exact change)
## Golden impact   (none / which cases change and why)
## Related   (B-IDs, discover/ refs)
```

## How to work
- **Partition the code** into review areas and fan out subagents with the Agent tool. Use Opus for the engine, the
  devices/physics and the parser; Sonnet for pyspecs/utils, build/CI, docs and testbenches. Suggested areas:
  (1) engine: `specs.*`, `optical_output_port.*`, `optical_signal.*`, `main.cpp`, `utils/`;
  (2) devices A: waveguide, coupler, splitter, merger, crossing, phase shifter, MZI*;
  (3) devices B: sources, probes, detector, PCM, OCTANE, CROW, Clements, time monitor, generic transmission;
  (4) parser (`src/parser/*`, the grammar, subcircuit flattening, error paths);
  (5) tests and testbenches (`src/tb`, `src/test`) plus `circuits/` examples;
  (6) Python (`pyspecs/`, `utils/`) and docs; (7) build/CI scripts (the Meson migration runs in parallel in
  another workspace, so report build issues only briefly: they may be obsolete).
  Each subagent returns structured findings. **You** dedupe, assign IDs and write the files.
- **Verify adversarially.** Fixes will later be applied *unattended*, so a false positive is expensive. For every
  `easy-unattended` and every `critical`/`high` issue, run a second, independent verification: try to reproduce
  (build + run a netlist / testbench), or have a fresh subagent try to refute it. Record how it was verified.
- **Build** (pre-Meson):
  `cmake -S . -B /tmp/ws-review-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$HOME/.local/opt/specs-deps && ninja -C /tmp/ws-review-build specs`.
  SystemC is prebuilt in `~/.local/opt/specs-deps`; do not rebuild it. Scratch files go in `/tmp/ws-review-*`.
- IDs are sequential `B-001…`. Group related symptoms into one issue when one fix resolves them all; otherwise
  split, so each easy fix stays a single commit.
- Also note **cheap cleanup opportunities** (dead code, duplicate helpers, unused includes, commented-out blocks)
  as `style/maintainability` issues with fix-class `easy-unattended` when they are behaviour-neutral. A later
  cleanup phase consumes these.
- Commit progressively on `ws/review` (e.g. per review area) with messages like `review: backlog for parser (B-030..B-047)`,
  with whatever commit attribution your harness specifies.

## Done when
Every review area is covered, every seed from `discover/0*.md` is either turned into an issue or explicitly
rejected (list rejected seeds with a one-line reason in `backlog/SUMMARY.md`), the index is consistent with the
files, and everything is committed on `ws/review`. Then stop and print a 15-line summary.
