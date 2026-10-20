# B-014: Sub-tick delays are silently rounded (to zero); the OOP check meant to catch it is dead
severity: med | category: physics | effort: S | fix-class: needs-human | verdict: CONFIRMED
location: src/optical_output_port.cpp:329-331, src/devices/waveguide.cpp:72,98
## Problem
Delays become `sc_time(double, SC_NS)`, which rounds to the nearest tick (100 fs by default) without any warning.
A ring round trip of 13 fs becomes 0, so the whole ring response collapses into delta cycles at one timestamp. A
53 fs round trip becomes 100 fs, almost twice the physical delay. The OOP check
`t.value() - now.value() != delay.value()` compares integers that are equal by construction (`t = now + delay`),
so it can never fire.
## Evidence
`backlog/repro/a1-zero-tick-delay-ring.cir`: the VCD has a single timestamp `#10`. With `length=4e-6`: 40
timestamps, with a round trip of 1 tick (100 fs) instead of 53 fs. No warning in either case.
## Expected
A warning or error when the delay is quantised by more than X% or to zero, or an automatic finer resolution.
## Suggested fix
Remove the dead check (OOP lines 329-331). In the devices that build delays (waveguide.cpp:72,98, GTD), compare the
double delay with the rounded `sc_time` and warn above a threshold, with an error for non-zero → 0. The threshold
and policy are a numerics decision.
## Golden impact
None for a warning to stderr only. It depends on the policy otherwise.
## Related
A2 (waveguide). Paper benchmarks compare 100 fs and 1 fs resolution.

- Discovery seeds: S1.03 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-14.
## Verification
Ran the repro with both lengths. Code reading of sc_time addition.
