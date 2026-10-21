# Phase D1 — `--stats` and ablation switches (branch `ws/stats`) — DRAFT

Follow `COMMON.md` and `PD-common-f15.md`. Phase id: `PD1`.
- `--stats`: per run, aggregated from per-port counters (no globals): values scheduled, coalesced (same-time merges),
  processed, emitted, suppressed (by abstol / by reltol / by snap), max and mean pending-queue length, last
  emission time, wall time; written as `<trace>.stats.json` next to the trace (and printed on request); scope per
  [Q5] (per-device-type breakdown, energy/charge accounting yes/no).
- Documented expert switches for ablation [Q4]: no coalescing (equal-time entries kept, FIFO ties) and an event
  cap; form (`.options`, CLI) per [Q4]; documented in `doc/numerics.md` as expert options.
- Progress reporting (`--progress`) only if [Q6] says so.
- Expected golden impact: none for existing cases (stats off by default); `cli/help`/`cli/no-args` gain the new
  flags; new cases with `--stats` (JSON compared with masked wall time) and each switch.
