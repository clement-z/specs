# PD3 progress (F-15 part 3: `SC_METHOD` conversion)

Branch `ws/methods` (from `sprint/2026-10c` 39f1bf9). Heartbeat: `acf69ae1` (PD3-heartbeat).
Scratch (not in the repo): `/tmp/pd3` (binaries per step, recordings, scripts).

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 0 | references: base build (39f1bf9) strict golden, event counters, clang/asan-gcc recordings; D1 tip (56e5808) build for the bench | in progress | | base `pixi run test gcc` 12/12 OK |
| 1 | `OpticalOutputPort::on_data_ready` → `SC_METHOD` | todo | | |
| 2 | stateless device reactions → `SC_METHOD` (waveguide, coupler, splitter, merger, crossing, phase shifter) | todo | | `--vci` print order kept by a one-shot thread |
| 3 | `AGENTS.md` process style, docs, CHANGELOG | todo | | |
| 4 | bench (`C-notrace N=30` vs D1 tip 56e5808), gate, report | todo | | |
