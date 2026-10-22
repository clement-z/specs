# Phase F (pixi) — progress

Heartbeat: `45fdd6ee` (PF-heartbeat, every 5 min, expires 48 h after 2026-10-22 11:5x UTC). Delete after the report.

| Step | Status | Commit | Notes |
|---|---|---|---|
| 1. pixi.toml + pixi.lock | done | 3a1408f | gcc/clang/asan-gcc/asan-clang 12/12, golden 212/212 --atol 0; strict diffs -> B-183 (65c6da3) |
| 2. Docs (README, CLAUDE.md, CHANGELOG, meson DECISIONS) | done | fc7b176 | Sonnet draft, reviewed; CLAUDE.md B-183 note corrected |
| 3. CI pixi job | done | d0bff37 | YAML parses; local walk-through pending (fresh clone) |
| 4. Worktree/ccache measurements | in progress | | /tmp/PF-m1, /tmp/PF-m2, cold cache /tmp/PF-cold |
| 5. macOS notes | todo | | |
| Checks | todo | | |
| Report | todo | | |
