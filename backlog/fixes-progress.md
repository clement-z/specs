# ws-fixes — progress log (working file; the final report is `FIXES-REPORT.md`)

Orchestrator: Opus 5.5 on branch `ws/fixes` (from `sprint/2026-10` @ 5539495). Instructions:
`discover/orchestration/ws-fixes.md`. If this run is interrupted, resume from the tables below: an item is done
when its `fix(B-NNN)` commit is on `ws/fixes`.

## Scope decisions
- 70 `easy-unattended` items, all `CONFIRMED` (no `PLAUSIBLE` candidate).
- **Deferred to the cleanup phase (ws-cleanup)**, as the orchestration rules ask for pure `style/maintainability` items
  (22): B-026, B-027, B-028, B-041, B-043, B-045, B-065, B-067, B-068, B-073, B-075, B-090, B-094, B-097, B-110,
  B-111, B-112, B-141, B-142, B-173, B-174, B-178.
- **Obsolete (Meson migration, b86a745)**: B-117, B-118 (CMake files removed; Meson uses explicit source lists and
  defaults to `buildtype=release`). Also bookkept, though not `easy-unattended`: B-119, B-120, B-121.
- **B-023** (field label `V.m⁻¹` → `√W`) re-records the stdout of ~25 cases, so it is done last, on top of every
  other fix (B-047, B-132 add stdout that also carries the label).
- B-044, B-072, B-170 are listed under "cleanup" in `SUMMARY.md` but are categorised bug / memory, so they are
  taken here.
- Golden harness: `record --only --note` now keeps the note (`manifest.json` → `rerecords`, b8b2aaf). Test-only
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
| B-076 | 0ec69a4 | D (python) |
| B-077 | 1abd047 | D (python) |
| B-085 | dfa3440 | D (python) |
| B-086 | 33fc498 | D (python) |
| B-087 | 94921a3 | D (python) |
| B-088 | 7def19f | D (python) |
| B-089 | ffb51fa | D (python) |
| B-091 | 956ab10 | D (python) |
| B-092 | 5e54f9b | D (python) |
| B-099 | 936f458 | D (python) |
| B-114 | a066d23 | wave 1 |
| B-083 | 3a8577c | wave 1 |
| B-100 | a4cece1 | wave 1 |
| B-102 | 4371bc3 | wave 1 |
| B-082 | a968ab8 | wave 1 |
| B-064 | b4508ef | wave 1 |
| B-044 | 027cccf | wave 1 |
| B-072 | 06222d2 | wave 1 |
| B-132 | 9dbea23 | wave 1 |
| B-123 | 50bd413 | wave 1 |
| B-125 | 538d8f8 | wave 1 |
| B-136 | 1510b75 | wave 1 |
| B-138 | dd26ddc | wave 1 |
| B-006 | e111932 | wave 1 |
| B-020 | d3478a9 | wave 1 |
| B-017 | 0577e14 | wave 1 |
| B-018 | 587c4de | wave 1 |
| B-131 | ac676da | wave 1 |
| B-008 | cb1f02a | wave 1 |
| B-148 | 987293d | wave 1 |
| B-165 | c95e3dd | wave 1 |
| B-153 | 5055fa1 | wave 1 |
| B-161 | 313a0cf | wave 1 |
| B-034 | 3f7eb1c | wave 1 |
| B-149 | 30cdd55 | wave 1 |
| B-150 | f597031 | wave 1 |
| B-156 | e22a9f9 | wave 1 |
| B-154 | 96262f1 | wave 1 |
| B-170 | 0446727 | wave 1 |
| B-109 | 5611f3d | wave 1 |
| B-177 | 2cf960d | wave 1 |
| B-047 | 302e640 | wave 1 |
| B-031 | 3a99564 | wave 1 |
| B-054 | f398a14 | wave 1 |
| B-036 | fb97125 | wave 1 |
| B-023 | 33bb8bb | orchestrator (last) |

Done: see FIXES-REPORT.md.
