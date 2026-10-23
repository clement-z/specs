# B-190: SPAR interpolation is poor on paths whose phase rotates fast and that have nulls
severity: low | category: physics | effort: S | fix-class: needs-human | verdict: CONFIRMED | status: fixed in 4f323e4 (PF03c; PCHIP partly, see Fix)
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

## Validation (PF03c, `backlog/sprint-2026-10c/PF03c-REPORT.md`)
A stdlib prototype of the interpolants, bit-identical to the C++ (27624 `.dc` evaluations), on exports of the
devices (field error between grid points, all paths; leave-one-out = grid check), today -> fix:
- unbalanced MZI (arms 300/400 um, 0.2 nm, 28 points per FSR): linear 0.0226 -> 0.0016, grid check 0.114 -> 0.0064
  (no warning); PCHIP 0.016 -> 0.016, grid check 0.109 -> 0.043;
- add-drop ring, 0.05 nm: linear 9.7e-4 -> 5.4e-4 (4.0e-4 where |S| >= 0.1, unchanged), grid check 0.0039 -> 0.0021;
  PCHIP 6.6e-4 -> 5.65e-4, grid check 0.0037 -> 0.0019;
- delay lines (the `wg.*` fixtures), SiEPIC BDC, smooth dispersive paths: bit-identical;
- under-resolved (ring k = 0.4 on 0.05 nm, ring on 1 nm): 0.21 -> 0.044 and 0.54 -> 0.54; all still warn.

## Fix
`SParamData::removeDelay()`: per entry, tau0 = the pulse-pair estimate arg(sum_k S(k+1) conj S(k)) / (2 pi mean
step), refined by two Newton steps of g(tau) = sum_k |z_k| sin(arg z_k - 2 pi h_k tau) h_k for non-uniform grids,
intervals with a step above 3 times the median step left out (PF03d, decision "SPAR second discussion"; it minimises
the variation of the delay-removed entry and needs no unwrapping; until PF03d the median of the finite-difference
group delays, which fails with 3 to 5 points around a null; a phase fit or the end-to-end slope is biased to one
arm's delay by the pi steps of the unwrapped phase at nulls), 0 when it turns S by less than 1e-3 rad over the
largest step; f_ref = first grid point. Real and imaginary
parts of S exp(-j 2 pi (f - f_ref) tau0) are interpolated (linear or PCHIP) on the intervals near a null, the delay
put back; an interval is near a null when |S| at either end (as before) **or the segment between the two
delay-removed values** comes within 0.07 max|S| of 0. Magnitude/phase intervals unchanged (linear: mathematically
the same with or without the delay; PCHIP on the detrended phase would give the MZI 0.0035 but degrades smooth
dispersive paths, 6e-7 -> 1.9e-4: not done, a maintainer option). Grid points, group delays and the grid check's
threshold unchanged; the grid check uses the same interpolant. Golden: `out` at 1.55 um in the four ring cases
(~1e-12). Test: gtest `SParamFile.DelayRemovedInterpolationOfARotatingPathWithNulls`. PF03d (pulse-pair tau0): ring
0.05 nm through 0.762 -> 0.749 ps, drop 0.806 -> 1.124 ps, its grid check 0.00215 -> 0.00218; `out` at 1.55 um moves
by 1.7e-14 (`cir/spar-add-drop-ring`, `regress/spar-ring-tran`) and 1.7e-13 on the 1 nm file
(`regress/spar-grid-warning`, tau0 -0.015 -> 0.623 ps), whose grid warning becomes 0.704106 at 1.554 um (0.707522
at 1.546 um: the mirror point of the null, with the grid check's re/im decision on the actual intervals).

