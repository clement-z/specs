# P6 progress — F-05 outputs and tracing (branch `ws/outputs`)

Prompt: `discover/orchestration/2026-10b/P6-outputs.md`. Heartbeat: paseo heartbeat **`cabaafbe`** (P6-heartbeat,
every 5 min, expires 48 h). Delete it right after committing `P6-REPORT.md`.

Start: `ws/outputs` @ 43a50d7 (P1–P5 merged). Build: gcc `/tmp/P6-build`; pre-change binary copied to
`/tmp/P6-specs-pre` (bench and probed-signal comparisons).

## Items
| # | item | owner | status | commit | notes |
|---|---|---|---|---|---|
| 1 | trace-all fallback: PROBE/MLPROBE present → trace only those, unless `.options traceall` is explicit | orchestrator | done | (item-1 commit) | 26 cases re-recorded, all checked by `backlog/repro/p6-verify-traceall.py` (auto-probes gone, traceall 1→0, everything else bit-identical); new `regress/f05-mlprobe-in-subckt` |
| 2 | `.save` directive (`.save out P(drop) ...` → `PROBE{<net>}` probes) | orchestrator | pending | | |
| 3 | `--no-trace` | orchestrator | pending | | |
| 4 | CSV / `.npz` table export (`--format`), harness `.npz` decoder | orchestrator | pending | | |
| 5 | CROW bench (full tracing vs probes only), docs, backlog, report | orchestrator | pending | | |

## Decisions (running list, copied into the report)
- An explicit `.options traceall=0|1` always wins; only its absence triggers the new rule.
- "PROBEs" = PROBE and MLPROBE elements (also inside subcircuits). PDET and PWR meters are traced as before and do
  not count.
- The rule applies to netlists only (the parser); `-t` testbenches and C++ users keep `trace_all_optical_nets`.
