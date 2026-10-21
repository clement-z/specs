# P5 progress — F-28 `.op`/`.dc` convergence + F-12 netlist pack (branch `ws/netlist`)

Prompt: `discover/orchestration/2026-10b/P5-netlist.md`. Heartbeat: paseo heartbeat **`50c26c63`** (P5-heartbeat,
every 5 min, expires 48 h). Delete it right after committing `P5-REPORT.md`.

Start: `ws/netlist` @ 0dee6b2 (P1–P4 merged). Builds: gcc `/tmp/P5-gcc`.

## Items
| # | item | owner | status | commit | notes |
|---|---|---|---|---|---|
| 1 | F-28 iteration cap for the `.op`/`.dc` fixed point (`.options maxiter`, `--maxiter`), residual report, non-convergence error + lossless-loop golden case | orchestrator | in progress | | repro: lossless all-pass ring k=1e-3/1e-4 off resonance, `.op` > 60 s (k=1e-2: 0.14 s) |
| 2 | F-12 SI suffixes (`f p n u m k MEG G T`, `m`/`M` milli), also in expressions | Opus implementer `lexgram`, `/tmp/P5-wt/lexgram` (`P5-wip/lexgram`) | in progress | | |
| 3 | F-12 `.ends NAME` | `lexgram` | in progress | | |
| 4 | F-12 `.dc WL(src)` alias of `/src/WL` | `lexgram` | in progress | | |
| 5 | F-12 `.dc` includes the stop value (re-record every `.dc` case: one more point) | Opus implementer `semantics`, `/tmp/P5-wt/semantics` (`P5-wip/semantics`) | in progress | | |
| 6 | F-12 `--param name=value` overrides `.param` | `semantics` | in progress | | |
| 7 | F-12 `.include` relative to the including file, then the cwd; warning if both exist and differ | `semantics` | in progress | | |
| 8 | F-12 legacy ring netlists to `circuits/legacy/` + README note; golden case paths | | pending | | |
| 9 | docs (netlist syntax, numerics, README, CHANGELOG, golden README), backlog status, report | | pending | | |

## Decisions (running list, copied into the report)

## Log
- 2026-10-21 — heartbeat 50c26c63 created; context read. Baseline gcc /tmp/P5-gcc: meson test 12/12, golden 153/153 `--atol 0`.
- 2026-10-21 — implementers launched: `lexgram` (items 2-4), `semantics` (items 5-7); orchestrator on item 1 (F-28).
