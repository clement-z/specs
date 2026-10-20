# F-15: Engine performance pass and activity accounting (`--stats`)

value 4 | effort M | risk med (must stay bit-identical; coalescing excluded) | prerequisites: F-06 (bench to
measure against) | horizon 2

## Problem / user story
*As a user running thousands of PUF or Monte-Carlo points I want each run as cheap as possible, and I want to
know where the events went.* Profile of a 10-ring CROW at 100 fs (`bench/RESULTS.md` §3): 16 % coroutine
switching (`qt_abort`, `next_cor`, `yield`, `wait`), 13 % kernel event triggering, 11 % `hypot` from three
`std::abs(complex)` calls per emitted event in the tolerance gate (`src/optical_output_port.cpp:63, 70-71, 169`),
4.6 % the linear `find_if` over pending events (`:341`), 4.4 % `sincos` from `polar()` recomputed per event in
the waveguide although the phase depends only on wavelength (`src/devices/waveguide.cpp:60-65`), ~4 %
`malloc/free` (`sc_event_queue::notify` allocates an `sc_time` per call; `PQueue::push`), 3.8 % the kernel's
timed heap duplicated by the port's own queue. Every device process and every output port is an `SC_THREAD`
(64 of them, 0 `SC_METHOD`). There is no event/energy accounting (issue #1); the `TimeMonitor` exits the
process after 0.5 s (`src/devices/time_monitor.cpp:105-108`).

## Prior art
SystemC practice: `SC_METHOD` for stateless reactions, one timed event per module instead of `sc_event_queue`
when only the earliest matters; DES kernels keep binary heaps unless event populations are huge (Fujimoto);
Xyce/ngspice print run statistics; PhoenixSim reported energy per message (SOURCES B, C).

## Proposed design
1. **Bit-identical refactors** (verified by the golden check at `--atol 0 --strict-order`):
   - Tolerance gate on squared norms: `norm(d - l) > abstol^2` and `norm(d - l) > reltol^2 * norm(l)`
     (monotonic transform; identical decisions for finite values; add a unit test for `l == 0` and NaN).
   - Per-wavelength S-parameter caches in waveguide, coupler, phase shifter (`unordered_map<wl_id, complex>`),
     computed once per wavelength; same arithmetic, same bits.
   - One pending structure per port: `std::map<pair<sc_time, wl_id>, OpticalSignal>` ordered by time, so the
     same-time merge is a lookup and the earliest entry is `begin()`; a single `sc_event` notified for the
     earliest time (re-notified when an earlier entry arrives) replaces `sc_event_queue` and its allocations.
   - `OpticalSignal`: drop the virtual destructor and the per-copy global id bump (debug ids become opt-in).
2. **`SC_METHOD` conversion** for `OpticalOutputPort::on_data_ready` and the stateless device reactions
   (waveguide, coupler, splitter, merger, crossing, phase shifter), keeping thread style only where `wait()`
   with time is needed (detector tick, sources). Expected to remove most of the 16 % switching cost; may
   reorder evaluation within a delta cycle, so validate with the golden harness (results should be identical
   because each port writes once per delta).
3. **`--stats`**: counters per port (`delayedWrite` calls, emitted events, suppressed by abstol/reltol, merged
   same-time events, max pending size), per device type, per source energy (`sum P dt`), per detector charge,
   wall/user time, VCD bytes; printed as a table and written to `<trace>.stats.json`; `TimeMonitor` becomes a
   periodic progress line (`--progress`) instead of an exit.
4. Out of scope here (needs-human, F-08): event coalescing windows, tolerance semantics, time quantisation.

## Touch points
`src/optical_output_port.{h,cpp}` (`:19-37` constructor, `:60-73`, `:95-188`, `:326-358`) · `src/optical_signal.h`
(`:52-56` copy/id, virtual dtor) · `src/devices/waveguide.cpp:51-100`, `directional_coupler.cpp`,
`phaseshifter.cpp`, `merger.cpp`, `splitter.cpp`, `crossing.cpp` · `src/specs.cpp:41-60, 240` (print stats) ·
`src/devices/time_monitor.{h,cpp}` · `src/utils/pqueue.h` (retire).

## Validation plan
- After each refactor: `run_golden.py check --atol 0 --strict-order` passes; `bench/bench_crow.py` cases A, C,
  D before/after (target >= 1.5x on case C, N = 30).
- Unit tests for the squared-norm gate edge cases and for the single-event rescheduling (earlier insert,
  same-time merge, DC `reset()`).
- `--stats` totals match the VCD record counts for a probed net; energy accounting conserves power through a
  lossless coupler (sum of outputs equals input).

## Effort breakdown
Gate + caches 1 d · queue/event rewrite 3 d · `SC_METHOD` conversion 3 d · stats + progress 2 d · validation 2 d.

## Open questions for the maintainer
- Is bit-identity the acceptance criterion, or is the golden harness's default `rtol 1e-9` enough?
- Keep `OpticalSignal::m_id` (debug) behind a compile-time flag?
- Does the house style accept `SC_METHOD` reactions named `on_<port>_changed()` (CLAUDE.md names threads)?
