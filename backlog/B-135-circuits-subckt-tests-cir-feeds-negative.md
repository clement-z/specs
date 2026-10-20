# B-135: circuits/subckt_tests.cir feeds a negative power into a value-list source, producing a NaN field that is baked into the golden baseline
severity: low | category: bug | effort: XS | fix-class: needs-design | verdict: CONFIRMED
location: circuits/subckt_tests.cir:26, tests/golden/baseline/cases/cir/subckt_tests/stdout.txt (and opt/vci)
## Problem
`vlsrc1 in values=[[1e-9,1,{lambda0}], [2e-9,2,], [3e-9, -2,], [4e-9, 0,]]` has power `-2` at 3 ns. The source emits `sqrt(P)`, so the log shows `@3 ns, ROOT/VLSRC1 emitted: ... (nan V.m^-1, -nan W, -nan rad)` and NaN travels through the circuit. Whether that was intended (sign meaning a pi phase?) is unknown; `-nan` is normalised to `nan` by the harness precisely because of this, so the golden baseline codifies a NaN.
## Evidence
```
$ specs -f backlog/repro/a5-negative-power-vlsrc.cir | grep emitted
@3 ns, ROOT/VLSRC1 emitted: ...  (nan V.m^-1, -nan W, -nan rad)
```
## Expected
No NaN in a shipped example; negative power rejected (or defined).
## Suggested fix
Decision: either change the example value to a valid number (e.g. `[3e-9, 0.5,]`), or make `VLSource` reject negative power with an error (device area), then fix the example. Fixing the example changes the golden stdout/VCD of cir/subckt_tests, cir-json/subckt_tests (values in JSON) and opt/vci.
## Golden impact
cir/subckt_tests, cir-json/subckt_tests, opt/vci: re-record.
## Related
Reviewer A2/A3 (`a2-coupler-nan.cir`, VLSource validation); `-nan` normalisation in run_golden.py:56.

- Coupled with B-063: if VLSRC starts rejecting negative power, this example must be fixed in the same commit.

- Review ids: F-A5-15.
## Verification
Ran the repro and the shipped circuit (both print `nan`).
