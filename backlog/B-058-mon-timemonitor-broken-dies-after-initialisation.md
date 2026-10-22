# B-058: `--mon` (TimeMonitor) is broken: it dies after initialisation, shifts time, and calls exit(1) on its first report
severity: med | category: cli | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: fixed in 575209a (sprint 2026-10b P2): TimeMonitor and --mon removed
location: src/devices/time_monitor.cpp:33-37,38-57,67-108, src/main.cpp:374-380
## Problem
1. Once the first report is due (wall-clock period 0.5 s, main.cpp:377/379), it prints "Time monitor incompatible with new multi-wavelength features" and calls `exit(1)` (time_monitor.cpp:107-108).
2. In practice it rarely gets there in netlist runs. The thread calls `wait()` with no static sensitivity as soon as it sees no pending activity (:33-37), which happens at the end of the first `sc_start()` of `runOPAnalysis()`. After that it never runs again. Reports and `exit` are reachable only in testbench flows, or if initialisation alone takes more than 0.5 s.
3. It still changes results. When the poll period is representable (for example `timescale=-15`, period 1e-14 s), its `wait(delay)` during initialisation advances simulated time by 10 fs before the OP and TRAN. VLSRC samples at t=0 are then dropped as "invalid time for signal emission".
4. Two smaller points:
   - main.cpp:376 tests `simulation_mode` before the netlist `.options` are parsed.
   - The period of 1e-14 s rounds to 0 at the default 100 fs timescale, so the thread then wakes every delta while it is alive.

Seed S1.01's statement that "any simulation longer than 0.5 s is killed" does not hold for netlists, because of point 2.
## Evidence
`backlog/repro/a3-mon-shift.cir` (`timescale=-15`, VLSRC values at 0 and 1 ps):
- Without `--mon`: `@0 s ... 0.001000 W`, `@1 ps ... 0.002000 W`, "Simulated 2 ps".
- With `--mon`: `ROOT/VLSRC1: invalid time for signal emission !`, only the 1 ps sample, "Simulated 2010 fs".

A 1.5 s netlist run (2 PDETs, `.tran 1e-6`) with `--mon` exits 0 and prints no report (point 2).
## Expected
`--mon` periodically prints progress without changing results or exit status. Otherwise it should be removed.
## Suggested fix
Decide whether to keep it.
- If kept: rewrite as a passive monitor. For example, do not advance time on its own, never `exit`, report from a time-stepped callback or `sc_pending_activity`-independent hook, and delete the `#if 0` and `if (false)` blocks (:69-101, :127-149) and the unused `vec2str` (:11-21).
- If not: remove `TimeMonitor` and the `--mon` flag.
## Golden impact
None: no golden case uses `--mon`.
## Related
B-050 (the same `sc_pending_activity()` liveness pattern).

- Discovery seeds: S1.01, S2.15 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-12.
## Verification
I ran the repro with and without `--mon` using the release binary, and timed all testbenches with `--mon` (all under 0.1 s, none reaches the exit).
