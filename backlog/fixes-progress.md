# ws-fixes — progress log (working file; the final report is `FIXES-REPORT.md`)

Orchestrator: Opus 5.5 on branch `ws/fixes` (from `sprint/2026-10` @ c4dd3ef). Instructions:
`discover/orchestration/ws-fixes.md`. If this run is interrupted, resume from the tables below: an item is done
when its `fix(B-NNN)` commit is on `ws/fixes`.

## Scope decisions
- 70 `easy-unattended` items, all `CONFIRMED` (no `PLAUSIBLE` candidate).
- **Deferred to the cleanup phase (ws-cleanup)**, as the orchestration rules ask for pure `style/maintainability` items
  (22): B-026, B-027, B-028, B-041, B-043, B-045, B-065, B-067, B-068, B-073, B-075, B-090, B-094, B-097, B-110,
  B-111, B-112, B-141, B-142, B-173, B-174, B-178.
- **Obsolete (Meson migration, 8b7733a)**: B-117, B-118 (CMake files removed; Meson uses explicit source lists and
  defaults to `buildtype=release`). Also bookkept, though not `easy-unattended`: B-119, B-120, B-121.
- **B-023** (field label `V.m⁻¹` → `√W`) re-records the stdout of ~25 cases, so it is done last, on top of every
  other fix (B-047, B-132 add stdout that also carries the label).
- B-044, B-072, B-170 are listed under "cleanup" in `SUMMARY.md` but are categorised bug / memory, so they are
  taken here.
- Golden harness: `record --only --note` now keeps the note (`manifest.json` → `rerecords`, 9a53213). Test-only
  netlists go in `tests/golden/netlists/`; new case ids are `err/bNNN-*` (clean rejection) or `regress/bNNN-*`.
- "Reordered" notes (heap-layout dependent stdout order, B-016) are accepted and not re-recorded per item.

## Wave 1 (implementer subagents, isolated worktrees)
| Agent | Model | Items (in order) |
|---|---|---|
| A test infra | Opus | B-123, B-125, B-136, B-138 |
| B engine/CLI | Opus | B-006, B-020, B-017, B-018, B-131 |
| C parser validation | Opus | B-008, B-148, B-165, B-153, B-161, B-034 |
| E parser lexer/tree/memory | Opus | B-149, B-150, B-156, B-154, B-170, B-109, B-177 |
| F devices crash/UB | Opus | B-047, B-031, B-054, B-036 |
| D python | Sonnet | B-076, B-077, B-085, B-086, B-087, B-088, B-089, B-091, B-092, B-099 |
| G docs/misc | Sonnet | B-114, B-083, B-100, B-102, B-082, B-064, B-044, B-072, B-132 |
| (last) | — | B-023 |

## Merged into ws/fixes
| ID | commit | note |
|---|---|---|
| B-076 | a8e85a5 | D (python) |
| B-077 | 859242d | D (python) |
| B-085 | 3eb4088 | D (python) |
| B-086 | 30f6820 | D (python) |
| B-087 | e6785a4 | D (python) |
| B-088 | 0e8db9c | D (python) |
| B-089 | 6da7886 | D (python) |
| B-091 | c12a08c | D (python) |
| B-092 | 9d68f74 | D (python) |
| B-099 | 6590a7e | D (python) |
| B-114 | 4c44c8d | wave 1 |
| B-083 | b918231 | wave 1 |
| B-100 | 3892f9a | wave 1 |
| B-102 | 9b2422b | wave 1 |
| B-082 | f9ea7f7 | wave 1 |
| B-064 | 417c053 | wave 1 |
| B-044 | c0766e5 | wave 1 |
| B-072 | 4bb350e | wave 1 |
| B-132 | 65f9286 | wave 1 |
| B-123 | 2be107d | wave 1 |
| B-125 | 7ab418c | wave 1 |
| B-136 | 542dfde | wave 1 |
| B-138 | 7b43d7c | wave 1 |
| B-006 | 2b812c6 | wave 1 |
| B-020 | 126969f | wave 1 |
| B-017 | 41bc091 | wave 1 |
| B-018 | 9ef2a5d | wave 1 |
| B-131 | 11339e9 | wave 1 |
| B-008 | 9917667 | wave 1 |
| B-148 | 3e55eb3 | wave 1 |
| B-165 | b50ced3 | wave 1 |
| B-153 | 93df9a1 | wave 1 |
| B-161 | 8903599 | wave 1 |
| B-034 | 50fe059 | wave 1 |
| B-149 | 9c13e71 | wave 1 |
| B-150 | c89946f | wave 1 |
| B-156 | af92c25 | wave 1 |
| B-154 | 97339bd | wave 1 |
| B-170 | e2f0768 | wave 1 |
| B-109 | 23b5f0f | wave 1 |
| B-177 | 77d87b6 | wave 1 |
| B-047 | 4369a7d | wave 1 |
| B-031 | 913dbcd | wave 1 |
| B-054 | 122fa88 | wave 1 |
| B-036 | 898552a | wave 1 |
| B-023 | 773dd69 | orchestrator (last) |

Done: see FIXES-REPORT.md.
