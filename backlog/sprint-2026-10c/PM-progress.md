# PM progress: B-188, per-event cost vs WDM channel count (branch `ws/wdmcost`)

Heartbeat: `71e81c94` (PM-heartbeat, every 5 min, expires 48 h).
Base: `sprint/2026-10c` at d86b816. Gates before any change: `pixi run test gcc` 17/17 OK; strict golden
(`--rtol 0 --atol 0 --print-slack 0`) 278 passed, 0 failed/new/missing; counters recorded with the base build
(`stats-record`, 277 cases, 161 with statistics) for the later `stats-compare`.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 1 | Profile 1/4/16/64 channels | done | (this commit) | findings below |
| 2a | Pending-queue order without `abs()` | done | c9c6a33 | 4 ch 181 -> 143, 16 ch 324 -> 246, 64 ch 507 -> 391 ns/event (N = 3) |
| 2b | Pending-entry lookup (B-015) | done | 27c9189 | 16 ch 246 -> 224, 64 ch 389 -> 343 ns/event; 1 ch within noise (+1 %) |
| 2c | Per-wavelength containers indexed by id | done | 782d85b | 16 ch 225 -> 175, 64 ch 346 -> 223 ns/event; 1 ch unchanged |
| 2d | Next notification from the queue (no second heap) | done | 70db0e1 | all channel counts faster; C-notrace N=30 158 -> 148 ns/event |
| 3 | Measure after | done | (item 3 commit) | table below |
| 4 | Docs | todo | | |

## Item 1: profile (gcc release, no trace)

