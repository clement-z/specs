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

Exact list from the second check (25 cases, stdout label only): cir/add_drop, cir/add_drop_pyspecs, cir/subckt_tests, opt/{tracefile-o, tol-resolution-overrides, mode-sampled-time, mode-fd, multi-file, vci}, tb/{wg, waveguide, merg, merger, dc, directional_coupler, spli, splitter, det, detector, pcm, mzi, ring, lambda, phaseshifter, ps}. No consumer parses the label (pyspecs only echoes stdout).
## Related
Paper says "10^-8 V m^-1" (A6 docs).

- Discovery seeds: S2.20, S1.31 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-23.
## Verification
Code reading. Grepped pyspecs and utils for the label.

**Independent second check (V3, of F-A1-23): CONFIRMED.** Method: applied both edits, rebuilt, golden check --atol 0, then a full `record` compared with a baseline in which V.m⁻¹ was replaced by √W; grepped pyspecs/utils

Exactly 25 cases change, stdout only, label only (236/236 differing lines differ only by the label; recorded tree == substituted baseline except the known-nondeterministic tb/det VCD). The precise list for the golden impact text is cir/add_drop, cir/add_drop_pyspecs, cir/subckt_tests, opt/{tracefile-o, tol-resolution-overrides, mode-sampled-time, mode-fd, multi-file, vci}, and tb/{wg, waveguide, merg, merger, dc, directional_coupler, spli, splitter, det, detector, pcm, mzi, ring, lambda, phaseshifter, ps}. Not affected: cir/add_drop_sub, cir-json/*, tb/ac_*, tb/mesh, cli/*. No consumer parses the label (pyspecs only echoes stdout).
