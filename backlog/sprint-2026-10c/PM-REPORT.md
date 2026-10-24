# PM report: B-188, per-event cost vs WDM channel count (branch `ws/wdmcost`)

Base: `sprint/2026-10c` at d86b816. Decision: `backlog/DECISIONS-2026-10-21.md`, row "B-186 and B-188"
(2026-10-24). Details, profiles and every measurement: `backlog/sprint-2026-10c/PM-progress.md`.
**Outputs bit-identical and counters identical at every step; no baseline re-recorded.**

## Items

| # | Item | Outcome | Commit | Tests | Golden / counters |
|---|---|---|---|---|---|
| 1 | Profile 1/4/16/64 channels (perf, gcc release, no trace) | done: four costs grow with the channel count (below) | 01e62b6 | | |
| 2a | Equal-time pending entries ordered without `abs()` | done: `PendingEntryLater`, same order decided on squared norms where sure, else `abs()` as before | c9c6a33 | gtest `OpticalOutputPort.PendingEntryOrderMatchesPairComparison` (pairwise vs `std::greater` on edge magnitudes, heap replay with in-place merges; a comparator without the `abs()` fallback fails it) | strict 278 pass; 277 same |
| 2b | Pending-entry lookup (B-015) | done: `OpticalPendingQueue` (`src/optical_pending_queue.h`), same heap, no scan for a write later than the latest pending entry of its wavelength, else a scan from the newest entries | 27c9189 | gtest `OpticalOutputPort.PendingQueueMatchesLinearScanQueue` (50000 random writes/merges/pops vs PQueue + the former `find_if`; a mutated bound fails it); `FrequencyDomainWriteMergesBehindQueueTop` adapted | strict 278 pass; 277 same |
| 2c | Per-wavelength memories indexed by wavelength id | done: `WavelengthMap<T>` (`src/utils/wavelength_map.h`) replaces every `std::map<uint32_t, field_type>` (port emitted values; coupler, crossing, merger, detector, phase shifter, PCM, power meter memories) | 782d85b | gtest `WavelengthMap.BehavesAsStdMap` (random ops vs `std::map`, same contents and order) | strict 278 pass; 277 same |
| 2d | Next notification from the queue | done: the second heap `m_pending_times` held the same times as `m_queue`; removed | 70db0e1 | 3 error-path tests notify the event directly; `Pending*` notification tests unchanged | strict 278 pass; 277 same |
| 3 | Measure after | done (table below) | 94b6319 | | |
| 4 | Docs | done: `backlog/B-188-*.md` (findings, fix, what remains; status fixed), `backlog/B-015-*.md` (fixed), `backlog.md` rows, CHANGELOG `[unreleased]` Changed (performance); stale comments | 5b6b9d7, 1477ebc | | |

### Profile findings (item 1)
Pending entries per port grow linearly with the channels (N = 3: mean 1.3 with 1 channel, 3.4 / 10.8 / 41.6 with
4 / 16 / 64): the channels share the delays, so W entries sit at each time step. Costs that grew with them:
1. `hypot` in the heap comparisons (0 % with 1 channel, 21-27 % from 4 on): with equal times the C++20 `pair`
   comparison falls through to `OpticalSignal::operator<` (`modulus()` both ways, up to four `hypot`).
