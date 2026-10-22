# PD2b progress (branch ws/cleanup2)

Heartbeat: `12a5e2a5` (PD2b-heartbeat, every 5 min, expires in 48 h).
Start: 2140dd4. "Before" builds: build-pixi/{gcc,clang,asan-gcc,asan-clang} at 2140dd4; recordings in /tmp/pd2b
(golden check gcc, stats-record gcc, record clang/asan-gcc/asan-clang).

| item | status | commit | notes |
|---|---|---|---|
| 1 no-coalescing switch | done | 8c17275 | predicted golden impact only (help, 9 stats JSON lose "coalesce", gtest list); 2 cases removed, 2 added; gcc strict 228/228, meson test gcc + asan-clang OK |
| 2 delta writes, generic devices | in progress | | |
| 3 F-03 brief, MLPROBE backlog item | todo | | |
| 4 gate, report | todo | | |
