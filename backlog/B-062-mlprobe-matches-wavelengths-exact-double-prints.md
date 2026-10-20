# B-062: MLPROBE matches wavelengths with exact double `==` and prints on every unmatched event
severity: low | category: bug | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/devices/probe.cpp:150-162, src/parser/parse_element.cpp:1093-1130
## Problem
`m_lambda_signals.find(s.getWavelength())` requires bit-exact equality between the MLPROBE list and the source wavelength. Any source wavelength computed by an expression (`{lambda0}+1.5e-9` = 1.5514999999999999e-06) or by `setFrequency` (c/f) misses.
- The probe traces nothing.
- It prints `received unsupported signal (lambda = 1.5515e-06m)` once per event, with a 6-digit print that looks identical to the requested λ.
## Evidence
`backlog/repro/a3-mlprobe-exact.cir`: two "received unsupported signal (lambda = 1.5515e-06m)" lines, and the VCD has no `MLPROBE1` scope values.
## Expected
A tolerant match (for example |Δλ| ≤ 1e-6·λ, or matching to the nearest registered λ), and a one-time warning.
## Suggested fix
Look up by tolerance, choosing the tolerance as part of the design. Warn once per unknown λ with full precision.
## Golden impact
None: paul_3ring_puf (the only MLPROBE user) fails before simulating.
## Related
B-057, B-066.

- Review ids: F-A3-16.
## Verification
I ran the repro with the release binary.
