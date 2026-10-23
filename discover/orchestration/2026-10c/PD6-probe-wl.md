# Phase D6 — PROBE always traces its wavelength (branch `ws/probewl`)

Follow `COMMON.md` and `PD-common-f15.md` (builds, strict golden, clang/asan before/after, counters, report).
Phase id: `PD6`. Maintainer decision (2026-10-23): a PROBE always traces the wavelength it records. Since D5 a
PROBE on a multi-wavelength net traces one wavelength chosen by a rule (the swept one in a `.dc` wavelength or
frequency sweep, otherwise the smallest present); its `wavelength` signal is what tells which one, and it is the
x-axis of a transmission sweep. Last item before the v0.3.0 release.

## To do
1. `PROBE` elements and the automatic `PROBE{<net>}` probes (traceall, and `.save <net>` without a quantity)
   always trace `wavelength`, whatever their arguments.
   - The 4th positional argument and the keywords `wl` / `wavelength` stay accepted, so existing netlists run:
     a true value changes nothing; a false value is ignored with **one warning per run** on stderr (all probes
     together, in name order, e.g. "the wavelength of a PROBE is always traced since v0.3: wl=0 ignored for
     PROBE1, PROBE5"). Nothing per event.
   - `.save QTY(net)` targets (`P(...)`, `E(...)`, `PHI(...)`, `WL(...)`) keep exactly the quantity requested
     (SPICE semantics); `doc/netlist.md` says that `WL(net)` adds the wavelength.
   - C++ users: the `Probe` constructor's `trace_wavelength` parameter: keep it (API compatibility) but document
     that the netlist always sets it; do not break `src/tb/` testbenches (their outputs must stay identical).
2. Docs: `doc/netlist.md` (PROBE syntax `[power [abs [phase]]]`, the 4th argument / `wl` as accepted and ignored),
   element table, `CHANGELOG.md` `[unreleased]` (Changed), unit/golden tests.

## Predicted golden impact (verify, state it in `PD6-progress.md` before changing outputs)
- `regress/b153-zerotol-lossy-ring-tran` (`probe1 out 0 1 0 0`) and `regress/probe-latch-tran` (`probe5 c wl=0`):
  each gains the probe's `wavelength` signal in the VCD and the warning line on stderr; nothing else changes.
  Search all netlists (`tests/golden/netlists`, `circuits`, cases with inline args) for other false 4th arguments
  or `wl=0`/`wavelength=0`/`false` on PROBE.
- New golden case for the warning (positional and keyword forms). gtest list if tests are added.
- Everything else strictly bit-identical, counters identical.
