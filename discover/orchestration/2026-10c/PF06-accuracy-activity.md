# Phase F06+F31 — accuracy suite and activity map (branch `ws/accuracy`)

Follow `COMMON.md` and the gate of `PD-common-f15.md`. Phase id `PF06`. Briefs: `discover/scout/F-06-benchmark-and-accuracy-suite.md`
and `discover/scout/F-31-activity-map.md`. Decisions: "Decisions of 2026-10-23, third round", rows "Accuracy suite"
and "Activity map", in `backlog/DECISIONS-2026-10-21.md`. Use Sonnet subagents (effort medium) for scripts and docs
where the task is well specified; review their diffs.

## Accuracy suite (F-06)
- Move the CROW generator and the analytic oracles out of `discover/scout/bench/` into a maintained place (e.g.
  `tests/bench/`), with the paper-style CROW (random ring lengths, as the brief says) and analytic checks
  (add-drop ring and CROW closed forms: Yariv / Bogaerts / Poon) as tests that fail on a physics regression.
- Timing: **ratio assertions only** in CI (e.g. a size-scaling ratio, or a ratio against a reference run in the
  same job); absolute times are recorded locally in a history file, not asserted (a fixed benchmark rig comes later).
  Do not republish paper numbers.
- Keep `src/tb/crow_tb.cpp` and its golden case until the generator covers it; say in the report whether it now does.
- Fix what the brief lists as prerequisites if small (e.g. `--nrings_crow`, identical ring lengths), or record them.

## Activity map (F-31, static map only)
- A separate tool (`utils/activity_map.py` or similar, run through pixi's `py` environment; add its Python deps to
  the `py` feature if needed) that reads the `--json` circuit export and a `--stats=ports,devices` file of the same
  run, and draws the circuit with Graphviz:
  - each device coloured by its **processed events on a linear scale**;
  - the background at **zero activity everywhere**, with the colour **interpolated between components** (a smooth
    field over the layout, e.g. from the Graphviz node positions), if feasible; say how it is done or why not;
  - **subcircuit instances enclosed** (Graphviz clusters);
  - option: the **suppression ratio** as a solid colour disc on top of each component;
  - a legend and the run totals; SVG and PNG output.
- Time windows (activity over time) are **not** in scope.
- A test on a small netlist (the tool runs and its output contains the expected nodes), docs (`doc/` and README
  pointer), CHANGELOG `[unreleased]`.

Existing outputs must stay bit-identical; counters identical.
