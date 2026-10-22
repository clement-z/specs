# B-078: Wavelength merge in FD is fragile (`has_wl` never True); first probe on an unpowered net or without `wl` breaks the parse
severity: med | category: python | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: pyspecs/specsparse.py:59-60, :70-84, :92, :100-101
## Problem
Three related failures of the probe loop; one restructuring fixes all of them.
1. `has_wl = False` (line 76, comment "set to true will save WL vector only once") is never set to True, so every probe's `wavelength` signal is merged with an outer join on `['tick','wavelength']` (all common columns). A net that carries no light never changes its wavelength (VCD has a single sample `(0, 0.0)`, e.g. `PROBE{ROOT/add}` in add_drop). If such a probe sorts before the others in the VCD, the join duplicates rows. Seed S3.15 suggests flipping `has_wl` to True: that is NOT a fix (see evidence), it takes the wavelength vector from whichever probe comes first.
2. If the first signal of the first probe is `wavelength` (probe with `power=0 mag=0 phase=0 wl=1`, `traceall=0`), `current_signal_df` is unbound at line 82. If a non-first signal is skipped, the stale previous `current_signal_df` is merged again.
3. A sweep in which no probe traces `wavelength` (`probe1 in wl=0`, `traceall=0`) raises `KeyError: 'wavelength'` at line 92.
## Evidence
Repro netlists `backlog/repro/a6-pyspecs-unpowered-first.cir` (add_drop with net `add` renamed `Aadd` so `PROBE{ROOT/Aadd}` sorts first), `a6-pyspecs-wl-first.cir`, `a6-pyspecs-nowl-fd.cir`; driver `backlog/repro/a6-pyspecs-repro.sh <repo> <python>`.
```
original add_drop 30 sweep points, with only the 'b1' fix of B-076 applied:
  unpowered-first: probes=(59, 31)    <- 59 rows, wavelength column contains invented points 1.5365e-06, 1.5375e-06, ...
  + has_wl = True (seed S3.15 suggestion): probes=(29, 31), wavelength column = [nan, nan, nan, ...]
wl-first (TD):  UnboundLocalError: local variable 'current_signal_df' referenced before assignment (specsparse.py:82)
nowl-fd (FD):   KeyError: 'wavelength' (specsparse.py:92)
```
## Expected
One wavelength vector (the most complete one) as the first column, no duplicated rows, no exceptions for valid probe options; a clear error when the sweep has no wavelength trace at all.
## Suggested fix
Collect per-signal frames first, skip `wavelength` signals in TD, keep the longest `wavelength` signal as the single master in FD, merge everything on `tick` only, raise `RuntimeError` with a hint when there is no wavelength in a sweep, use `.ffill()`. Reference implementation (also contains the B-076 and B-077 edits): `backlog/repro/a6-specsparse-fix.diff` (unified diff against the current file, ~50 changed lines). Design points to confirm: choice of the "longest" vector as master (alternative: require the user to trace wl on the source net), dropping the old `wavelength == 0` row filter (no longer needed because spurious rows are no longer created), row index now starts at 0.
## Golden impact
none.
## Related
B-076, B-077, B-096.

- Discovery seeds: S3.15 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-03.
## Verification
With the reference patch: all 23 time-domain baseline VCDs give DataFrames identical (`equals`) to the original code; the 4 `.dc` baselines now parse as FD with 30 rows, values equal to the b1-only version ignoring the row index; unpowered-first gives (30, 31) with the same out/drop/wavelength values as add_drop (`allclose`); wl-first parses; nowl-fd raises the new RuntimeError.
