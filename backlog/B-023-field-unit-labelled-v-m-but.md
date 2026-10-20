# B-023: Field unit is labelled V/m but is √W
severity: low | category: docs | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/optical_signal.h:26,234
## Problem
`power() = |E|²` is printed in W, and `CWSource` sets `|E| = sqrt(P)`. The field is therefore √W, but the comment
and every printout say "V/m" / "V.m⁻¹".
## Evidence
`… (1.000000 V.m⁻¹, 1.000000 W …)` for a `power=1` source.
## Expected
`√W`.
## Suggested fix
optical_signal.h:26 comment `(sqrt(W))`; line 234 `" V.m⁻¹"` → `" √W"`. No consumer parses the label (checked pyspecs/utils).
## Golden impact
Stdout of every case that prints a signal (cir/*, opt/*, most tb/*): label only. Re-record.
## Related
Paper says "10^-8 V m^-1" (A6 docs).

- Discovery seeds: S2.20, S1.31 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-23.
## Verification
Code reading. Grepped pyspecs and utils for the label.
