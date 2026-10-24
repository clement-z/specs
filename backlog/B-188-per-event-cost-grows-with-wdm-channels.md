# B-188: Per-event cost grows with the number of WDM channels
severity: low | category: performance | effort: M | fix-class: needs-design | verdict: CONFIRMED | status: fixed in c9c6a33, 27c9189, 782d85b, 70db0e1 (phase PM): growth reduced, a log(pending) term remains
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

## Findings (phase PM, 2026-10-24)
Profiled with perf on the CROW (`tests/bench/crow_gen.py`, 3 and 10 rings, 1/4/16/64 channels, no trace, gcc
release; `backlog/sprint-2026-10c/PM-progress.md`). The pending entries of a port grow linearly with the channels
(N = 3: mean 1.3 with 1 channel, 41.6 with 64), and four costs grew with them:
1. the pending-queue comparison of equal-time entries (every time step with WDM: the channels share the delays)
   went through `OpticalSignal::operator<`, up to four `std::abs()` (`hypot`) per heap comparison: about 25 % of
   the run from 4 channels on, 0 % with one;
2. the linear `find_if` over all pending entries for each written value (B-015);
3. `std::map` per-wavelength memories (output port emitted values, device input memories);
4. a second heap of the pending times next to the queue.

## Fix
- c9c6a33: `PendingEntryLater`, the same order decided on squared norms where they are sure (else `abs()` as
  before): same comparison results, same heap and order.
- 27c9189: `OpticalPendingQueue`: no scan when the write is later than the latest pending entry of its
  wavelength (most writes), else a scan from the newest entries (B-015).
- 782d85b: `WavelengthMap`, per-wavelength memories as a vector indexed by the wavelength id, same semantics and
  iteration order as the maps.
- 70db0e1: the next notification is the queue's earliest entry (the second heap held the same times).

ns per event, before -> after: N = 3: 1 ch 132 -> 124, 4 ch 180 -> 125, 16 ch 325 -> 157, 64 ch 504 -> 200;
N = 10: 1 ch 141 -> 129, 4 ch 253 -> 173, 16 ch 487 -> 227; CROW C-notrace N = 30 (1 channel) 160 -> 150.
Cost at 16 channels / cost at 1 channel: N = 3 2.46 -> 1.26, N = 10 3.46 -> 1.76. Outputs bit-identical (strict
golden on gcc; clang and asan-gcc identical before/after), `--stats` counters identical (`stats-compare`).

## What remains
The heap of the pending entries is O(log pending) per entry, and pending grows linearly with the channels, so
the cost still grows as log W (plus a larger working set): at 16 channels and N = 10, `emit_due_entry()` (heap
pop) is 34 % of the run against 17 % with one channel. The heap layout fixes the emission order of equal-time
entries (by magnitude, and where an in-place merge left an entry), so replacing it (e.g. one bucket per time
step) could change the delta-cycle order of emissions in some circuits: an engine-semantics change that needs
a decision, not done.
