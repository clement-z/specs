# PD2 progress (F-15 part 2: bit-identical engine refactors)

Branch `ws/engine2` (from `sprint/2026-10c` 56e5808). Heartbeat: `8e2ab409` (PD2-heartbeat).

Baseline before any change (pixi gcc 15 release, `build-pixi/gcc`): `pixi run test gcc` 12/12 OK;
`pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0` 227 passed, 0 failed/new/missing.
Event counters of the base: `stats-record` 226 cases, 134 with statistics (`/tmp/pd2/stats/base-gcc`).
Base binary for the bench: `/tmp/pd2/bin/base/specs`. Base recordings of clang, asan-gcc, asan-clang:
`/tmp/pd2/rec/base-<toolchain>` (helper `/tmp/pd2/scripts/record_builds.sh`).

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 1 | tolerance gate on squared norms (+ unit tests: last = 0, NaN, inf) | todo | | |
| 2 | per-wavelength S-parameter caches (waveguide, coupler, phase shifter) | todo | | |
| 3 | pending events: time-only order, insertion order on ties, single `sc_event`, merge as a lookup [Q8] | todo | | predicted re-records: plain probes/VCD of multi-wavelength nets |
| 4 | `OpticalSignal`: no virtual destructor, id behind a compile-time flag [Q2] | todo | | predicted re-record: `[NNNN]` prefix of listings |
| 5 | bench after each step, report | todo | | |
