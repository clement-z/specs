# B-186: No per-wavelength probe in a `.dc` sweep (MLPROBE traces nothing there)
severity: med | category: feature | effort: M | fix-class: needs-design | verdict: CONFIRMED | status: fixed (sprint 2026-10c PL)
location: src/devices/probe.cpp (`MLambdaProbe`), the `.dc` runner in src/specs.cpp

## Problem
In a `.dc` sweep, MLPROBE declares its per-wavelength VCD signals but records no value: MLPROBEs stay disabled
for the whole `.dc` (noted in the sprint 2026-10c PD5 report). A plain PROBE traces each point, but only one
wavelength per point (the PROBE wavelength latch, phase D5). So a `.dc` of a multi-wavelength (WDM) circuit has no
way to record per-wavelength values, e.g. the transmission of each channel while a voltage, a power or one
source's wavelength is swept.

## Evidence (2026-10-23, sprint/2026-10c after D3, gcc)
`backlog/repro/b186-mlprobe-dc-wavelength-sweep.cir`: a swept 1.548–1.549 um source and a fixed 1.55 um source
merged onto one net, with a PROBE and an MLPROBE (1.548, 1.549, 1.55 um) on it. The VCD has the MLPROBE's 9
signals but no non-zero value for any of them; PROBE1 records each point.

## Expected
A per-wavelength probe that records each `.dc` point: one value per listed wavelength and point, in the VCD and
the CSV/npz tables (which already have one row per point, probe and wavelength).

## Suggested fix (needs design)
Enable MLPROBEs in `.dc` (as PROBEs are), sampling each point's converged value per listed wavelength; decide what
a wavelength absent at a point shows (0, or its last value), and how this interacts with B-185 (nets whose value
does not change between points are not rewritten, so a fixed wavelength is only seen at the first point). Related
design question for later (maintainer, 2026-10-23): the respective roles of PROBE (one wavelength, the swept one
in a wavelength sweep) and MLPROBE in `.dc` and frequency/transmission sweeps.

## Golden impact
`.dc` cases with an MLPROBE (if any) gain values; add the repro as a golden case.

## Related
B-184 (MLPROBE at `.tran` start), B-185 (`.dc` and unchanged nets), PD5 report (PROBE latch).

## Update (sprint 2026-10c PH)
B-185 is fixed (b6488a3): every wavelength of a net is written again at every `.dc` point, so the interaction
mentioned above is gone; an MLPROBE enabled in `.dc` would see each of its wavelengths at every point. MLPROBE
stays disabled in `.dc` (separate decision); the repro's evidence is unchanged.

## Fix (sprint 2026-10c PL, maintainer's decision 2026-10-24)
`runDCAnalysis()` enables the MLPROBEs (`m_per_point`). During a point, an MLPROBE keeps the last value of each
listed wavelength received at that point (events only: every wavelength of a net is written again at each
point, B-185); once the point converged, `runDCAnalysis()` calls `MLambdaProbe::tracePoint()`, which traces
every listed wavelength in wavelength order, the received value or 0 for a wavelength the net did not carry at
that point (fresh steady state), in the VCD signals and in the table (`TraceTable::appendZero()`: a zero row
at the listed wavelength, which need not be registered). The trace signals are written before the
`sc_start()` that ends the point, so the VCD records them at the point's time step. PROBE is unchanged
(it follows the swept wavelength, PD5). `.op` is not changed: there, PROBEs and MLPROBEs are both disabled by
`runOPAnalysis()` (the operating point of a `.tran` too), a separate question (PL report).
Tests: gtest `MLambdaProbe.DcPointTracesEveryListedWavelengthOrZero`; golden cases
`regress/b186-mlprobe-dc-wavelength-sweep` (this repro) and `regress/b186-mlprobe-dc-power-sweep-wdm`
(a power sweep of a two-channel add-drop ring). No existing golden case changed (none has an MLPROBE in a `.dc`).
