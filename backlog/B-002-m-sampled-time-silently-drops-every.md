# B-002: `-m sampled-time` silently drops every optical write when it takes effect
severity: med | category: cli | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/optical_output_port.cpp:360-369,413-415, src/main.cpp:204-207,297-301
## Problem
`delayedWriteSampledTime` is an empty TODO. When `-m sampled-time` is in effect, every device output is
discarded, the VCD contains only initial values, and the exit code is 0. `-m` is only in effect for netlists
without an analysis directive (default TRAN) and for testbenches, because `.op`/`.dc`/`.tran` overwrite the mode
(B-004). That is why golden `opt/mode-sampled-time` (with `.tran`) looks fine.
## Evidence
`backlog/repro/a1-tran-no-analysis.cir` (no analysis line, VLSRC → WG):
```
-m td           : "Simulated 3300 fs", VCD has #10 #20 #23 value changes
-m sampled-time : "Simulated 2 ps",   VCD after $dumpvars is empty (0 timestamps), rc=0
```
## Expected
Either the mode works, or it is rejected with an error and a non-zero exit.
## Suggested fix
Until the mode is implemented, reject it. In main.cpp:297-301 print
`cerr << "Error: sampled-time mode is not implemented" << endl; return 1;`. In `delayedWrite` (line 413) make
the SAMPLED_TIME case report an error instead of returning silently. Remove "sampled-time" from the help text.
This needs a decision together with B-004 (if `-m` stops being overridden, the golden case would start dropping
output).

Correction from the second check: do not report the error inside `delayedWrite` (per-event hot path). Reject `SAMPLED_TIME` once, at configuration time: in `main.cpp:297-301` and in `verifyConfig()`/`applyConfig`, so testbenches and API users are covered too. Scope: every `.op`/`.dc`/`.tran` overrides `-m` (B-004); only the `ac_add_drop` and `ac_crow` testbenches honour it; in an analysis-less netlist the OP phase still writes, so only post-OP writes are dropped.
## Golden impact
`opt/mode-sampled-time` becomes exit 1 (re-record it as expect=fail, or drop it). `cli/help` changes if the help text is edited.
## Related
B-004.

- Discovery seeds: S1.02, S5.02 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-02.
## Verification
Ran the repro with all 3 modes and compared the VCDs. Code reading: lines 360-369 have no body and line 414 returns.

**Independent second check (V3, of F-A1-02): ADJUST.** Method: ran backlog/repro/a1-tran-no-analysis.cir with -m td/sampled-time/fd; ran `-t ac_add_drop -m td|sampled-time`; grepped every simulation_mode writer

Reproduced. With sampled-time the VCD has 0 timestamps after $dumpvars (td: 3), "Simulated 2 ps", and rc=0. The ac_add_drop testbench VCD body has 25 lines instead of 1286. Scope is narrower than stated, in two ways. (a) Among testbenches, only ac_add_drop and ac_crow honour -m. The other 17 tb_map entries force EVENT_DRIVEN (src/tb/*_tb.cpp `simulation_mode = EVENT_DRIVEN`). (b) In an analysis-less netlist, the OP phase still runs in NO_DELAY, so CW propagation and the OP printout work. Only writes after OP (in the TRAN phase) are dropped. Every .op/.dc/.tran overrides -m. The output is conspicuously empty, not plausibly wrong. I suggest severity med instead of high. Fix note: do not report the error inside delayedWrite (CLAUDE.md: no printing in per-event hot paths). Reject SAMPLED_TIME once, in main.cpp:297-301 and in verifyConfig()/applyConfig, so that testbenches and API users are covered too. needs-design and golden impact text are OK.

**Orchestrator:** Severity lowered high -> med (the effect is a conspicuously empty trace on a narrow path); fix location corrected.
