# PG — cleanup after v0.3.0: progress

Branch `ws/cleanup3`. Heartbeat id: `837055bd` (delete after the report is committed).

| Item | Status | Commit | Notes |
|---|---|---|---|
| 1 backlog statuses | done | 70fc144 | 3 Sonnet subagents (medium), diffs reviewed; 3 fixed, 13 partly, rest still open |
| 2 MLPROBE per-event print | done | 5b946f2 | as predicted: no existing case changed; new case regress/b062-mlprobe-unlisted-wavelength; counters identical |
| 3 SC_HAS_PROCESS | done | 9f31c11 | 38 uses removed; clang warnings 455 -> 4 (pre-existing), gcc 9 -> 9; bit-identical and counters identical on gcc, clang, asan-gcc, asan-clang |
| 4 B-182 subckt line numbers | todo | | |
| 5 small items | todo | | |
| 6 VLSource prints (look only) | todo | | |

## Before references (tip 6e5e83c)
Binaries in `/tmp/pg/before/<tc>` (gcc, clang, asan-gcc, asan-clang), recordings `/tmp/pg/rec-before-<tc>`,
counters `/tmp/pg/stats-before-<tc>`. gcc strict golden: 235/235 pass. `meson test` OK on the four toolchains.

## Predicted golden impact
- Item 2 (MLPROBE print): no existing case prints "received unsupported signal" (grep of the baseline: none;
  the only MLPROBE netlists list every wavelength of their net, paul_3ring_puf fails before simulating, B-134).
  So no existing case changes. New case(s) show the stderr warning.
  Result: as predicted (gcc strict 236/236, counters identical gcc and asan-clang, asan-clang meson test OK).
- Note: the saved before binaries need `subprojects/systemc-3.0.2/libsystemc.so*` next to them (rpath
  `$ORIGIN/...`); the first before recordings were invalid (exit 127) and were redone.
- Item 3 (SC_HAS_PROCESS): predicted none (bit-identical, counters identical). Result: as predicted. The before
  recordings predate item 2, so the comparisons show `regress/b062-...` only (item 2's change; that case equals
  the committed baseline on all four toolchains).
- Item 4 (B-182): the 11 golden cases with a "Parsing error" all have the error at top level (file:line correct);
  none has an error inside a `.subckt` body, so no existing case changes. New err cases: errors inside a body
  (syntax error, B-180 integer literal, nested `.ends` name mismatch, error in a nested body) give the defining
  file and the line in that file.
