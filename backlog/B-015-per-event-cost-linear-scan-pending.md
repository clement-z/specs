# B-015: Per-event cost: linear scan of the pending queue, plus a heap allocation per notify
severity: med | category: perf | effort: M | fix-class: needs-design | verdict: CONFIRMED | status: fixed in 27c9189 (phase PM, B-188), after 67d1702 (sprint 2026-10c D2: no heap allocation per notify, sc_event_queue replaced); 70db0e1 also drops the second heap of pending times
location: src/optical_output_port.cpp:341-343,348-349
## Problem
Every event-driven write does a `find_if` over all pending entries of the port. That is O(n) per write and O(n²)
for long delay lines or bitstreams with many events in flight. Each push also calls `sc_event_queue::notify`,
which does `new sc_time` (kernel sc_event_queue.cpp:notify) and keeps a second heap. So each event pays for two
O(log n) heaps, one heap allocation, and the linear scan.
## Evidence
`perf record` on an 8-waveguide chain with ~4900 events in flight per port: 6.4 % of total time in
`delayedWriteEventDriven` (the run is dominated by parser list handling, see cross-area notes). Code reading gives
the scaling.
## Expected
Sub-linear merge lookup.
## Suggested fix
Index pending entries by (time, λ), for example `std::map<sc_time, small_vector<entry>>`, or an
`unordered_map<pair<time,λ>, idx>` next to the heap. Take the fast path when `t` is later than the latest queued
time. Optionally replace `sc_event_queue` with a single `sc_event` re-notified for the queue head. Event semantics
must stay identical.
## Golden impact
None if semantics are preserved.
## Related
B-001 (its fix adds the same scan to the FD path).

- Discovery seeds: S1.19 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-15.
## Verification
Profiled with perf. Read the kernel source.

## Fix (phase PM, 2026-10-24)
27c9189: `OpticalPendingQueue` (`src/optical_pending_queue.h`) keeps, per wavelength id, the number of pending
entries and the latest time among them. A value written later than that time cannot merge, so there is no scan
(the case of delay lines: a device writes at now + its delay); otherwise (a merge, or a write before the latest
entry, e.g. paths of different delays into one port) the heap is scanned from its end, where the newest entries
are; at most one entry matches. Worst case still linear in the pending entries, for writes that are not later
than the latest of their wavelength. 70db0e1: the second heap (`m_pending_times`, the times not yet fired) is
gone; the next notification is the queue's earliest entry. Same heap, same order, same notifications: outputs
and counters identical.
Measured: a chain of 8 waveguides of 10 cm fed by 20000 values 1 ps apart (1230 entries in flight per port on
average, 1435 at most; ad hoc netlist, not committed): 0.17 s -> 0.05 s user, same VCD.
