# Phase M — B-188: per-event cost vs WDM channel count (branch `ws/wdmcost`)

Follow `COMMON.md`. Phase id `PM`. Decision: `backlog/DECISIONS-2026-10-21.md`, row "B-188" (2026-10-24). Read
`backlog/B-188-*.md`, `backlog/B-015-*.md` (linear scan of pending entries), the PF06 report (how 139/246/329 ns per
event were measured: `tests/bench/`), the F-15 phase reports (`backlog/sprint-2026-10c/PD1..PD3-REPORT.md`: how
engine changes were made bit-identical and measured), `src/optical_output_port.{h,cpp}`, `src/optical_signal.h`.
The coordinator keeps the host otherwise idle while this phase runs; still report ratios (median of 3 or more).

## Scope
1. **Profile first** (gcc release, no trace): a CROW from `tests/bench/crow_gen.py` with 1, 4 and 16 channels
   (and 64 if it runs in reasonable time), same ring count. Use `perf record`/`perf report` (user-space events;
   `perf_event_paranoid` is 2) or `valgrind --tool=callgrind`. Report the per-event cost per channel count and the
   functions whose share grows with the channel count, with evidence, in `PM-progress.md` before changing code.
2. **Fix the hot spots** that make the per-event cost grow with the channel count: e.g. per-wavelength
   containers searched per event replaced by direct indexing by wavelength id (ids are small dense integers), the
   pending-entry scan (B-015). **Outputs bit-identical** (strict golden unchanged, no re-record) and **counters
   identical** (`stats-compare`). One commit per change, each with its measurement.
3. Measure after: per-event cost for 1/4/16 (/64) channels before and after, and the CROW `C-notrace N=30` case of
   `discover/scout/bench/bench_crow.py` (single channel: must not get slower beyond noise). Target: a per-event cost
   much less dependent on the channel count; report what remains and why.
4. Docs: `backlog/B-188-*.md` (findings, status), B-015 if fixed, CHANGELOG `[unreleased]` (Changed: performance).

## Acceptance
- `pixi run test gcc`, `pixi run test asan-clang`: all OK, no sanitizer message.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: all pass, **no re-record**.
- clang and asan-gcc: before/after `compare` (strict): identical.
- `stats-compare` (gcc) before/after: identical.
- Report `backlog/sprint-2026-10c/PM-REPORT.md` committed on `ws/wdmcost`: the completion marker.
