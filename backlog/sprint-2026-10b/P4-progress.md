# P4 progress — F-01 reproducible runs (branch `ws/repro`)

Prompt: `discover/orchestration/2026-10b/P4-repro.md`. Heartbeat: paseo heartbeat **`99a0d01d`** (P4-heartbeat,
every 5 min, expires 48 h). Delete it right after committing `P4-REPORT.md`.

Start: `ws/repro` @ 3fc4f94 (P1–P3 merged). Baseline check at start (gcc `/tmp/P4-gcc`): `meson test` 12/12,
golden 134/134 `--atol 0`, 0 notes.

## Items
| # | item | owner | status | commit | notes |
|---|---|---|---|---|---|
| 1 | B-176 element creation backlog in netlist order | orchestrator | done | 7c12a61 | 9 cases re-recorded (stdout line order only, multiset-equal; paul_3ring_puf error stop point 2 elements earlier) |
| 2 | B-016 name-ordered `sc_get_all_*` (init, OP listing, auto-probes, VCD order) + golden re-record (ordering only, proven) | orchestrator | done | 5c6d742 | 56 cases: 55 reordered (proof script), b144-three-writers explained; tb/det, tb/detector left for item 4 |
| 3 | `test(golden)`: drop every `unordered_streams` exception | orchestrator | done | (see log) | 14 cases (P1's 8 + 6 later) |
| 4 | F-01 seeds: `--seed`, `.options seed`, `spx::rng_for`, Detector streams per name, `darkfield` option (B-055, B-056) | Opus implementer a96cbaf6240d36dfe, `/tmp/P4-wt/rng` (`P4-wip/rng`) | done | 083c8be | 22feaa5 cherry-picked + .options seed INT_MAX guard; lexer fix d6e81d4 held back (to park as P4-ready/lexer-int-overflow); 17 new cases, tb/det(ector) compared |
| 5 | F-01 run manifest `<trace>.manifest.json` + `--print-manifest` | Opus implementer a8aba4f323342d840, `/tmp/P4-wt/manifest` (`P4-wip/manifest`) | done | 4e3e87d | 2815930 + 72eedeb squashed with the re-record; 84 cases gain only the manifest |
| 6 | verification: `check --strict` twice, clang ASan/UBSan, `record --repeat 3` | orchestrator | done | | gcc/clang/-O0/ASan 153/153 strict; record --repeat 3 byte-identical to the baseline |
| 7 | docs, backlog status, CHANGELOG, report | orchestrator | done | e05090d, 1eb606c, report | |

## Decisions (running list, copied into the report)
- Default seed when no `--seed`/`.options seed`: **0** (fixed, documented).
- Element creation backlog (B-176): ordered by netlist position (index in the flattened element list), not by
  name: it is a work queue, today's glibc pointer order is allocation (= netlist) order, so this keeps today's
  creation order; name order would change slot assignments (results), which the decision does not predict.
- Dark-field injection kept **on by default** (today's behaviour, now seeded); PDET keyword `darkfield=0` turns it off.

## Log
- 2026-10-21 — heartbeat 99a0d01d created; context read; baseline build green.
- 2026-10-21 — item 1 (B-176) committed 7c12a61; golden 134/134 `--atol 0`, 0 notes.
- 2026-10-21 — items 2 (5c6d742) and 3 committed; 134/134 strict, 0 notes. upgrade_signal assert did not fire (creation order = netlist order). clang ASan build started in /tmp/P4-asan.
- 2026-10-21 — clang 19 ASan+UBSan -O0 (/tmp/P4-asan, ASAN_OPTIONS=detect_leaks=0) at 5c6d742 code: check --atol 0 --strict: 134 passed, 0 notes (no --lenient-order).
- 2026-10-21 — same code: clang 19 release (/tmp/P4-clang) and gcc -O0 (/tmp/P4-debug): check --atol 0 --strict 134/134, 0 notes.
- 2026-10-21 — item 4 (seeds) committed 083c8be; golden 151/151 --atol 0; meson 12/12. Next: integrate the manifest (P4-wip/manifest).
- 2026-10-21 — item 5 (manifest) 4e3e87d, docs e05090d; golden 153/153 --atol 0. Next: park lexer fix (P4-ready/lexer-int-overflow), final verification (fresh builds, strict x2, ASan, record --repeat 3).
- 2026-10-21 — lexer fix parked on P4-ready/lexer-int-overflow @ 7c49a98 (cherry-picks cleanly on HEAD). Final verification green. Worktrees /tmp/P4-wt/* removed. Report committed; heartbeat 99a0d01d deleted after it.
