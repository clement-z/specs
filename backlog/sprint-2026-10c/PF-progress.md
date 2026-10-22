# Phase F (pixi) — progress

Heartbeat: `45fdd6ee` (PF-heartbeat, every 5 min, expires 48 h after 2026-10-22 11:5x UTC). Delete after the report.

| Step | Status | Commit | Notes |
|---|---|---|---|
| 1. pixi.toml + pixi.lock | done | 3a1408f | gcc/clang/asan-gcc/asan-clang 12/12, golden 212/212 --atol 0; strict diffs -> B-183 (65c6da3) |
| 2. Docs (README, CLAUDE.md, CHANGELOG, meson DECISIONS) | done | fc7b176 | Sonnet draft, reviewed; CLAUDE.md B-183 note corrected |
| 3. CI pixi job | done | d0bff37 | YAML parses; local walk-through pending (fresh clone) |
| 4. Worktree/ccache measurements | done | | cold install 221 s / 468 MiB download; 2nd worktree 1.3 s +198 MiB (reflinks); ccache 2nd worktree native 48 % (march=native hashing), nonative 99.5 % (2 s) |
| 5. macOS notes | collected | | in the report |
| Checks | running | | /tmp/PF-scratch/final.sh -> /tmp/PF-final.out |
| Report | todo | | |
