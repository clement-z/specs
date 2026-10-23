# B-062: MLPROBE matches wavelengths with exact double `==` and prints on every unmatched event
severity: low | category: bug | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: fixed in 59479b7 (sprint 2026-10c PH; the per-event print was replaced in 5b946f2, PG)
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
## Status
Open. Sprint 2026-10b P6 (F-05, 824702e) changed the lookup from the VCD signals to the MLPROBE's wavelength list
(still exact `==`): without a trace file (`-o ""`, `--no-trace`, `--format csv`) a listed wavelength used to be
reported as unsupported on every event.

## Fix (sprint 2026-10c PH, 59479b7)
Maintainer decision (2026-10-23, third round): an MLPROBE matches its list within a relative 1e-12, the rule
of the wavelength registry (B-010; `OpticalSignal::sameWavelength()`). `MLambdaProbe::listedWavelength()` finds
the listed wavelength once per wavelength id and caches it; the sample is traced under the listed wavelength
(VCD signals), the CSV/npz row keeps the net's wavelength. The repro's `{lambda0}+1.5e-9` is now traced.
Golden: `regress/b062-mlprobe-unlisted-wavelength` re-recorded (MLPROBE1 traces; MLPROBE0 added for a near miss
beyond the tolerance), new `regress/b062-mlprobe-computed-wavelength` (c/f).
