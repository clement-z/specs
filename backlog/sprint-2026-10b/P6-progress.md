# P6 progress — F-05 outputs and tracing (branch `ws/outputs`)

Prompt: `discover/orchestration/2026-10b/P6-outputs.md`. Heartbeat: paseo heartbeat **`cabaafbe`** (P6-heartbeat,
every 5 min, expires 48 h). Delete it right after committing `P6-REPORT.md`.

Start: `ws/outputs` @ 43a50d7 (P1–P5 merged). Build: gcc `/tmp/P6-build`; pre-change binary copied to
`/tmp/P6-specs-pre` (bench and probed-signal comparisons).

## Items
| # | item | owner | status | commit | notes |
|---|---|---|---|---|---|
| 1 | trace-all fallback: PROBE/MLPROBE present → trace only those, unless `.options traceall` is explicit | orchestrator | done | fd381fc | 26 cases re-recorded, all checked by `backlog/repro/p6-verify-traceall.py` (auto-probes gone, traceall 1→0, everything else bit-identical); new `regress/f05-mlprobe-in-subckt` |
| 2 | `.save` directive (`.save out P(drop) ...` → `PROBE{<net>}` probes) | orchestrator | done | 9feac83 | 6 new cases; saved signals bit-identical to the old automatic probes (`backlog/repro/p6-vcd-subset.py`); paper Fig. 1b runs once dc1/dc2 are renamed coupler1/2 |
| 3 | `--no-trace` | orchestrator | done | 02a05de | new `regress/f05-no-trace`; `cli/help`, `cli/no-args` re-recorded (flag in the help) |
| 4 | CSV / `.npz` table export (`--format`), harness `.npz` decoder | orchestrator; harness decoder by a Sonnet subagent (`/tmp/P6-wt/harness`, `P6-wip/harness` 45b069b, reviewed, squashed in) | done | 824702e | 6 new cases, gtest `TraceTable.*` (5); oracle `backlog/repro/p6-table-vs-vcd.py` OK on 9 netlists; VCD unchanged by `--format` |
| 5 | CROW bench (full tracing vs probes only), docs, backlog, report | orchestrator | done | report commit | CROW-30: old default 19.45 s / 1003 MB → new default 5.28 s / 15.4 MB, probes identical; csv 4.88 s, npz 4.90 s, --no-trace 4.75 s; B-157 partly fixed, notes on B-057/B-062; clang cross-check |

## Decisions (running list, copied into the report)
- An explicit `.options traceall=0|1` always wins; only its absence triggers the new rule.
- "PROBEs" = PROBE and MLPROBE elements (also inside subcircuits). PDET and PWR meters are traced as before and do
  not count.
- The rule applies to netlists only (the parser); `-t` testbenches and C++ users keep `trace_all_optical_nets`.
- `.save` probes are created at elaboration (ParseTree::build_circuit), one per signal of the net, named
  `PROBE{<signal>}` like the automatic probes; targets on one net OR their quantities. With an explicit
  `traceall=1` the saved net's probe traces only what `.save` lists (it replaces that net's automatic probe).
- `.save` targets: top-level nets only; a `.save` in a subcircuit body is ignored with the existing warning
  (every directive is). No `.save all` (would clash with a net named `all`). Quantity keywords = PROBE's.
- Unknown quantity: parse error (file:line); unknown net / electrical net: error after elaboration, exit 1.
- `--no-trace` = `-o ""` (no VCD, no table, no manifest file; `--print-manifest` still prints, with a null
  trace file); it wins over `-o` and prints `Tracing disabled (--no-trace)`; no effect on `-t` testbenches.
- Table rows: one per (time step, probe, wavelength) = last sample of that wavelength in the step (first version
  wrote every delta-cycle sample: 128082 rows for the 201-point Fig. 1b sweep, now 402).
- Columns: time_s, signal, wavelength_m, power_W, field_re, field_im (no abs/phase: derivable). Numbers with
  `std::to_chars` shortest round-trip (exact; the VCD has 16 digits). `.npz`: stored zip, fixed 1980 dates,
  arrays per column + `signal_names` (`<U`, ASCII names widened), in memory until the end; > 4 GiB → error.
- `--format` repeatable, exactly the listed formats (VCD only if listed); with `--no-trace` → error.
  Tables hold PROBE/MLPROBE/automatic/.save probes only; a warning when PDET/power meters exist and no VCD.
- Manifest `trace_file`: the VCD, else the first table (csv, then npz); no schema change.
- MLPROBE: listed-wavelength check uses `m_lambdas` (the VCD signals only exist with a trace file).

