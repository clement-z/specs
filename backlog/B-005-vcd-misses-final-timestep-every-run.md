# B-005: VCD misses the final timestep of every run that ends by event starvation
severity: med | category: bug | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/specs.cpp:235-240,365-370 (no sc_close_vcd_trace_file anywhere in the CLI path)
## Problem
SystemC writes a timestep's values to the VCD only when time advances past it (`do_timestep` →
`trace_cycle`, kernel sc_simcontext.cpp:973-981). A `.tran` without a duration (`sc_start()`) ends by
starvation, so the values of the last event time are never written. The VCD is also never closed. Closing it
alone only appends the final `#time` line, without values.
## Evidence
`backlog/repro/a1-tran-no-analysis.cir`: stdout says `Simulated 3300 fs`, but the VCD ends at `#23`. The out-net
change to 0.5 W at 3.3 ps is missing. A patched binary that calls `sc_start(sc_time::from_value(1))` and
then `sc_close_vcd_trace_file` writes `#33 r0.5 … r0.7071 … #34`. On golden `cir/subckt_tests`, the same patch
reveals `PROBE{ROOT/out_}.abs` = 1.46e-11 at #4163204, where the baseline has 0.
## Expected
The VCD contains the final state, and the file is closed.
## Suggested fix
At the end of `runTRANAnalysis`, and only when it used `sc_start()` with no duration: print "Simulated …"
first, then run `sc_start(sc_time::from_value(1))` so the kernel writes the last timestep. Then call
`sc_close_vcd_trace_file(default_trace_file)` at the end of `runAnalysis`. Do not add the extra tick after DC
(it re-triggers the pending CW reset and adds an "emitted" line, as seen on `cir/add_drop`) or after a finite
`.tran` (that one already ends with `do_timestep`).
## Golden impact
`cir/subckt_tests` and `opt/vci` (TRAN without duration) gain the final records. With close-only, every VCD
gains a trailing `#t` line.
## Related
B-007.

- Discovery seeds: S1.12 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-05.
## Verification
Ran the repro, read the kernel source (thirdparty systemc 3.0.1), and ran env-gated patched binaries plus the golden check.
