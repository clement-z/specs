# B-124: lossless ring settles at 0.995 W instead of 1 W with the default reltol (error is about 50 x reltol)
severity: med | category: physics | effort: M | fix-class: needs-human | verdict: CONFIRMED | status: fixed in c527d7d (sprint 2026-10b P3)
location: src/optical_output_port.cpp (convergence check, owner: core reviewer), tests/golden/baseline/cases/tb/ring, CLAUDE.md "Physics & numerics"
## Problem
For a lossless all-pass ring the output power must be exactly 1 W at steady state for any phase. With the default `reltol=1e-4` the simulator settles at 0.995098 W (-0.49 %), with `reltol=1e-6` at 0.999952 W, with `reltol=1e-9` at 0.9999999 W. So energy conservation is violated by roughly 50 to 100 times the requested relative tolerance, because event propagation is cut off when the change per round trip is below the tolerance. Users who read `reltol=1e-4` as "1e-4 accuracy" are off by two orders of magnitude (the ECOC paper states "0.01 %" for the benchmark tolerance). Not a crash, but analytic tests must use a tolerance of about 100 x reltol, and the golden baselines (tb/ring, opt/*) encode the truncated values.
## Evidence
```
$ specs -f backlog/repro/a5-allpass-ring-op.cir --reltol 1e-4     -> ROOT/out ... 0.995098 W
$ specs -f backlog/repro/a5-allpass-ring-op.cir --reltol 1e-9     -> ROOT/out ... 1.000000 W
.tran version (30 ns, coupler k=sqrt(0.15), 775 um ring): final probe power 0.9950975 / 0.9999522 / 0.9999999 for reltol 1e-4 / 1e-6 / 1e-9
specs -t ring (golden tb/ring): probe_ring.power settles at 0.9950975205875567 for 10 ns
```
## Expected
Either documented error scaling (cheatsheet: "steady-state error of a resonator is about 50-100 x reltol") or a convergence criterion that bounds the accumulated error, as a maintainer decision.
## Suggested fix
Decision only: document, or change the convergence criterion in `OpticalOutputPort` (changes simulation results, therefore needs-human per CLAUDE.md).
## Golden impact
none for documentation; any criterion change alters tb/ring, cir/add_drop_pyspecs, tb/ac_crow and most TRAN cases.
## Related
B-137, B-140 (analytic checks must use the loose tolerance).

- See also B-021 (tolerance gate semantics).

- Review ids: F-A5-03.
## Verification
Ran the CW and TRAN variants at three tolerances (above). The `.op` variant is in the repro.