Method. Circuit: `tests/bench/crow_gen.py N --traceall 0 --probes 0 --nwl W` (the paper's CROW, ten 1 ns pulses,
all W channels pulsed together on a 100 GHz grid), run with `specs -f crow.cir --no-trace`. Times: user time of the
child (`os.wait4`, `tests/bench/bench.py`'s `run_once`), median of 3 runs, idle host; per-event cost = user time /
processed pending entries (`--stats`). Profiles: `perf record -e cycles:u` (flat, 2 kHz) and `--call-graph dwarf`
on the same release build and on a copy configured with `-Ddebug=true` (release `-O3` code plus debug info, for
inlined frames and source lines; same code generation).

### Per-event cost (base build d86b816)
| N | channels | processed | user s | ns / event |
|---|---|---|---|---|
| 3 | 1 | 718030 | 0.096 | 134 |
| 3 | 4 | 2918458 | 0.528 | 181 |
| 3 | 16 | 12471311 | 4.020 | 322 |
| 3 | 64 | 48008518 | 24.39 | 508 |
| 10 | 1 | 8394360 | 1.158 | 138 |
| 10 | 4 | 34976897 | 8.813 | 252 |
| 10 | 16 | 148242888 | 71.83 | 485 |
| 30 (C-notrace) | 1 | 12289043 | 1.958 (median of 5) | 159 |

Pending entries per output port when a value is written (`--stats` `pending_mean` / `pending_max`, N = 3):
1.3 / 2 (1 channel), 3.4 / 8 (4), 10.8 / 34 (16), 41.6 / 143 (64): the queue of a port grows linearly with the
channel count (every channel has its own entries in flight, at the same times: the channels share the delays).

### Flat profile, share of user cycles (N = 3)
| Function (self) | 1 ch | 4 ch | 16 ch | 64 ch |
|---|---|---|---|---|
| `__hypot_finite` + `hypotf32x` (complex `abs()`) | 0 % | 21.5 % | 25.7 % | 26.5 % |
| `OpticalOutputPort::emit_due_entry()` | 17.6 % | 20.8 % | 23.7 % | 22.0 % |
| `OpticalOutputPort::delayedWriteEventDriven()` | 14.5 % | 10.8 % | 11.6 % | 13.9 % |
| `DirectionalCouplerUni::on_port_in{1,2}_changed()` | ~4 % | 7.8 % | 11.4 % | 17.2 % |
| `OpticalOutputPort::fire_pending()` | <1 % | 2.6 % | 3.8 % | 4.0 % |
| SystemC kernel (`sc_event::trigger`, `crunch`, `sc_ppq_base`, signal write/update) | ~40 % | ~25 % | ~15 % | ~10 % |

Since the per-event cost grows 3.8x from 1 to 64 channels while the kernel's share falls, every row above but
the kernel grows in absolute cost per event. Attribution (call graphs with inlined frames, source lines, 16 and
64 channels):

1. **Pending-queue comparisons compute `abs()`** (about 25 % of the time from 4 channels on, 0 % with one):
   all `__hypot_finite` samples come from the heap operations of `m_queue` (`PQueue<pair<sc_time,
   OpticalSignal>>` with `std::greater`): 17 % from `pop()` in `emit_due_entry()`, 7 % from `push()` in
   `delayedWriteEventDriven()` (16 channels). The C++20 `pair` comparison compares the times, then, **when the
   times are equal**, the signals with `OpticalSignal::operator<`, which compares `modulus()` (`std::abs`,
   `hypot`) of both, in both directions (`__synth3way`): up to four `hypot` per comparison and O(log n)
   comparisons per push and pop. With one channel two entries never share a time (same time and wavelength are
   merged), so the second key is never reached; with W channels the W entries of a time step all tie on time.
2. **Linear scan of the pending entries** (B-015): `find_if` over `m_queue` in `delayedWriteEventDriven()`
   (`optical_output_port.cpp:339` and the loop in `stl_algobase.h`: 9.3 % at 64 channels): O(pending) per
   written value, and pending grows linearly with the channels (above).
3. **`std::map` per wavelength**: `m_emitted_fields` in `emit_due_entry()` (`try_emplace`: `stl_tree.h` lines,
   about 15 of its 22 % at 64 channels) and the coupler memories `m_memory_in1/2` in
   `DirectionalCouplerUni::on_port_in*_changed()` (`operator[]`: `stl_tree.h`/`stl_map.h`, about 12 of its 17 %):
   O(log W) tree walks with a cache miss per level, several per event.
4. **`m_pending_times` heap** (`fire_pending()`, `stl_heap.h`): O(log pending) per event; inherent to a heap
   of pending times, grows slowly (log W). Not changed.

Plan (each change bit-identical by construction, measured on its own): (2a) compare equal-time entries on their
magnitudes without `hypot` where the result is certain (exactly the same comparison results, so the same heap
and the same emission order); (2b) an index of the pending entries by (time, wavelength id) for the merge
lookup; (2c) per-wavelength containers indexed by the (dense) wavelength id in the output port and the devices.

## Item 2a: pending-queue order without `abs()`

`PendingEntryLater` (`optical_output_port.h`) replaces `std::greater<pair<sc_time, OpticalSignal>>` as the
comparator of `m_queue` (`PQueue` takes the comparator as a template parameter now). For equal times it decides
on the squared norms when `compareSquaredMagnitudes()` is sure and falls back to `modulus()` otherwise: the
same result as the former comparison for every pair of entries, so the same heap layout and emission order.
Test: gtest `OpticalOutputPort.PendingEntryOrderMatchesPairComparison` (every pair of a corpus of edge
magnitudes, about 1.9 M comparisons both ways, equal and different times; then 20000 random pushes, in-place
replacements and pops on two heaps, one per comparator: same layout after every step). Mutation check: the
comparator without the `abs()` fallback (norms only) fails it (0 vs 5e-324).
Gates: `pixi run test gcc` 17/17; strict golden 278 passed (1 note: the new gtest, baseline not re-recorded);
`stats-compare` vs base: 277 same, 0 differ.

| N = 3, ns/event (median of 3) | 1 ch | 4 ch | 16 ch | 64 ch |
|---|---|---|---|---|
| base | 129.5 | 180.6 | 324.0 | 506.8 |
| 2a | 130.9 | 142.9 | 246.0 | 390.8 |

## Item 2b: pending-entry lookup (B-015)

New `OpticalPendingQueue` (`src/optical_pending_queue.h`, which now also holds `compareSquaredMagnitudes()` and
`PendingEntryLater`): the same by-value heap (`PQueue<pair, PendingEntryLater>`, so the same layout and order),
plus per wavelength id the number of pending entries and the latest time among them. `find(t, wl)` answers
"none" without a scan when t is later than that time (most writes: a device writes at now + its delay), else
scans the heap from its end (the newest entries; at most one match, so the order does not matter).
The latest time is exact while entries of that wavelength are pending (entries leave in time order, so the
one at the latest time leaves last), and only needs to be an upper bound for correctness.

First design, dropped: a heap of slot numbers into stable storage plus a deque of slots per wavelength
(exact lookup in O(entries of that wavelength)). Same outputs, but slower where pending is small: 1 ch
131 -> 150, 4 ch 145 -> 161 ns/event (indirection in every heap comparison, deque bookkeeping), no gain at
16/64 channels beyond the version kept.

Tests: gtest `OpticalOutputPort.PendingQueueMatchesLinearScanQueue` (50000 random writes with merges, at
times before, at and after the latest pending ones, and pops, against PQueue + the former linear `find_if`:
same lookups, size and next entry at every step, also across `clear()`); a mutated bound (latest time not
updated) fails it. `FrequencyDomainWriteMergesBehindQueueTop` adapted to `find()` (the queue is not iterable).
Gates: `pixi run test gcc` 17/17; strict golden 278 passed (note: the new gtests); `stats-compare` vs base:
277 same.

| ns/event (median of 3; N = 30: of 5) | 1 ch | 4 ch | 16 ch | 64 ch | C-notrace N=30 |
|---|---|---|---|---|---|
| base | | | | | 159.0 |
| 2a | 130.9 | 147.3 | 245.9 | 389.0 | 161.0 |
| 2b | 135.1 | 143.2 | 224.4 | 342.8 | 162.5 |

`perf stat` (N = 30, 1 channel, 3 runs): 2a 7.13 G cycles, 14.51 G instructions; 2b 7.19 G cycles (+0.9 %,
run-to-run +-1.2 %), 14.78 G instructions (+1.9 %: the per-wavelength bookkeeping, about 20 instructions per
event; the growth path is out of line).

## Item 2c: per-wavelength containers indexed by the wavelength id

`WavelengthMap<T>` (`src/utils/wavelength_map.h`): one vector of (id, value) indexed by the wavelength id, the
slot of an absent id marked by an id of `UINT32_MAX`; `operator[]` (adds `T()` also on a read, as the map did),
`try_emplace()`, `clear()`, `size()`, iteration over the present ids in increasing order (the map's order: the
sums over wavelengths in the detector, power meter and PCM, and the writes of the phase shifters per
wavelength, keep their order). It replaces every `std::map<uint32_t, field_type>`: `OpticalOutputPort::
m_emitted_fields` and the input memories of the couplers (uni, bi), crossings (uni, bi), merger, detector,
phase shifters (uni, bi), PCM device and power meter. References into it are invalidated when a new id grows
the vector (a map's were not): checked every use, none is held across an insertion into the same container.
The probe's `m_listed_by_id` (`std::map<uint32_t, double>`, MLPROBE's cache, a `find` per event) is left as it is
(not in this benchmark; "Found, not done").

First version, changed before the commit: values and presence flags in two vectors. Same outputs, but +1.6 %
cycles with 1 channel (N = 30: 7.23 G vs 7.12 G; +10 M L1 misses, two cache lines per access where a one-node
map touched one). The presence in the id field brought it back to the base (7.05 G cycles, 14.51 G
instructions; base 7.05 G, 14.53 G).

Test: gtest `WavelengthMap.BehavesAsStdMap` (`src/test/utils/wavelength_map.cpp`: 20000 random writes, reads
through `operator[]`, `try_emplace()` and `clear()` against `std::map`; same size, contents and iteration order
after every step). Gates: `pixi run test gcc` 17/17; strict golden 278 passed; `stats-compare` vs base: 277 same.

| ns/event (median of 3; N = 30: of 5) | 1 ch | 4 ch | 16 ch | 64 ch | C-notrace N=30 |
|---|---|---|---|---|---|
| base (same session) | | | | | 160.7 |
| 2b | 135.1 | 141.9 | 225.4 | 346.1 | 160.7 |
| 2c | 129.5 | 134.3 | 175.1 | 222.9 | 161.1 |

## Re-profile after 2c, and item 2d

After 2a-2c (N = 3, flat profile, debug-info build): 1 ch: kernel (`sc_event::trigger`, `crunch`, ...) about 45 %,
`emit_due_entry()` 14.6 %, `delayedWriteEventDriven()` 13.6 %; 64 ch: `emit_due_entry()` 36 %,
`delayedWriteEventDriven()` 15 %, `fire_pending()` 7.2 %, kernel about 30 %. What grows now is the heap work:
`pop()`/`push()` of `m_queue` (moves of the 32-byte entries, `PendingEntryLater` on equal times: source lines
`stl_pair.h`, `stl_heap.h`, `optical_pending_queue.h:23-27`) and the second heap `m_pending_times` in
`fire_pending()`, both O(log pending) with pending growing linearly with the channels.

Item 2d: `m_pending_times` (F-15, the times not yet fired, replacing `sc_event_queue`) always holds the same
times as `m_queue`: both get one per new entry (merges add none), lose one per fired entry
(`emit_due_entry()` pops the queue, `fire_pending()` popped the time) and are cleared together (`drop_queue()`).
So its top is `m_queue.top().first` (the heap keeps the earliest time on top, also after in-place merges, which
change only magnitudes). `notify_pending()` now compares with the queue's top before the entry is pushed;
`fire_pending()` notifies the queue's earliest time. Same notifications, one heap push and pop less per entry.
Tests: the three error-path tests that called `notify_pending()` without an entry (empty queue, desync) now notify
`m_pending_event` directly; `KeepsValueQueuedBeforeThreadStart` calls `notify_pending()` before pushing. The
pending-notification tests (`Pending*`) pass unchanged. Gates: `pixi run test gcc` 17/17; strict golden 278
passed; `stats-compare` vs base: 277 same.

| ns/event (median of 3; N = 30: of 5) | 1 ch | 4 ch | 16 ch | 64 ch | C-notrace N=30 |
|---|---|---|---|---|---|
| base (same session) | | | | | 158.1 |
| 2c | 128.1 | 132.9 | 175.8 | 223.4 | 160.5 |
| 2d | 122.6 | 125.1 | 161.7 | 199.8 | 148.1 |

## Item 3: before / after (base d86b816 vs 70db0e1, same session, interleaved runs)

CROW `crow_gen.py N --traceall 0 --probes 0 --nwl W`, `--no-trace` (C-notrace also with `-o`, as `bench.py`
runs it), gcc release, user time of the child, median of 3 (N = 30: of 5), ns per processed event:

| N | channels | before | after | after / before |
|---|---|---|---|---|
| 3 | 1 | 132.3 | 124.0 | 0.94 |
| 3 | 4 | 180.2 | 125.1 | 0.69 |
| 3 | 16 | 325.4 | 156.6 | 0.48 |
| 3 | 64 | 504.3 | 200.4 | 0.40 |
| 10 | 1 | 140.7 | 128.9 | 0.92 |
| 10 | 4 | 252.9 | 172.7 | 0.68 |
| 10 | 16 | 486.7 | 226.7 | 0.47 |
| 30 (C-notrace, `--no-trace`) | 1 | 159.5 | 150.5 | 0.94 |
| 30 (C-notrace, `-o traces/crow`) | 1 | 160.5 | 148.7 | 0.93 |

Growth with the channel count (cost at W channels / cost at 1 channel): N = 3: before 1.36 (4), 2.46 (16),
3.81 (64); after 1.01 (4), 1.26 (16), 1.62 (64). N = 10: before 1.80 (4), 3.46 (16); after 1.34 (4), 1.76 (16).
Event counts unchanged (counters identical).

What remains (perf, N = 10, 16 channels, after): `emit_due_entry()` 34 % (1 ch: 17 %), mostly the heap pop of
`m_queue` (sift-down: moves of the 32-byte entries, `stl_pair.h:627`, `stl_iterator.h`; `PendingEntryLater` on
equal times) and the first touch of the queue (`optical_output_port.cpp:157,167`); `delayedWriteEventDriven()`
15 % (heap push, `stl_heap.h:215`); `fire_pending()` 6 %. The heap is O(log pending) per entry and the pending
entries of a port grow linearly with the channels (N = 10: mean 2.3 with 1 channel, 7.7 with 4, 30.9 with 16;
N = 3: 1.3, 3.4, 10.8, 41.6 with 64), so a log W term remains, plus a larger working set. Its layout fixes the
emission order of equal-time entries (by magnitude, and after in-place merges, by where the merged entry sits),
so a structure other than this heap (e.g. one bucket per time) would change the delta-cycle order of the
emissions of some cases: an engine-semantics change, not done here (see the report).
