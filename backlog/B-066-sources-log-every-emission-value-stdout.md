# B-066: Sources log every emission and value to stdout (per DC point, per VL sample)
severity: low | category: perf | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: partly addressed in 5b946f2 (sprint 2026-10c PG: the MLPROBE per-event print is gone) and baf872b (PJ: the VLSRC/EVLSRC per-value prints and their "was enabled" only with --vci, the past values in one stderr warning per source); open: the CWSRC emission line per point and the PDET "was enabled" (not in the 2026-10-23 decision)
location: src/devices/cw_source.cpp:33, src/devices/value_list_source.cpp:31,38,49,62, src/devices/value_list_source.h:78,84,95,124, src/devices/electrical_value_list_source.cpp:31,38,60, src/devices/electrical_value_list_source.h:75,81,92,112, src/devices/detector.cpp:45, src/devices/probe.cpp:161
## Problem
Every CW emission is logged unconditionally, and a DC sweep re-emits once per point (30 lines in cir/add_drop). Also logged unconditionally:
- every VL/EVL sample, at parse time and again at emission;
- each "was enabled";
- each unmatched MLPROBE event.

A value file with 1e5 rows prints 2e5 lines. These are user-facing logs rather than pure debug output, so silencing them is a behaviour choice.
## Evidence
cir/add_drop golden stdout: 30 x `ROOT/CWSRC1 emitted: ...`. subckt_tests prints 5 `emitted` lines.
## Expected
Quiet by default, with per-event logging behind a verbosity option.
## Suggested fix
Gate these behind a new verbosity level, or behind `verbose_component_initialization`. Decide the option name and default.
## Golden impact
stdout of cir/add_drop, opt/* (5 cases), cir/add_drop_pyspecs, cir/subckt_tests, opt/vci and tb/lambda would lose these lines.
## Related
B-065.

- Discovery seeds: S2.27 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-20.
## Verification
Golden stdouts grepped.
