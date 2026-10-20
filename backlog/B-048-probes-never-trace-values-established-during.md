# B-048: Probes never trace values established during the OP, so constant (CW) nets show 0 in .tran
severity: high | category: bug | effort: XS | fix-class: needs-design | verdict: CONFIRMED
location: src/devices/probe.cpp:26-31,143-148, src/specs.cpp:79-87,208-216
## Problem
`runTRANAnalysis()` first runs the OP with every PROBE/MLPROBE (and every auto-probe) disabled (specs.cpp:79-87).
- The first event a probe receives while disabled makes it block on `wait(enable.posedge_event())`.
- When TRAN re-enables it (specs.cpp:208-216), it executes `continue` and goes back to `wait()` without sampling `p_in`.

Any net whose value was set during the OP and does not change afterwards therefore reads 0 in the VCD for the whole transient: every CW-laser path, bias arms, and so on. Nets that do change show 0 until their first post-OP event. Explicit PROBEs and the `traceall` auto-probes are both affected. The OP printout on stdout is correct, so the trace contradicts it.
## Evidence
`backlog/repro/a3-probe-cw-tran.cir` is `cwsrc 1 mW -> wg -> probe1`, `.tran 1e-9`.
- stdout OP: `ROOT/out = 0.031623 V.m⁻¹, 0.001000 W`.
- VCD: all probe signals (`ROOT/PROBE1.*`, `PROBE{ROOT/in}.*`, `PROBE{ROOT/out}.*`) are `r0` in `$dumpvars` and are never written again.
## Expected
At TRAN start the probe traces the current (OP) value of its net.
## Suggested fix
The minimal fix, which I tested, is to fall through and sample after the enable posedge instead of `continue`, in both `Probe` and `MLambdaProbe`:
```
 if (!enable.read().to_bool())
 {
     wait(enable.posedge_event());
-    continue;
 }
```
This needs a design call, though. A probe that received *no* event during the OP stays in `wait()` and is not woken by the enable; this is harmless today because its net is then still at its initial value. A more robust alternative is to make the thread sensitive to `enable` as well, or to force a sample from `runTRANAnalysis()` after enabling.
## Golden impact
None for the tested minimal fix: the full golden check with the patched binary passed on every case except tb/mesh (B-047). No golden case combines CW with `.tran`. Netlists with CW + `.tran` get new t=0 values in their VCD.
## Related
B-057 (probe semantics on WDM nets), B-062 (MLPROBE).

- Discovery seeds: S1.09, S2.25 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-02.
## Verification
With the patched binary, the same netlist dumps `r0.001` (power), `r0.0316` (abs), `r-0.405` (phase) and `r1.55e-06` at t=0 for PROBE1 and `PROBE{ROOT/out}`.

**Independent second check (V1, of F-A3-02): CONFIRMED.** Method: ran backlog/repro/a3-probe-cw-tran.cir with the release binary; applied the suggested minimal fix (remove `continue` in Probe and MLambdaProbe) on the scratch copy and ran the golden check

- **Before:**
  - Stdout OP shows `ROOT/out ... 0.001000 W`.
  - In the VCD all 12 probe signals are r0 in $dumpvars and are never written again (69-line VCD).
- **Code:** probe.cpp:27-31 and :143-148 do `wait(enable.posedge_event()); continue;`.
- **With the minimal fix:**
  - The VCD $dumpvars has power 0.001, abs 0.0316, phase -0.405 and wl 1.55e-06 for PROBE1 and PROBE{ROOT/out}.
  - Golden 44/44 pass, also with --atol 0, so the claimed golden impact "None" holds.
- **Severity and class:** high (a silently wrong trace for every CW + .tran netlist) and needs-design are justified.
