# B-003: DC sweep: a point whose output falls below abstol keeps the previous point's value and λ
severity: med | category: bug | effort: XS | fix-class: needs-human | verdict: CONFIRMED | status: fixed in b6488a3 (sprint 2026-10c PH)
location: src/specs.cpp:161-175, src/optical_output_port.cpp:149,161-175
## Problem
Before each sweep point `runDCAnalysis` calls `oop->reset()`, which clears `m_emitted_fields`. The first value of
each λ is then gated against 0. A port whose new value is ≤ abstol (a dark fringe or a filter null) is never
written. Its net, the auto-probe and the VCD therefore keep the previous sweep point's field **and wavelength**,
so a notch shows up as a bright point. The `#elif 0` branch (lines 165-171) and the never-set flag
`m_skip_next_convergence_check` show that forcing this first emission was intended.
## Evidence
`backlog/repro/a1-dc-dark-point-stale.cir`: MZI, `.dc` with point 1 on an exact null, default tolerances.
VCD `PROBE{ROOT/out}`:
```
power      {0: 1.0, 2: 0.99990, 3: 0.000846}          <- no record at sweep point 1
wavelength {0: 1.552795e-06, 2: 1.562470e-06, ...}     <- point 1 still shows point 0's λ and P=1
```
With the one-line fix below, point 1 reads `power 1.0e-27 @ 1.557632e-06`.
## Expected
Every sweep point reflects that point's steady state.
## Suggested fix
At specs.cpp:163, after `oop->reset();`, add `oop->m_skip_next_convergence_check = true;` so that the first
pop after a reset always writes. Note that the flag is per port, not per λ: with ≥2 λ on one port only the
first pop is forced. A per-λ variant would seed `m_emitted_fields` with NaN.
## Golden impact
None for the one-line fix (validated: all cir/* and opt/* cases pass, including the DC cases add_drop, add_drop_sub, mode-fd and multi-file).
## Related
B-021 (tolerance gate), A3 S1.08 (GTD deltas across DC points), A6 (pyspecs reads DC points by time index).

- Review ids: F-A1-03.
## Verification
Ran the repro (stale point) and relinked with the env-gated one-line fix (correct point). Ran the golden check with the fix active.

## Fix (sprint 2026-10c PH, b6488a3)
Maintainer decision (2026-10-23, third round): each `.dc` point is a fresh steady state. Per wavelength, not per
port (the one-line fix above forced only the first pop of a port): `runDCAnalysis()` sets
`OpticalOutputPort::m_emit_each_wavelength`, so after the per-point reset the first entry of each wavelength is
written whatever the gate says, snapped like any emitted value, and with a value-change event even if the net
already holds that value (`OpticalNet::writeWithEvent()`, `src/optical_net.h`). The repro's point 1 now reads
0 W (snapped) at 1557.63 nm. `m_skip_next_convergence_check` is still never set (B-027). Golden case
`regress/b003-dc-dark-point`; gtest `OpticalOutputPort.DcPointWritesFirstValueOfEachWavelengthWithAnEvent`. No
existing golden case had such a point (their outputs are unchanged by this part).