2. The linear `find_if` over all pending entries per written value (9 % at 64 channels), B-015.
3. `std::map` walks per wavelength: `m_emitted_fields` in `emit_due_entry()` (about 15 of its 22 % at 64) and
   the coupler memories (about 12 of the couplers' 17 %).
4. The second heap of pending times (`fire_pending()`, up to 7 %).

### Before / after (item 3; ns per processed event, median of 3, N = 30: of 5; same session, interleaved)

| N | channels | before (d86b816) | after (70db0e1) | ratio |
|---|---|---|---|---|
| 3 | 1 | 132.3 | 124.0 | 0.94 |
| 3 | 4 | 180.2 | 125.1 | 0.69 |
| 3 | 16 | 325.4 | 156.6 | 0.48 |
| 3 | 64 | 504.3 | 200.4 | 0.40 |
| 10 | 1 | 140.7 | 128.9 | 0.92 |
| 10 | 4 | 252.9 | 172.7 | 0.68 |
| 10 | 16 | 486.7 | 226.7 | 0.47 |
| 30, C-notrace (`--no-trace`) | 1 | 159.5 | 150.5 | 0.94 |
| 30, C-notrace (`-o`, as `bench.py`) | 1 | 160.5 | 148.7 | 0.93 |

Cost at W channels / cost at 1 channel: N = 3, before 1.36 / 2.46 / 3.81 (4 / 16 / 64), after 1.01 / 1.26 / 1.62;
N = 10, before 1.80 / 3.46 (4 / 16), after 1.34 / 1.76. Single channel: faster, not slower (C-notrace -6 %).
B-015 delay line (8 x 10 cm waveguides, 20000 values 1 ps apart, about 1230 entries pending per port; ad hoc,
not committed): 0.17 s -> 0.05 s user, same VCD.

**What remains**: the heap of pending entries is O(log pending) per entry and pending grows linearly with the
channels, so a log W term remains, plus a larger working set (N = 10, 16 channels: `emit_due_entry()`, mostly the
heap pop, is 34 % of the run, 17 % with one channel). The heap's layout fixes the emission order of equal-time
entries (larger |E| first, and where an in-place merge left an entry), so a different structure (e.g. one bucket
per time step) could change the delta-cycle order of emissions in some circuits: an engine-semantics change
that needs a decision; not done.

## Decisions taken (not covered by the decision file)
- **Golden baseline not re-recorded**, as the acceptance says, although earlier phases re-recorded
  `gtest/unit_tests` for new gtests: the 3 new gtests show as notes ("new test ... PASS"); the run passes. The
  coordinator may re-record `--only '^gtest/'` at merge if the gtest list should include them.
- 2b: a first design (heap of slot numbers into stable storage, a deque of slots per wavelength: exact lookup)
  gave the same outputs but was slower with few channels (1 ch 131 -> 150, 4 ch 145 -> 161 ns/event) and was
  dropped before committing for the by-value heap plus per-wavelength latest time. The scan remains linear (from
  the newest entry) for writes not later than the latest entry of their wavelength (merges, mixed delays).
- 2c: a first version (values and presence flags in two vectors) cost +1.6 % cycles at 1 channel (two cache
  lines per access); changed to presence encoded in the slot id before committing. All `std::map<uint32_t,
  field_type>` memories were converted, not only the CROW's devices, with the same semantics (insert on read,
  increasing-id iteration). The probe's `m_listed_by_id` was left (below).
- 2d is not in the brief's examples: a hot spot that grew with the channel count (`fire_pending()` up to 7 %),
  removable with the same notifications by construction; done as its own commit and measured.
- The brief's `discover/scout/bench/bench_crow.py` does not exist (moved to `tests/bench/` in PF06): the C-notrace
  N = 30 case is `tests/bench/bench.py`'s (`30 --traceall 0 --probes 0`), measured with its `run_once()` both with
  `--no-trace` and with `-o` (as `bench.py` runs it).
- Profiling used a copy of the gcc release build configured with `-Ddebug=true` (same `-O3` code, debug info for
  inlined frames); timings only on the plain gcc release build. Comment-only commit 1477ebc changes `__LINE__`
  values in the binary; the gcc gates were re-run on it.

## Found, not done
- `Probe::m_listed_by_id` (`src/devices/probe.h:156`, MLPROBE's wavelength cache) is a `std::map<uint32_t, double>`
  with a `find` per event; `WavelengthMap` would fit (needs a `find`). Not in the CROW benchmark.
- The order of equal-time entries depends on the heap layout after in-place merges (a merge changes |E| without
  repairing the heap; existing behaviour, kept bit for bit). It also depends on the standard library's heap
  algorithm (libstdc++ vs libc++), so macOS/libc++ builds may order some equal-time emissions differently.
- `discover/01-architecture-engine.md` (step 3 of the output port) describes the port before F-15
  (`m_use_deltas`, line numbers, a thread); stale, not updated here.
- The release builds compile with `-D_GLIBCXX_ASSERTIONS=1` (Meson, from `b_ndebug=false`), which adds bounds
  checks to `std::vector` accesses in the hot paths; not measured.

## Final results
- `pixi run test gcc`: 17/17 OK (tip 1477ebc).
- `pixi run test asan-clang`, `pixi run test asan-gcc`, `pixi run test clang`: 17/17 OK each, no AddressSanitizer,
  LeakSanitizer or UBSan message in the test logs; `tests/sanitizers/lsan.supp` unchanged (code of 70db0e1; later
  commits change comments only).
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: `Summary: 278 passed (1 with notes), 0 failed,
  0 new, 0 missing, 0 skipped` (the note: 3 new gtests). **No re-record.**
- clang and asan-gcc before/after (d86b816 built in a scratch worktree, `record` each, then `compare --rtol 0
  --atol 0 --print-slack 0`): 278 passed, 0 failed, 0 new, 0 missing (notes: the 3 new gtests).
- `stats-compare` (gcc) base vs tip: `277 same (116 of them without run statistics), 0 differ, 0 new, 0 missing`.
- Before the phase (d86b816): gcc 17/17, strict golden 278 passed.
