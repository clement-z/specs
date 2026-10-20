# B-003: DC sweep: a point whose output falls below abstol keeps the previous point's value and λ
severity: med | category: bug | effort: XS | fix-class: needs-human | verdict: CONFIRMED
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
