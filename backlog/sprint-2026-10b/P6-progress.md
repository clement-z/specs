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
| 3 | `--no-trace` | orchestrator | done | (item-3 commit) | new `regress/f05-no-trace`; `cli/help`, `cli/no-args` re-recorded (flag in the help) |
| 4 | CSV / `.npz` table export (`--format`), harness `.npz` decoder | orchestrator | pending | | |
| 5 | CROW bench (full tracing vs probes only), docs, backlog, report | orchestrator | pending | | |

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
