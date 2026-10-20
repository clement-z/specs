# B-029: `sc_trace(SPECSConfig)` traces uint64/enums through `(int&)` casts
severity: low | category: memory/UB | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/specs.cpp:385-388
## Problem
`(int&)` on `sc_time::value_type` (uint64) and on enums breaks strict aliasing (GCC 11 `-O2`: 4×
`-Wstrict-aliasing`). It traces only 32 bits of the multiplier, and the wrong half on big-endian.
## Evidence
`c++ -O2 -Wall -fsyntax-only src/specs.cpp` → `dereferencing type-punned pointer will break strict-aliasing rules` at 385-388.

Additional evidence (independent report F-A7-08, A7 reviewer):

```
specs.cpp:385: sc_trace(tf, (int&)s.default_resolution_multiplier, parent_tree + "resolution_multiplier");
specs.cpp:386-388: same for engine_timescale, simulation_mode, analysis_type
baseline VCDs: "$var wire 32 aaabq resolution_multiplier [31:0] $end" (and engine_timescale/simulation_mode/analysis_type, 32 bit)
```
## Expected
Well-defined tracing.
## Suggested fix
Trace the uint64 directly (`sc_trace` has a `uint64` overload; this changes the VCD var width 32→64). For the
enums, give them a fixed `int` underlying type and trace `int` mirror members, or stop tracing these constants.
## Golden impact
VCD var `resolution_multiplier` width (all VCD cases) if traced as uint64.
## Related
none.

- Discovery seeds: S1.26, S4.14 (see `backlog/README.md` for seed numbering).
- Merged duplicate reports: F-A7-08 (A7).
- Review ids: F-A1-29, F-A7-08.
## Verification
Compiled specs.cpp and saw the warnings.
