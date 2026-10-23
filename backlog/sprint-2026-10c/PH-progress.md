# PH — `.dc` points as fresh steady states, wavelength matching: progress

Branch `ws/dcnum`. Heartbeat id: `57fc614b` (delete after the report is committed).

| Item | Status | Commit | Notes |
|---|---|---|---|
| 1 `.dc` point = fresh steady state (B-003, B-185) | done | (see git log) | as predicted; gcc strict 241/241 |
| 2 wavelength matching tolerance (B-010, B-062) | todo | | |
| 3 statuses (B-003, B-010, B-062, B-185, backlog.md) | todo | | |

## Before references (tip c975167)
gcc build copied to `/tmp/ph-before/gcc` (whole build dir: rpath `$ORIGIN/...`), counters
`/tmp/ph-stats-before` (239 cases, 141 with statistics). gcc strict golden: 240/240 pass (239 cases + gtest).

## Item 1: predicted golden impact (written before the re-record)
Mechanism: after the per-point reset, the first value of each wavelength at each port is written whatever the
gate says (snapped) and with a forced value-change event (`OpticalNet::writeWithEvent()`). Outputs can change only
(a) where a port's first value of a point is <= abstol (B-003): no golden `.dc` case has such a point (add_drop
and the f12/f05 sweeps have one source and no null; checked: their outputs are bit-identical), and (b) where a
probe sees a wavelength that is not re-written today (B-185): the `.dc` cases with two or more sources are
`regress/probe-latch-dc`, `-swept-wl`, `-swept-f`. Their probes on a net carrying the swept wavelength latch it
(PD5 follow-up 2), so they do not change; only `probe2` of `-swept-wl` (net n, two fixed sources) gains a
sample at points 2-4 (CSV rows; the VCD shows no change since the values repeat).
- Re-recorded: `regress/probe-latch-dc-swept-wl` (3 CSV rows of PROBE2, 1520 nm; netlist comment),
  `regress/probe-latch-dc` (netlist comment only: manifest sha256). New: `regress/b003-dc-dark-point`.
- Counters (`stats-compare` vs `/tmp/ph-stats-before`): 3 cases differ, the 3 multi-source `.dc` cases; per
  point after the first, the fixed wavelengths are propagated again (probe-latch-dc: merger +1 per point,
  5 -> 8; -swept-f 6 -> 10; -swept-wl: splitter +2 per point, merger1 +1, merger2 +2). 236 identical.
