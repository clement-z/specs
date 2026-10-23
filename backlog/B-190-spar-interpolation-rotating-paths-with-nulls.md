# B-190: SPAR interpolation is poor on paths whose phase rotates fast and that have nulls
severity: low | category: physics | effort: S | fix-class: needs-human | verdict: PLAUSIBLE | status: approved 2026-10-23 (validate, then fix: phase PF03c)
location: src/io/sparam_file.cpp (interpolation of the tabulated entries)

## Problem
Found in phase PF03b (`backlog/sprint-2026-10c/PF03b-REPORT.md`, "Found, not done"). On a path whose phase turns
by a large angle between grid points and whose magnitude has nulls (an unbalanced MZI, FSR 5.6 nm, sampled every
0.2 nm, 28 points per FSR), neither interpolant is good: field error 0.023 between grid points with the mixed
interpolation (magnitude/phase, real/imaginary near nulls), 0.070 with magnitude/phase alone, 0.079 with
real/imaginary alone. Resolved rings and delay lines are fine (<= 1e-3, delay lines exact).

## Expected
Interpolation error on such paths comparable to the resolved cases, without degrading them.

## Suggested fix (approved by the maintainer, conditional on validation)
Delay-removed interpolation: interpolate S(f) exp(-j 2 pi f tau0) with a delay tau0 per path taken out (e.g. the
mean group delay or the slope of a linear fit of the unwrapped phase over the file's band), then put the linear
phase back. Validate first (measure the error with and without on the MZI, the resolved ring, the delay line and
the SiEPIC BDC); keep the grid-point values; the grid-adequacy check uses the same interpolant.

## Golden impact
SPAR cases evaluated between grid points (to be predicted by the phase).

## Related
F-03, PF03b item 4 (mixed interpolation), `backlog/sprint-2026-10c/SPAR-discussion.md`.
