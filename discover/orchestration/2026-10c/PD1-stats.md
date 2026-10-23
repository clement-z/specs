# Phase D1 — `--stats` and ablation switches (branch `ws/stats`)

Follow `COMMON.md` and `PD-common-f15.md`. Phase id: `PD1`.
- `--stats`: per run, aggregated from per-port counters (no globals): values scheduled, coalesced (same-time merges),
  processed, emitted, suppressed (by abstol / by reltol / by snap), max and mean pending-queue length, last
  emission time, wall time; written as `<trace>.stats.json` next to the trace (and printed on request); with no
  trace file (`--no-trace`, `-o ""`) it must still be obtainable (printed, or an explicit path: choose, document). Tiers [Q5]:
  `--stats` (= `summary`), `--stats=ports` (one row per output port), `--stats=devices` (per device type); per-port
  counters are collected whenever stats are on, tiers change the output only. No energy/charge accounting.
- Documented expert switches for ablation [Q4]: no coalescing (equal-time entries kept, FIFO ties) and an event
  cap, as `.options` keys and CLI flags; documented in `doc/numerics.md` as expert options.
- `--progress` [Q6]: a passive periodic progress line on stderr (wall-clock period; SystemC stage callback), never
  advancing simulated time and never exiting.
- **Counter collection for the later gates:** a reproducible way to run every golden case with `--stats` and
  compare the per-case counters of two builds (wall time and other run-dependent fields masked), e.g. a
  `run_golden.py` option or a small script next to it, with a unit or golden test. D2 and D3 are gated on it.
- Measure the cost of collecting stats: with stats off, the CROW bench must not slow down (ratio, `PD-common-f15.md`).
- Expected golden impact: none for existing cases (stats off by default); `cli/help`/`cli/no-args` gain the new
  flags; new cases with `--stats` (JSON compared with masked wall time) and each switch.
