# B-185: In a `.dc`, a net whose value does not change between points is not written again, so a probe sees a non-swept wavelength only at the first point
severity: low | category: bug | effort: M | fix-class: needs-design | verdict: CONFIRMED | status: fixed in b6488a3 (sprint 2026-10c PH)
location: src/specs.cpp (`SPECSConfig::runDCAnalysis()`: per-point reset of the output ports and CW sources), src/devices/probe.cpp (`Probe::accept()`, the per-point latch)

## Problem
Found in phase PD5 (sprint 2026-10c, report `backlog/sprint-2026-10c/PD5-REPORT.md`, decision 3 and "Found, not
done"). At each `.dc` point the output ports are reset and every CW source re-emits its value. A source that is
not swept writes the same value to its net as at the previous point. An `sc_signal` written with its current
value gets no value-change event, so nothing downstream is evaluated again at that wavelength, and the nets
after it are not written at that wavelength either.

A plain PROBE latches, at each `.dc` point, the smallest wavelength **written to its net at that point** (PD5).
On a net fed by a swept source and a fixed one, it therefore sees both wavelengths at the first point only. At
the following points it sees only the swept wavelength, although the fixed one is still present (the devices keep
its per-wavelength value). This was already the case before PD5: the probe then showed whichever wavelength was
written last.

## Evidence
`regress/probe-latch-dc` (`tests/golden/netlists/probe-latch-dc.cir`): cwsrc1 at 1550 nm and cwsrc2 swept from
1541 to 1559 nm into a merger, `probe1` on its output. Expected by the rule "the smallest wavelength present at
the point": 1541, 1547, 1550, 1550 nm. Recorded: 1541, 1547, 1553, 1559 nm (CSV rows of `ROOT/PROBE1`). Only the
first point has a row at 1550 nm. Documented in `doc/netlist.md`, "PROBE, MLPROBE and traced nets in `.tran`".

## Suggested fix (needs design)
- (a) At each point, make every source emit an event even when its value is unchanged (e.g. reset the nets, or
  emit through a different value first). This changes the `.dc` event counts and possibly outputs; check
  `stats-compare` and the strict golden.
- (b) Let the probe keep the last value of each wavelength across points and treat it as present. It cannot tell
  a fixed source (still present) from a swept one that moved on (gone), so it needs to know which wavelengths the
  sources emit at the point.
- (c) Keep it and document it (current state).

## Related
PD5 (PROBE wavelength latch), B-001 (per-wavelength values in FD writes), B-048, B-184.

## Update (PD5 follow-up 2, maintainer decision: option (a) for the swept wavelength)
In a `.dc` that sweeps a source's wavelength or frequency (`WL(src)`, `F(src)`, `/src/WL`, `/src/F`), the `.dc`
runner now passes the swept wavelength of each point to the probes (`Probe::setSweptWavelength()`), and a PROBE
latches it when its net carries it at that point, before the smallest-wavelength rule. **Solved:** the first point,
where both sources write the net, now shows the swept wavelength even when the fixed one is smaller (golden cases
`regress/probe-latch-dc-swept-wl`, `regress/probe-latch-dc-swept-f`; before, the fixed 1530 nm was latched there).
**Still open:** the unchanged-net part. A net whose value does not change between points is not written again, so
a probe on a net that carries only non-swept wavelengths gets a sample at the first point only, and the fixed
wavelength is not seen again after the first point (`regress/probe-latch-dc`, `probe2` of
`regress/probe-latch-dc-swept-wl`). Options (a) to (c) above still apply to it.

## Fix (sprint 2026-10c PH, b6488a3): option (a)
Maintainer decision (2026-10-23, third round): each `.dc` point is a fresh steady state; after the per-point
reset every output port emits each of its wavelengths at least once. A CW source that is not swept did write its
net at each point (its port was reset, so the gate compared the value with 0), but with the value the net
already held, so `sc_signal` notified nothing. The nets are now `OpticalNet` (an `sc_signal` with
`writeWithEvent()`), and the first entry of each wavelength at a `.dc` point is written with a value-change event
(B-003 for the details), so the fixed wavelengths are propagated again at every point and reach the probes.
`.op` and `.tran` are unchanged (bit-identical, same counters).
- `regress/probe-latch-dc-swept-wl`: `probe2` (net n, two fixed sources) has a sample at every point (was the first
  only); `regress/probe-latch-dc`: unchanged output (its probe latches the swept wavelength, PD5 follow-up 2; the
  expectation "1550, 1550 nm" above predates that rule), comment updated.
- Event counters: the 3 multi-source `.dc` golden cases process more entries (e.g. merger 5 -> 8 over 4 points);
  every other case is identical.
