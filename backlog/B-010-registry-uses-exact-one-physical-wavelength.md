# B-010: λ registry uses exact `==`: one physical wavelength can get two ids (coherence lost)
severity: med | category: physics | effort: S | fix-class: needs-human | verdict: CONFIRMED | status: fixed in 59479b7 (sprint 2026-10c PH)
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

## Fix (sprint 2026-10c PH, 59479b7)
Maintainer decision (2026-10-23, third round): wavelengths equal within a relative 1e-12 are one wavelength,
a compile-time constant (`OpticalSignal::WAVELENGTH_MATCH_RTOL`, `sameWavelength()`, `doc/numerics.md`
"Wavelength matching"). `getIDFromWavelength()` returns the first registered wavelength that matches; its value
is kept. Lookups are still made only at setup and once per `.dc` point. The repro now gives `ROOT/m = @ 1550 nm
(1.414214, 2.000000 W)` (golden `regress/b010-near-duplicate-lambda`). Golden impact: 8 `.dc` cases built on
`circuits/add_drop.cir` and the paper's Fig. 1b change at the one point that drifted to within 1e-12 of the
source's registered 1.55e-6 (B-009); that point now equals a fresh point at exactly 1.55e-6. The tolerance as a
source of error: B-187.
