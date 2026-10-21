# P1 progress — golden harness hardening (branch `ws/harness`)

Builds: `/tmp/P1-gcc` (Meson default, gcc 11.5), `/tmp/P1-clang` (clang 19.1.7), `/tmp/P1-debug` (gcc `-Dbuildtype=debug`, -O0).

| item | status | commit | notes |
|---|---|---|---|
| B-129 | done | 23797a8 | baseline rewritten in place (30 files, IDs only); unordered streams re-ranked in sorted order |
| B-126 | done | b43e7cd | 8 cases unordered_streams (gcc/clang/-O0 vs baseline); 7 more reorder under clang ASan only: documented, not exempted |
| B-127 | done | bfd6a15 | --allow-new, --no-unit-tests; --only empty -> exit 2; gtest selected by --only |
| B-129 follow-up | in progress | | multiset check broke on tied lines (tb/wg + --lenient-order): replace sorted re-rank by masked multiset + ID profiles |
| B-128 | pending | | |

Heartbeat: **not created**. `paseo heartbeat create ... --name P1-heartbeat` (coordinator rule) was denied by the
Claude Code auto-mode permission classifier ("Unauthorized Persistence"); no workaround attempted. The coordinator
must resume this agent manually after an interruption.
