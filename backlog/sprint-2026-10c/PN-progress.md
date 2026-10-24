# PN progress: equal-time order by wavelength, then a per-time-step queue (branch `ws/timeorder`)

Heartbeat: `8b9a9f97` (PN-heartbeat, every 5 min, expires 48 h).
Base: `ws/timeorder` at ec05f36. Gates before any change: `pixi run test gcc` 17/17 OK; strict golden
(`--rtol 0 --atol 0 --print-slack 0`) 278 passed, 0 failed/new/missing; counters of the base build recorded
(`stats-record`, 277 cases, 161 with statistics) in `/tmp/PN/stats-base`; base binary kept in `/tmp/PN/base/`.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 0 | Base gates, base records (gcc stats; clang, asan-gcc outputs) | in progress | | |
| 1 | Equal-time order by wavelength (heap kept), prediction, re-record, pinning case, docs | todo | | |
| 2 | Per-time-step (bucket) queue, bit-identical to step 1, timing | todo | | |
| 3 | Docs (B-188, comments, CHANGELOG) | todo | | |
| 4 | Acceptance gates, report | todo | | |
