# Phase H — `.dc` points as fresh steady states, wavelength matching tolerance (branch `ws/dcnum`)

Follow `COMMON.md` and the gate of `PD-common-f15.md`. Phase id `PH`. Scope: "Decisions of 2026-10-23, third
round" in `backlog/DECISIONS-2026-10-21.md` (rows `.dc` points and Wavelength matching). Both items change results
by decision: predict the changed golden cases in `PH-progress.md` before changing outputs, re-record only those,
and prove each change is the intended one.

1. **Each `.dc` point is a fresh steady state** (B-003, B-185). After the per-point reset of the output ports, every
   output port emits each of its wavelengths at least once at that point (per wavelength, not only the first
   pop of a port: the B-003 file's one-line fix is per port and not enough with several wavelengths), so:
   - a point whose value falls below `abstol` (a transmission null) is written, instead of keeping the previous
     point's value (B-003 repro `backlog/repro/a1-dc-dark-point-stale.cir`);
   - a wavelength whose value does not change between points (a fixed source next to a swept one) is written again
     at every point, so probes and MLPROBEs see it (B-185, `regress/probe-latch-dc`).
   Look at how the sources write at each point (a CW source that is not swept writes the same value: does it emit
   at all?) and choose the smallest change that guarantees the rule for every net; `.op` and `.tran` must stay
   bit-identical. Expect `.dc` event counters to change (explain them). Update B-003 and B-185 (and B-186 if its
   evidence changes: MLPROBE in `.dc` is a separate decision, do not enable it), `doc/netlist.md` / `doc/numerics.md`
   (what a `.dc` point is), CHANGELOG `[unreleased]` (Changed/Fixed).
2. **Wavelength matching tolerance** (B-010, B-062). Wavelengths equal within a relative `1e-12`
   (`|a-b| <= 1e-12*max(|a|,|b|)`) are one wavelength: in the wavelength registry (`getIDFromWavelength`, the
   first registered value is kept) and in MLPROBE's wavelength list matching. The tolerance is a **compile-time
   constant** (one named constant, documented in its header and in `doc/numerics.md`). Registry lookups are not on
   the per-event path: confirm, and keep it so. Add a golden case from `backlog/repro/a1-near-duplicate-lambda.cir`
   (two sources at "the same" wavelength written differently now interfere) and one for a frequency-swept or
   computed wavelength matching an MLPROBE list. Create a backlog item (next free number, low severity, status
   open, "not studied further unless an issue points to it"): the tolerance can merge two genuinely different
   wavelengths closer than 1e-12 relative, or miss computed values that drift more than that (e.g. accumulated
   `.dc` steps, B-009).
3. Update the statuses of B-003, B-010, B-062, B-185 and their `backlog.md` rows.
