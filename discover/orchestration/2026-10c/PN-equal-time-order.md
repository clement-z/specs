# Phase N — equal-time order by wavelength, then a per-time-step queue (branch `ws/timeorder`)

Follow `COMMON.md`. Phase id `PN`. Decisions: `backlog/DECISIONS-2026-10-21.md`, rows "Equal-time order" and
"Equal-time order, follow-up" (2026-10-24). Read first: `backlog/sprint-2026-10c/PD2-REPORT.md` item 3 (the
time-only order tried in D2, its golden and value changes, the mechanism), `PM-REPORT.md` and `PM-progress.md`
(the current queue: `src/optical_pending_queue.h`, `PendingEntryLater`, what remains of B-188),
`backlog/B-188-*.md`, `backlog/B-198-*.md` (what is NOT in scope), `src/optical_output_port.{h,cpp}`.
The host is otherwise idle; report timing ratios (median of 3 or more), never during a build.

## Scope
1. **Order equal-time pending entries by wavelength, smallest first** (the PROBE latch convention), instead of
   larger |E| first. Two entries never share (time, wavelength) (writes merge), so the order is total. The key does
   not change on a merge, so the heap stays consistent after in-place merges and the order no longer depends on
   the standard library's heap algorithm. Keep the current heap for this step.
   - Predict before re-recording (`PN-progress.md`): which golden cases change and how (listings of
     multi-wavelength nets: another last write per step; per-wavelength values at tolerance level in WDM circuits
     with zero-delay paths; `pending_mean`-type counters), using PD2's list as a starting point.
   - Re-record exactly the predicted cases (note "equal-time order by wavelength, decision 2026-10-24"); add a
     pinning case: a 4-channel CROW (`tests/bench/crow_gen.py`) with MLPROBEs at default tolerances.
   - `doc/numerics.md`: the equal-time order (by wavelength, smallest first) and its effect at tolerance level
     (B-198). CHANGELOG `[unreleased]` (Changed).
2. **Per-time-step queue** (bucket queue), replacing the heap: entries grouped by time (few distinct pending times
   per port), each bucket holding its entries in wavelength order with a direct merge lookup; push and pop about
   O(1) instead of O(log pending). Your design; it must be **bit-identical to step 1** (strict golden after step 1's
   re-record, no further re-record; `stats-compare` identical) and keep the port's statistics' meaning.
   Measure ns per event for 1/4/16/64 channels (CROW, as PM) and the C-notrace N=30 case, before (step 1) and after;
   single channel must not get slower beyond noise. If the bucket queue does not pay off, keep step 1 and report.
3. Docs: `backlog/B-188-*.md` ("what remains"), comments, CHANGELOG (performance).

## Acceptance
- `pixi run test gcc`, `pixi run test asan-clang`: all OK, no sanitizer message.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: after step 1 only the predicted cases changed; after
  step 2 nothing more.
- clang and asan-gcc: before/after `compare` (strict): the same set as gcc.
- `stats-compare` (gcc): only predicted changes (step 1), none for step 2.
- Report `backlog/sprint-2026-10c/PN-REPORT.md` committed on `ws/timeorder`: the completion marker.
