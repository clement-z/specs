# F-31: Activity map — event activity drawn on the circuit (schematic export of the run statistics)

value 3–4 | effort S (static map) / M (time-resolved) / L (back-annotated KiCad schematic) | risk low | prerequisites: F-15
`--stats=ports,devices` (done, v0.3.0), `--json` circuit export (exists) | horizon 1 (static map)
Requested by the maintainer on 2026-10-23 ("automatic export of the activity report to a schematic").

## Problem / user story
*As a designer or as the paper's author I want to see where the simulation spends its events: which rings, loops
and couplers are active, where the tolerance gate suppresses most writes, and how activity moves over time.*
Today `--stats=ports,devices` gives the numbers per output port and per device type as JSON, and `--json` writes
the flattened circuit (devices, ports, nets), but nothing puts the two together: the user has to match port names
to a mental picture of the netlist. Event-driven speed depends on activity (the paper's activity ratio), so a
picture of it explains a run's cost and guides tolerance choices.

## What exists
- `specs --json file`: the flattened circuit (devices, their ports and nets, net types).
- `utils/draw_graph_from_json.py`: draws that JSON with Graphviz (devices as nodes, nets as edges, colour by net
  type). Python, `graphviz` package; not tested, not documented in `doc/`.
- `--stats=ports` rows: per output port, its net, writer, wavelengths and counters (scheduled, coalesced,
  processed, emitted, suppressed by abstol/reltol/snap, pending maximum, last emission).

## Proposed scope
1. **Static activity map (S).** `--activity-map file.svg` (or a `utils/` script reading the run's `.stats.json`
   and the `--json` export): the circuit graph with each device coloured by its processed events (log colour
   scale) and each net labelled with emitted / suppressed counts; a legend and the run totals. Output SVG/PDF via
   Graphviz `dot` (already used by the Doxygen docs), or a DOT file when `dot` is absent. Works for netlists and
   `.subckt` hierarchies (cluster per instance).
2. **Time-resolved activity (M).** Counters per time window (e.g. `--stats-window 1ns`): a sequence of maps or one
   map plus per-device sparklines; needs a small extension of the per-port counters (a histogram per window).
3. **Back-annotation onto a drawn schematic (L).** Colour the symbols of a KiCad schematic (or a gdsfactory layout
   netlist) by activity. Depends on F-13 (design-tool interop: a stable mapping between schematic symbols and
   SPECS instance names).

## Usefulness
- Explains a run's cost at a glance (hot resonant loops, idle branches) and supports tolerance tuning per region.
- A ready-made figure for papers and talks about event-driven activity (the TCAD activity discussion).
- Teaching and demos (the web demo could show it next to the traces).
Low risk: read-only use of existing outputs; no change to simulation results.

## Effort
Static map: about one phase of S size (script + golden-style test on a small netlist + docs); time windows: M
(engine-side counters, stats format version bump); schematic back-annotation: L, after F-13.

## Open questions for the maintainer
- Output: a `specs` option, or a separate `utils/` tool that combines `--json` and `.stats.json` (proposed: tool
  first, no new runtime dependency in `specs`)?
- Which counter drives the colour by default: processed events, emitted events, or suppression ratio?
- Is the time-resolved variant needed for the paper, or is the static map enough for now?
