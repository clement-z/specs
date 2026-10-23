# B-188: Per-event cost grows with the number of WDM channels
severity: low | category: performance | effort: M | fix-class: needs-design | verdict: CONFIRMED | status: open
location: src/optical_output_port.{h,cpp} (per-wavelength containers), devices with per-wavelength memories

## Problem
Found in phase PF06 (`backlog/sprint-2026-10c/PF06-REPORT.md`, "Found, not done"). On the CROW benchmark
(`tests/bench/`, N = 3-10 rings, no trace, gcc release), the CPU time per processed event grows with the number of
wavelengths carried by the circuit: about 139 ns per event with 1 channel, 246 ns with 4, 329 ns with 16. The
event counts scale as expected (the `wdm-events` ratio check stays near 1), so the cost is per event, not more
events.

## Expected
A per-event cost roughly independent of the channel count, or a documented reason for the growth.

## Suggested fix (needs design)
Profile a 1- vs 16-channel run: likely candidates are per-wavelength maps or vectors searched per event in the
output port (pending-entry lookup, emitted values per wavelength) and in the devices' per-wavelength memories;
B-015 (linear scan of the pending entries) is related.

## Golden impact
None expected for a pure performance change (bit-identical outputs, counters identical).

## Related
B-015, F-15 (engine performance), F-22 (WDM vectorised events).
