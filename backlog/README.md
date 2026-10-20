# SPECS backlog — schema and labels

Produced by the sprint 2026-10 review workstream (`ws/review`). The index is [`../backlog.md`](../backlog.md);
the human summary (counts, top 10, themes, order of attack, rejected seeds) is [`SUMMARY.md`](SUMMARY.md).

## Layout
| Path | Content |
|---|---|
| `backlog.md` (repo root) | Index table, one row per issue, sorted by severity then ID |
| `backlog/B-NNN-<slug>.md` | One file per issue (template below) |
| `backlog/repro/` | Minimal repro netlists and scripts referenced from issue files |
| `backlog/SUMMARY.md` | Counts, top 10, themes, recommended order of attack, rejected discovery seeds |

IDs are sequential (`B-001`, `B-002`, …) and never reused. If an issue turns out to be wrong, keep the file and set
`verdict: REJECTED` with the reason, rather than deleting it.

## Taxonomy
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
  - Interpretation used in this backlog: a fix that changes the results of a netlist that **runs today** is
    `needs-human`, even when today's result is clearly wrong and the patch is mechanical (e.g. B-052, B-146, B-147).
    Such issues say "a maintainer sign-off is enough" in their *Suggested fix*. Turning a crash or abort into a clean
    error, or rejecting input that has no valid meaning (NaN-producing parameters), stays `easy-unattended`.
- **verdict**: `CONFIRMED` (reproduced by running, or proven by unambiguous code reading with a cited line) or
  `PLAUSIBLE` (strong suspicion, not proven). Anything weaker was dropped.

## Issue template (`backlog/B-NNN-slug.md`)
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
## Verification   (how the issue was verified, and by whom: reviewer run, code reading, second independent check)
```
`## Verification` is an addition to the sprint template: every `easy-unattended` issue and every `critical`/`high`
issue received a second, independent check (a fresh agent tried to reproduce or refute it). Its outcome is recorded
there.

## Conventions used in issue files
- Line numbers refer to the tree at the commit that added the issue (base: `sprint/2026-10` @ b66dc53). They drift
  as fixes land; search for the quoted code if a line moved.
- Repro commands assume a Release build at `build/specs` and a cwd that contains `circuits/` and an empty
  `traces/` dir, e.g. `mkdir -p /tmp/r/traces && cp -r circuits backlog/repro /tmp/r && cd /tmp/r &&
  <repo>/build/specs -f repro/<file>.cir`.
- Paths under `/tmp/ws-review-*` (e.g. `/tmp/ws-review-build/specs`, `/tmp/ws-review-ninja.log`) record where a
  reviewer or second checker ran something. They are provenance only and are not kept; everything needed to
  reproduce an issue is in `backlog/repro/`.
- **Golden impact** names case ids from `tests/golden/cases.json` (`python3 -I tests/golden/run_golden.py list`).
  A fix whose impact is not "none" must re-record the baseline in the same commit (see `CLAUDE.md`).
- **Seeds** `S<r>.<n>` refer to the n-th bullet of the "Seeds for backlog" section of
  `discover/0<r>-*.md` (e.g. `S3.07` = 7th seed of `discover/03-parser-netlist-pyspecs.md`). The disposition of all
  148 seeds is listed in `SUMMARY.md`.

## Workflow for fixers
- One commit per issue: `fix(B-NNN): <subject>` (cleanups: `refactor(B-NNN): …` or `chore(B-NNN): …`).
- Run `python3 -I tests/golden/run_golden.py check --specs build/specs --baseline tests/golden/baseline -j 8`
  before committing. If the issue's golden impact is "none", the check must pass unchanged.
- Only `easy-unattended` issues may be fixed without a human decision. `needs-human` issues touch physics,
  numerics or the results of valid simulations; `needs-design` issues need an API/behaviour choice first.
- When a fix lands, change the issue's verdict line to add `status: fixed in <sha>` (do not delete the file).
