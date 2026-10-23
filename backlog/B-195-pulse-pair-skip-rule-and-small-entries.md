# B-195: Pulse-pair delay estimate: skip rule depends on grid parity; small entries move between grid points
severity: low | category: physics | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: open
location: src/io/sparam_file.cpp (`removeDelay()`, `DELAY_REMOVAL_MAX_STEP_RATIO`)

## Problem
Found in phase PF03d (`backlog/sprint-2026-10c/PF03d-REPORT.md`, "Found, not done"), with the pulse-pair estimate
of the removed delay tau0 (B-190, accepted by the maintainer 2026-10-23):
- **Skip rule on alternating grids.** Intervals whose step exceeds 3 times the median step are left out. On a
  grid alternating short and long steps where the short steps are the majority and the interval count is odd,
  the median is a short step, so every long step is skipped (84 of 167 intervals on 0.2/1.0 nm steps). The
  estimate stays exact for a delay line, but which intervals are used depends on the grid's parity.
- **Small entries.** On the SiEPIC BDC's reflection and isolation entries (|S| about 1e-3, not used by its golden
  case), tau0 moves by up to 0.19 ps relative to the median estimate and S between grid points by up to 3.3e-4,
  tens of percent of those entries. No reference data says which is closer; a bidirectional BDC would see it.

## Expected
A skip rule independent of the grid's parity; a known error on weak entries.

## Suggested fix (needs design)
Base the skip rule on a robust step scale that does not flip with parity (e.g. the median of the distinct step
sizes, or a fixed fraction of the band); compare weak entries against a reference export of a device with known
reflections.

## Golden impact
None expected for the current cases (their grids are uniform; the BDC golden case uses the transmission entries).
