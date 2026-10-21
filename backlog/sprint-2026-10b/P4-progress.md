# P4 progress — F-01 reproducible runs (branch `ws/repro`)

Prompt: `discover/orchestration/2026-10b/P4-repro.md`. Heartbeat: paseo heartbeat **`99a0d01d`** (P4-heartbeat,
every 5 min, expires 48 h). Delete it right after committing `P4-REPORT.md`.

Start: `ws/repro` @ 3fc4f94 (P1–P3 merged). Baseline check at start (gcc `/tmp/P4-gcc`): `meson test` 12/12,
golden 134/134 `--atol 0`, 0 notes.

## Items
| # | item | owner | status | commit | notes |
|---|---|---|---|---|---|
| 1 | B-176 element creation backlog in netlist order | orchestrator | in progress | | parser `elements_backlog` was `set<const ParseElement*>` |
| 2 | B-016 name-ordered `sc_get_all_*` (init, OP listing, auto-probes, VCD order) + golden re-record (ordering only, proven) | orchestrator | pending | | |
| 3 | `test(golden)`: drop every `unordered_streams` exception | orchestrator | pending | | P1's 8 + later B-016 ones |
| 4 | F-01 seeds: `--seed`, `.options seed`, `spx::rng_for`, Detector streams per name, `darkfield` option (B-055, B-056) | Opus implementer, `/tmp/P4-wt/rng` (`P4-wip/rng`) | pending | | |
| 5 | F-01 run manifest `<trace>.manifest.json` + `--print-manifest` | Opus implementer, `/tmp/P4-wt/manifest` (`P4-wip/manifest`) | pending | | |
| 6 | verification: `check --strict` twice, clang ASan/UBSan, `record --repeat 3` | orchestrator | pending | | |
| 7 | docs, backlog status, CHANGELOG, report | orchestrator | pending | | |

## Decisions (running list, copied into the report)
- Default seed when no `--seed`/`.options seed`: **0** (fixed, documented).
- Element creation backlog (B-176): ordered by netlist position (index in the flattened element list), not by
  name: it is a work queue, today's glibc pointer order is allocation (= netlist) order, so this keeps today's
  creation order; name order would change slot assignments (results), which the decision does not predict.
- Dark-field injection kept **on by default** (today's behaviour, now seeded); PDET keyword `darkfield=0` turns it off.

## Log
- 2026-10-21 — heartbeat 99a0d01d created; context read; baseline build green.
