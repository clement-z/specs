# B-011: OP printout shows only the last-written wavelength of each net
severity: med | category: bug | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/specs.cpp:342-356
## Problem
`printOPAnalysisResult` prints `sig->read()`, which holds only the last (λ, field) written to the net (WDM is
time-multiplexed). On WDM circuits every other wavelength is silently missing from the only OP output.
## Evidence
`backlog/repro/a1-wdm-op-printout.cir`: `ROOT/in = @ 1551 nm (0.387298, 0.150000 W)`. The 1550 nm component
(0.5 W) on the same net is printed nowhere.
## Expected
One line per (net, λ).
## Suggested fix
Print the per-λ `m_emitted_fields` of the OOP that drives each net, or keep a per-λ map per net. The output format
is a design choice.
## Golden impact
OP printout format in all cases that print an OP (cir/*, opt/*), if the format changes for single-λ nets too.
## Related
B-001, A3 S1.10 (auto-probes on multi-λ nets).

- Review ids: F-A1-11.
## Verification
Ran the repro.
