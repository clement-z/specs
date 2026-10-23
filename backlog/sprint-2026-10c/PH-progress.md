# PH — `.dc` points as fresh steady states, wavelength matching: progress

Branch `ws/dcnum`. Heartbeat id: `57fc614b` (delete after the report is committed).

| Item | Status | Commit | Notes |
|---|---|---|---|
| 1 `.dc` point = fresh steady state (B-003, B-185) | done | (see git log) | as predicted; gcc strict 241/241 |
| 2 wavelength matching tolerance (B-010, B-062) | done | (see git log) | as predicted (9 cases + gtest); gcc strict 243/243; B-187 created |
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

## Item 2: predicted golden impact (written before the re-record)
Prediction method: a temporary build (not committed) that only printed a line when a registry lookup missed
exactly but matched an earlier value within 1e-12 relative, or when an MLPROBE missed a listed wavelength
exactly but matched it within 1e-12; run on every golden case (`--no-unit-tests`). It flagged 9 cases:
- 8 `.dc` sweeps whose accumulated point "1550 nm" (B-009) is 1.550000000000001e-06 (add_drop.cir:
  `cir/add_drop`, `opt/tol-resolution-overrides`, `opt/mode-fd`, `opt/multi-file`, `regress/b004-mode-td-dc`,
  `regress/f15-stats-dc`) or 1.5500000000000027e-06 (`regress/f05-save-paper-fig1b`,
  `regress/f05-format-vcd-csv-dc`), next to the source's `wl=1.55e-6` registered at parse time: that point now
  takes the registered 1.55e-6 (the first registered value is kept);
- `regress/b062-mlprobe-unlisted-wavelength`: {lambda0}+1.5e-9 = 1.5514999999999999e-06 now matches the listed
  1.5515e-06 (MLPROBE1 traces it, its warning goes). Netlist updated (comment; MLPROBE0 added so that the case
  still has a near miss beyond the tolerance and the name-order check).
Plus the gtest `MLambdaProbe.ReportsIgnoredWavelengthsOnce`, which asserted the exact match (updated).

Proof (gcc, strict): in each of the 8 sweeps, only that one point differs (VCD held values: #15, resp. #100;
CSV rows of that point); its wavelength becomes 1.55e-06 (within 1e-12 of the old one), values change by
<= 8e-14 relative in power (3e-11 for the f05 drop port, near a resonance), phases by <= 4.9e-10 rad. The new
values at that point are bit-identical to what the **before** binary gives for a sweep starting exactly at
1.55e-6 (add_drop: 40 probe signals, f05: 2), i.e. the point is the fresh steady state at 1.55e-6.
New cases: `regress/b010-near-duplicate-lambda` (m = 2 W, before 0.5 W shown),
`regress/b062-mlprobe-computed-wavelength` (c/f traced by MLPROBE, before ignored with a warning).
Counters (`stats-compare` vs the item-1 build): 239 identical with `--ignore wavelengths_m`; without it the 9
add_drop-based `.dc` cases list their ports' wavelengths in another order (the merged point has the id of the
source's 1.55e-6, registered first). Registry lookups: only `CWSource`/`BitstreamSource::setWavelength()` (setup
and once per `.dc` point) and the `OpticalSignal(field, wavelength)` constructor (VLSRC values at parse, legacy
testbench stimuli): not per event. MLPROBE matches once per wavelength id (`m_listed_by_id`).
