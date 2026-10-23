# B-187: The wavelength matching tolerance (relative 1e-12) can merge different wavelengths or miss drifted ones
severity: low | category: physics | effort: S | fix-class: needs-human | verdict: PLAUSIBLE | status: open (not studied further unless an issue points to it)
location: src/optical_signal.h (`OpticalSignal::WAVELENGTH_MATCH_RTOL`, `sameWavelength()`), src/optical_signal.cpp (`getIDFromWavelength()`), src/devices/probe.cpp (`MLambdaProbe::listedWavelength()`)

## Problem
Since sprint 2026-10c phase H (B-010, B-062), wavelengths equal within a relative 1e-12 are one wavelength: the
registry gives them one id (the first registered value is kept) and MLPROBE matches its list with the same rule.
The tolerance is a compile-time constant (maintainer decision of 2026-10-23, third round). It can be a source of
error in two ways:
- two wavelengths that really differ by less than 1e-12 relative (about 1.6e-18 m, or 190 Hz, at 1550 nm) are merged
  and add coherently (no realistic netlist is known to need such a spacing);
- a computed wavelength that drifts by more than 1e-12 relative from the value it stands for is not matched:
  e.g. a `.dc` sweep with very many points accumulates start + step + step + ... (B-009; of the order of 1e-16 relative per step,
  so thousands of steps before it can matter), or a long chain of expressions. It is then a separate wavelength again
  (as before phase H), and an MLPROBE ignores it with a warning.
A chain of values each within 1e-12 of the next is not merged transitively: a value matches the first
registered one within the tolerance, not a cluster.

## Evidence
Analysis only (phase H). The golden cases have one such drift: the `.dc` point 1.550000000000001e-06 of
`circuits/add_drop.cir` (6.4e-16 from 1.55e-6), now merged with the source's 1.55e-6 as intended.

## Suggested fix
None for now: not studied further unless an issue points to it. Options if it does: a tolerance option, warn when
two registered wavelengths are closer than some larger threshold, or compute the `.dc` points as start + i·step
(B-009).

## Related
B-009, B-010, B-062, `doc/numerics.md` ("Wavelength matching").
