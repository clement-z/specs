# B-197: Probes record nothing in `.op`, while the docs say a PROBE latches there
severity: low | category: docs | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: open
location: src/specs.cpp (`runOPAnalysis()` disables PROBEs and MLPROBEs), doc/netlist.md (PROBE)

## Problem
Found in phase PL (`backlog/sprint-2026-10c/PL-REPORT.md`, "Found, not done"). `runOPAnalysis()` disables every
PROBE and MLPROBE, also for the operating point before a `.tran`; a `.op` run writes an empty CSV (no probe row;
the result is the stdout listing). `doc/netlist.md` says a PROBE latches "in `.op` ... the smallest wavelength
written to its net at that point".

## Expected
The docs and the behaviour agree: either the probes record the operating point (one point, like a `.dc` point),
or the docs say that `.op` results are the stdout listing only.

## Suggested fix (needs a decision)
Decide whether probes should record in a `.op` (not in the operating point before a `.tran`); then change the code
or the docs.

## Golden impact
Docs only: none. Recording in `.op`: `.op` cases gain probe rows.
