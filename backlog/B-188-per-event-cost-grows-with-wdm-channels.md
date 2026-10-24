# B-188: Per-event cost grows with the number of WDM channels
severity: low | category: performance | effort: M | fix-class: needs-design | verdict: CONFIRMED | status: fixed in c9c6a33, 27c9189, 782d85b, 70db0e1 (phase PM), 2674103, 94ca1f3 (phase N): per-event cost at 4 to 64 channels at most 14 % above one channel
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

## What remained after phase PM
The heap of the pending entries was O(log pending) per entry, and pending grows linearly with the channels, so
the cost still grew as log W (plus a larger working set): at 16 channels and N = 10, `emit_due_entry()` (heap
pop) was 34 % of the run against 17 % with one channel. The heap layout fixed the emission order of equal-time
entries (by magnitude, and where an in-place merge left an entry), so replacing it needed a decision: taken on
2026-10-24 (`backlog/DECISIONS-2026-10-21.md`, "Equal-time order" and its follow-up).

## Fix (phase N, 2026-10-24)
- 2674103: equal-time entries are ordered by wavelength, smallest first, instead of larger |E| first (option 2
  of the decision): a key that a merge does not change, so the order is fixed by the keys alone (not by the heap
  layout or the standard library). Outputs changed as predicted (re-recorded, pinning case
  `regress/pn-wdm-crow-equal-time-order`; `doc/numerics.md`, "Order within a time step"; B-198).
- 94ca1f3: the heap is replaced by a per-time-step queue (`OpticalPendingQueue`, `src/optical_pending_queue.h`):
  the entries in (time, wavelength) order in a ring buffer, one run per time step, so a pop is O(1) and most
  writes are an append (a device writes at now + its delay, and the entries of a time step arrive in wavelength
  order since the ports upstream emit them in that order); a merge into the last entry is direct, other merges
  and insertions use a binary search and shift the shorter side. Same order as 2674103, so outputs and counters
  are bit-identical to it.

ns per event, PM -> phase N step 1 -> step 2 (gcc release, CROW `--no-trace`, median of 3; the PM column is
PM's own measurement, `backlog/sprint-2026-10c/PM-REPORT.md`, another session):

| N | channels | PM | step 1 | step 2 |
|---|---|---|---|---|
| 3 | 1 | 124.0 | 119.8 | 115.6 |
| 3 | 4 | 125.1 | 116.2 | 102.6 |
| 3 | 16 | 156.6 | 132.1 | 106.8 |
| 3 | 64 | 200.4 | 157.6 | 117.7 |
| 10 | 1 | 128.9 | 128.9 | 122.9 |
| 10 | 4 | 172.7 | 146.2 | 130.7 |
| 10 | 16 | 226.7 | 188.9 | 139.6 |
| 30 (C-notrace) | 1 | 150.5 | 151.9 | 140.3 |

The wavelength order also processes fewer entries in WDM circuits (fewer half-updated values pass the gate):
N = 3, 16 channels, 12471311 -> 11783441 processed entries (-5.5 %). Cost at 16 channels / cost at 1 channel
(step 2): N = 3 0.92, N = 10 1.14; 64 channels / 1 channel (N = 3) 1.02.

## What remains
`emit_due_entry()` is 19.8 % of the run at 16 channels (N = 10) against 16.5 % with one; the rest of the small
growth is the larger working set (one entry per channel per time step in every port) and the device
memories. A write that is neither the latest nor a merge into the last entry (mixed delays into one port, or
channels arriving out of wavelength order) costs a binary search plus a shift of the entries on its shorter
side: linear in the worst case, not seen in the benchmark (the writer function `writeBefore()` is below 1.7 %
of the profile).
