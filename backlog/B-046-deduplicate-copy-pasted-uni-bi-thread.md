# B-046: Deduplicate the copy-pasted Uni/Bi thread bodies (S-parameter setup)
severity: low | category: style/maintainability | effort: M | fix-class: needs-design | verdict: CONFIRMED | status: partly addressed in bd92622 (sprint 2026-10c D3): waveguide and coupler compute once in the Base; phase shifter and crossing copies and pure-function unit tests stay open
location: src/devices/waveguide.cpp:6-101, src/devices/waveguide.cpp:104-203, src/devices/waveguide.cpp:205-280, src/devices/directional_coupler.cpp:8-21, src/devices/directional_coupler.cpp:72-85, src/devices/directional_coupler.cpp:116-129, src/devices/directional_coupler.cpp:190-203, src/devices/directional_coupler.cpp:234-247, src/devices/directional_coupler.cpp:278-291, src/devices/phaseshifter.cpp:6-198, src/devices/crossing.cpp:6-128
## Problem
The S-parameter and delay computation is repeated in several places:
- 3× in the waveguide, ~75 lines each, with 16 copies of the `1.55e-6` literal;
- 6× in the directional coupler (writing the shared `m_through_power_dB`/`m_cross_power_dB` members each time);
- 5× for the phase-shifter transmission;
- 2× in the crossing.

I diffed the copies: they are currently consistent (WG Uni vs Bi-p0 vs Bi-p1 differ only in port names and verbose
output; the DC and PS forward/backward mappings are reciprocal). However, every physics fix (B-032/03/09) has to be
applied 2-6 times, and nothing can be unit-tested without SystemC.
## Evidence
`diff` of waveguide.cpp:6-101 vs :104-203 vs :205-280 after renaming ports shows only verbose-print differences.
## Expected
One computation per device: a pure function or Base method that returns the S-parameters and delay for a wavelength.
## Suggested fix
For example:
- `WaveguideBase::transfer(double lambda_m, OpticalSignal::field_type &S, sc_time &delay) const`;
- `DirectionalCouplerBase::sparams(field_type &t, field_type &k) const`;
- `CrossingBase::sparams(...)`;
- `PhaseShifterBase::transmission(double v)`.

Call them from the thread prologues and loops, and make the derived dB members locals. Unit-test the pure functions.
Design is needed (API, and where λ0/precomputation live).
## Golden impact
None if done carefully: the arithmetic must stay identical (same operation order) to keep VCD bit-equality.
## Related
B-039, B-032, B-033. A5 S2.29 (unit tests).

- Discovery seeds: S2.28, S5.17 (see `backlog/README.md` for seed numbering).
- Review ids: F-A2-16.
## Verification
Code reading and line diffs as above.
