# Phase F02 — parameter sweeps and Monte-Carlo (branch `ws/sweeps`)

Follow `COMMON.md` and the gate of `PD-common-f15.md`. Phase id `PF02`. Brief: `discover/scout/F-02-sweeps-and-monte-carlo.md`.
Decisions: "Decisions of 2026-10-23, third round", row "Sweeps and Monte-Carlo", in `backlog/DECISIONS-2026-10-21.md`;
the seeds of F-01 (`doc/reproducibility.md`: one master seed, per-device streams by hierarchical name) and
`--param` (F-12) already exist.

## Scope
1. `.step param X start stop step` (SPICE syntax, stop included when (stop-start)/step is an integer, like `.dc`),
   over any `.param`; nested steps if the brief proposes them.
2. `.mc N [seed=...]` with `gauss()` / `uniform()` (and what the brief lists) usable in `.param` expressions; each
   `.subckt` instance draws independently by default, `shared=` to share a draw; draws come from the F-01 seeded
   streams so a run is reproducible and each Monte-Carlo run is reproducible on its own.
3. **Outputs: one table for all runs**, with a run index and the stepped/drawn parameter values, in a format that
   holds large data efficiently (many runs × probes × time points). Evaluate the options first and write the choice
   in `PF02-progress.md` before implementing: no heavy new runtime dependency in `specs` (the existing dependency-free
   `.npz` writer, chunked/appendable layouts, or an optional dependency); readable from Python (pyspecs) without
   loading everything in memory.
4. **Resilient to the failure of one run**: a failing run (error, non-convergence, event cap) is recorded in the
   table (status, message) and the others continue; a rerun can resume and skip completed runs. SystemC can only
   elaborate once per process: say how runs are isolated (one process per run is acceptable).
5. pyspecs: read the table (concatenation, filtering by run/parameter); docs (`doc/netlist.md`,
   `doc/reproducibility.md`), CHANGELOG `[unreleased]`, golden cases (small steps and a small Monte-Carlo with a fixed
   seed), unit tests.

Existing outputs must stay bit-identical; counters identical.
