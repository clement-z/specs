# B-010: λ registry uses exact `==`: one physical wavelength can get two ids (coherence lost)
severity: med | category: physics | effort: S | fix-class: needs-human | verdict: CONFIRMED
location: src/optical_signal.cpp:111-137, src/devices/cw_source.h:30-38
## Problem
`getIDFromWavelength` uses an exact `find`. Values that are equal to the user but not bit-identical get different
ids: `1550*1e-9` is not `1.55e-6`, `c/f` from `setFrequency`, and drifted `range()` values. Sources at "the same"
λ then add incoherently, as separate wavelengths, and both print as "@ 1550 nm". The perf part of the seed is
wrong: registry lookups happen only at setup and once per DC point, never on the per-event path.
## Evidence
`backlog/repro/a1-near-duplicate-lambda.cir` (two in-phase 1 W sources into a MERGER):
`ROOT/m = @ 1550 nm (0.707107, 0.500000 W)`. Control with both written `1.55e-6`: `1.414214, 2.000000 W`.
## Expected
Wavelengths equal within a tolerance share an id.
## Suggested fix
Match with a relative tolerance (e.g. `|a-b| <= 1e-12·max(|a|,|b|)`) and return the existing id. The tolerance
value is a modelling decision.
## Golden impact
None expected for current cases (no near-duplicates). It changes results of netlists that rely on the bug.
## Related
B-009, B-012.

- Discovery seeds: S1.16 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-10.
## Verification
Ran the repro and the control.
