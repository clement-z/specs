# B-014: Sub-tick delays are silently rounded (to zero); the OOP check meant to catch it is dead
severity: med | category: physics | effort: S | fix-class: needs-human | verdict: CONFIRMED | status: fixed in c527d7d (warnings, sprint 2026-10b P3) and a23e4f9 (error for netlist waveguides, option (b), sprint 2026-10c phase A)
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

## Sprint 2026-10b P3 (needs-human: the error part)
Done in c527d7d: the dead check is removed; each waveguide checks its group delay once before the simulation,
only where delays are honoured (event-driven ports in a time-domain analysis: `.tran`, testbenches; not `.op`,
`.dc`, `-m fd`), against the port time step (engine step × `.options resolution`), and warns on stderr when the
delay is rounded by more than 10 % (`OpticalOutputPort::DELAY_ROUNDING_WARN_THRESHOLD`, chosen from the delays of
every golden case, testbench and repro: realistic parameters round by at most 5.95 %, 2.27 ps at 1 ps by 11.7 %,
the B-014 case by 87 %). A non-zero delay that rounds to 0 gets its own warning.

Held back: the decided **error** for a non-zero delay that rounds to 0. It makes the existing
`regress/b031-mzi-bidir` exit 1: its MZI-internal arm waveguides (43.4 fs, created by `MZI::init()`) round to 0
at the default 100 fs (harmless there: feed-forward). The phase's predicted golden impact was "stderr warnings
only, no value changes", so the error was not merged. Ready on branch `P3-ready/b014-error` (cfe0eb0 + cc1614c, on
top of c527d7d; 134/134 golden `--atol 0` with its own baseline): the error, `err/b014-delay-rounds-to-zero`, and
b031 run at `.options timescale=-14` (arms 40 fs / 90 fs, no message, same OP values; stdout gains the directive
and the timescale lines, VCD timescale 10 fs). Options for the maintainer: (a) take the branch as is; (b) keep
the warning for device-internal delays (MZI arms) and error only for netlist waveguides; (c) keep the warning.

## Sprint 2026-10c phase A (fixed, option (b))
Maintainer decision (2026-10-21, follow-up round): option (b). Done in a23e4f9, adapted from
`P3-ready/b014-error` without its timescale change to b031. `WaveguideBase::m_from_netlist` records where the
waveguide comes from: the parser sets it in `WGElement::create()` (netlist lines, also inside a `.subckt`); every
other waveguide (MZI arms, OCTANE, CROW, meshes, testbenches, C++ users) keeps the default `false`.
`checkDelayRounding(name, delay, zero_is_error)`: a non-zero delay that rounds to 0 is an error (exit 1, before
the simulation) for netlist waveguides and the existing warning otherwise. `regress/b031-mzi-bidir` is unchanged
(its 18 MZI-arm warnings stay). Golden: `regress/b014-delay-rounds-to-zero` became
`err/b014-delay-rounds-to-zero`; new `err/b014-subckt-wg-rounds-to-zero` and
`regress/b014-device-internal-rounds-to-zero`.
