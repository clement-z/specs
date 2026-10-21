# P1 progress — golden harness hardening (branch `ws/harness`)

Builds: `/tmp/P1-gcc` (Meson default, gcc 11.5), `/tmp/P1-clang` (clang 19.1.7), `/tmp/P1-debug` (gcc `-Dbuildtype=debug`, -O0).

| item | status | commit | notes |
|---|---|---|---|
| B-129 | in progress | | |
| B-126 | pending | | |
| B-127 | pending | | |
| B-128 | pending | | |

Heartbeat: **not created**. `paseo heartbeat create ... --name P1-heartbeat` (coordinator rule) was denied by the
Claude Code auto-mode permission classifier ("Unauthorized Persistence"); no workaround attempted. The coordinator
must resume this agent manually after an interruption.
