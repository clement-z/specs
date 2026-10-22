# B-012: "No wavelength" aliases λ-id 0: wrong printouts, exit(1) when no λ is registered
severity: low | category: bug | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/optical_signal.h:31-40,65-71,232-233, src/optical_signal.cpp:19-22,48-51,139-157
## Problem
A default or NaN wavelength maps to id 0, which is the first registered λ. There is no "unset" sentinel, so every
`isnan(getWavelength())` check is dead (optical_signal.cpp:19,21,48,50, optical_signal.h:233, probe.cpp:35,58,71,
GTD:177). Unwritten nets print as "@ <first λ> nm". When nothing has registered a λ, printing any signal calls
`exit(1)`.
## Evidence
`backlog/repro/a1-op-no-wavelength.cir`:
`║ - ROOT/a (OPTICAL) = Wavelength not found in global vector.`, rc=1, in the middle of the OP table.
With a VLSRC the unwritten nets print `@ 1550 nm (0.000000 …)`.
## Expected
Unset signals are printed as "no λ" and never terminate the program.
## Suggested fix
Introduce an explicit invalid id (e.g. `UINT32_MAX`), make the default constructor and `setWavelength(NaN)` use
it, make `getWavelength()` return NaN for it instead of exiting, and keep the existing isnan checks (they become
live). Devices that index memory maps with `[0]` "for NaN" need review (detector.cpp:12,68: A3).
## Golden impact
Stdout of cases that print unwritten nets (λ field would read "nan" or "-" instead of 1550 nm).
## Related
B-010, B-027.

- Discovery seeds: S1.15, S2.30 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-12.
## Verification
Ran the repro. Grepped all isnan sites.
