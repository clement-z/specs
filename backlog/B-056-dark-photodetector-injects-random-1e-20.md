# B-056: A dark photodetector injects a random 1e-20 field: nondeterministic readout and a VCD write every Ts
severity: med | category: determinism | effort: XS | fix-class: needs-human | verdict: CONFIRMED
location: src/devices/detector.cpp:74-75
## Problem
`if (norm(total_field) == 0) total_field = 1e-20*m_rngDist(m_rngGen);` runs even with `noiseBypass`, which is the default. Every Ts sample of a dark detector produces a different random readout around 1e-41..1e-40 A.
- The VCD differs from run to run.
- The readout VCD gets one entry per Ts while the detector is dark.

The intent is undocumented: perhaps avoiding log(0) in post-processing.
## Evidence
`backlog/repro/a3-pdet-dark-nondet.cir` (dark PDET, `.tran 100e-12`), run twice and diffed:
```
< r5.732407437321889e-42 aaaaa      > r1.540104175185926e-41 aaaaa
< r4.810381871310285e-41 aaaaa      > r2.94784722471562e-42 aaaaa
```
100 `readout` entries in 100 ps.
## Expected
A dark detector with noise bypassed reads exactly `m_darkCurrent_A`, deterministically.
## Suggested fix
Delete detector.cpp:74-75. This needs human confirmation that nothing (pyspecs notebooks, log plots) relies on non-zero readouts. If something does, the floor should be explicit and deterministic.
## Golden impact
None, since no golden detector is ever enabled.
## Related
B-055, B-050.

- Discovery seeds: S2.17 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-10.
## Verification
I ran the repro twice with the release binary and diffed the VCDs.
