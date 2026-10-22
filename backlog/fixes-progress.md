# ws-fixes — progress log (working file; the final report is `FIXES-REPORT.md`)

Orchestrator: Opus 5.5 on branch `ws/fixes` (from `sprint/2026-10` @ 3b94976). Instructions:
`discover/orchestration/ws-fixes.md`. If this run is interrupted, resume from the tables below: an item is done
when its `fix(B-NNN)` commit is on `ws/fixes`.

## Scope decisions
- 70 `easy-unattended` items, all `CONFIRMED` (no `PLAUSIBLE` candidate).
- **Deferred to the cleanup phase (ws-cleanup)**, as the orchestration rules ask for pure `style/maintainability` items
  (22): B-026, B-027, B-028, B-041, B-043, B-045, B-065, B-067, B-068, B-073, B-075, B-090, B-094, B-097, B-110,
  B-111, B-112, B-141, B-142, B-173, B-174, B-178.
- **Obsolete (Meson migration, 57714e7)**: B-117, B-118 (CMake files removed; Meson uses explicit source lists and
  defaults to `buildtype=release`). Also bookkept, though not `easy-unattended`: B-119, B-120, B-121.
- **B-023** (field label `V.m⁻¹` → `√W`) re-records the stdout of ~25 cases, so it is done last, on top of every
  other fix (B-047, B-132 add stdout that also carries the label).
- B-044, B-072, B-170 are listed under "cleanup" in `SUMMARY.md` but are categorised bug / memory, so they are
  taken here.
- Golden harness: `record --only --note` now keeps the note (`manifest.json` → `rerecords`, b1f333f). Test-only
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
| B-076 | 286bd34 | D (python) |
| B-077 | 601c777 | D (python) |
| B-085 | a921bc0 | D (python) |
| B-086 | d22213b | D (python) |
| B-087 | ef0ea37 | D (python) |
| B-088 | 685edc0 | D (python) |
| B-089 | b63f02f | D (python) |
| B-091 | aacfde1 | D (python) |
| B-092 | a20b67a | D (python) |
| B-099 | 5c29fbb | D (python) |
| B-114 | 427873a | wave 1 |
| B-083 | bfba715 | wave 1 |
| B-100 | b3f1719 | wave 1 |
| B-102 | acc890d | wave 1 |
| B-082 | 907abee | wave 1 |
| B-064 | 7caf561 | wave 1 |
| B-044 | f6ec1c5 | wave 1 |
| B-072 | 9939a3e | wave 1 |
| B-132 | 093f890 | wave 1 |
| B-123 | eb7ca0e | wave 1 |
| B-125 | 60972ff | wave 1 |
| B-136 | 84d087e | wave 1 |
| B-138 | 6fa8c9e | wave 1 |
| B-006 | eee3be8 | wave 1 |
| B-020 | 2f69dc5 | wave 1 |
| B-017 | 28adebe | wave 1 |
| B-018 | b588268 | wave 1 |
| B-131 | 449d63c | wave 1 |
| B-008 | ff391b2 | wave 1 |
| B-148 | 2310bdd | wave 1 |
| B-165 | 91dc39f | wave 1 |
| B-153 | d90348d | wave 1 |
| B-161 | c12ea44 | wave 1 |
| B-034 | 2408b9f | wave 1 |
| B-149 | bc74a11 | wave 1 |
| B-150 | 89db4c4 | wave 1 |
| B-156 | 00556f2 | wave 1 |
| B-154 | 0dcfda3 | wave 1 |
| B-170 | 86f2707 | wave 1 |
| B-109 | c712d3f | wave 1 |
| B-177 | 2a4df67 | wave 1 |
| B-047 | aa0e1c6 | wave 1 |
| B-031 | 2d1e116 | wave 1 |
| B-054 | 2997ba2 | wave 1 |
| B-036 | a8600c3 | wave 1 |
| B-023 | 5ea38e4 | orchestrator (last) |

Done: see FIXES-REPORT.md.
