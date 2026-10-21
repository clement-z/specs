# B-057: A PROBE on a WDM net traces only the last event (arbitrary λ), not the net's state
severity: med | category: bug | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/devices/probe.cpp:33-46, src/specs.cpp:280-298
## Problem
`Probe::on_port_in_changed()` writes power, abs, phase and wavelength of whichever event arrived last. On a net that carries several wavelengths, the `power` trace is the power of one carrier, and the `wavelength` trace hops between carriers. Since VCD keeps one value per timestep, which carrier you see depends on delta ordering.
- Every `traceall` auto-probe has this behaviour.
- PWR (PowerMeter) keeps per-λ memory and is correct.
## Evidence
`backlog/repro/a3-probe-multilambda.cir` (1 mW @1550 nm at 1 ps and 2 mW @1551 nm at 2 ps into a MERGER):
- At 2 ps `ROOT/PROBE1.power` = 0.0009999999999999998 and `.wavelength` = 1.551e-06.
- `ROOT/PWR1.power` = 0.0015 (the correct total).
## Expected
Documented, λ-independent semantics: for example total power Σ|E_k|² for `power`, and per-λ or "multi" for phase and wavelength. Or point users to MLPROBE.
## Suggested fix
Design choice. One option is to give Probe a per-λ memory like PowerMeter and trace total power, plus phase/abs only when a single λ is present. Another is to keep the current semantics but document them and warn once when a second λ shows up on a probed net.
## Golden impact
Unknown until a design is chosen. The single-λ golden cases would be unchanged if total power = |E|² for one carrier.
## Related
B-048, B-062.

- Discovery seeds: S1.10 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-11.
## Verification
I ran the repro with the release binary.
## Status
Open for the VCD. Related (sprint 2026-10b P6, F-05, 824702e): the CSV/`.npz` tables (`--format csv|npz`) have one
row per time step, probe and wavelength, so a PROBE on a WDM net keeps every carrier there; the VCD variables of
a PROBE still show the last event.
