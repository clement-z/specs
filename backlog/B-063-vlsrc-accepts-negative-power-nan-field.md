# B-063: VLSRC accepts negative power: a NaN field is emitted, then silently dropped
severity: low | category: bug | effort: XS | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parse_element.cpp:785-813, src/devices/value_list_source.h:109-125, circuits/subckt_tests.cir:26
## Problem
`values.emplace_back(t, spx::oa_value_type(sqrt(p), wl))` with p < 0 gives a NaN field. VLSource logs it as emitted. The output port's tolerance test never passes on NaN, so the event is dropped and the net silently keeps its previous value. The repository example circuits/subckt_tests.cir uses `[3e-9, -2,]`.
## Evidence
- Golden `cir/subckt_tests` stdout: `@3 ns, ROOT/VLSRC1 emitted: ... (nan V.m⁻¹, nan W, nan rad)`.
- Its VCD `PROBE{ROOT/in}.power`: `#1000000 r1`, `#2000000 r2`, `#4000000 r0`. It stays at 2 W through 3 ns.

Additional evidence (independent report F-A4-27, A4 reviewer):

tests/golden/baseline/cases/cir/subckt_tests/stdout.txt:101:
`@3 ns, ROOT/VLSRC1 emitted: [3219] @ 1550 nm (nan V.m⁻¹, nan W, nan rad)`.
## Expected
An error, or a documented clamp, for P < 0 or non-finite values, in both the list and the file paths.
## Suggested fix
Reject `p < 0` (and non-finite t/P/λ) with `std::cerr ...; exit(1);` in both places. Because subckt_tests.cir relies on the current acceptance, someone must decide whether to change the example (for example use 0.5) or to clamp.
## Golden impact
cir/subckt_tests and opt/vci (same netlist) would fail or change unless the example is edited, in which case they need re-recording.
## Related
B-053, A1 (OOP silently swallowing NaN).

- Coupled with B-135: `circuits/subckt_tests.cir` (golden case `cir/subckt_tests`) relies on a negative power today.

- Merged duplicate reports: F-A4-27 (A4).
- Review ids: F-A3-17, F-A4-27.
## Verification
Golden baseline outputs (stdout and VCD) inspected.
