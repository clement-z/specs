# PL — B-186: MLPROBE records each `.dc` point: report

Branch `ws/mlprobedc` (from `sprint/2026-10c` at 57fffc3). Scope: `backlog/DECISIONS-2026-10-21.md`, row
"B-186 and B-188" (2026-10-24), B-186 part. Progress and the predictions written before the re-record:
`PL-progress.md`.

## Items

| Item | Outcome | Commit | Tests | Golden impact |
|---|---|---|---|---|
| 1 MLPROBE enabled in `.dc`, one value per point and listed wavelength (VCD, CSV/npz) | done | 314727a | gtest `MLambdaProbe.DcPointTracesEveryListedWavelengthOrZero`; golden `regress/b186-mlprobe-dc-wavelength-sweep`, `regress/b186-mlprobe-dc-power-sweep-wdm` | new cases only; no existing case changed |
| 2 a listed wavelength absent at a point records 0 | done | 314727a | same | same |
| 3 roles: PROBE follows the swept wavelength, MLPROBE its fixed list | done (PROBE untouched) | 314727a | PROBE1 rows of the repro unchanged vs the old binary | none |
| 4 `.op` | checked, left as is (a separate question, below) | | | none |
| 5 tests, docs, backlog | done | 314727a | | |

### How it works
- `runDCAnalysis()` used to write `enable = 0` to every MLPROBE for the whole sweep. It now enables them and
  sets `MLambdaProbe::m_per_point`.
- During a point, the MLPROBE thread keeps the last value of each listed wavelength received at that point
  (`m_point_samples`). It only looks at events: since B-185, every wavelength of a net is written again, with an
  event, at every point. Unlisted wavelengths are noted for the end-of-run warning, as in `.tran` (B-062).
- Once the point's fixed point has converged, `runDCAnalysis()` calls `MLambdaProbe::tracePoint()` (MLPROBEs in
  name order). For every listed wavelength, in wavelength order, it traces the kept value or 0 (power, modulus,
  phase). In the tables, a wavelength that is absent at the point gets a zero row at the listed wavelength
  (`TraceTable::appendZero()`). The listed wavelength is not registered in the wavelength registry, so the
  wavelength ids stay the same.
- `tracePoint()` writes the trace signals before the `sc_start(1 tick)` that ends the point. That `sc_start()`
  first runs an update phase at the point's time (no evaluation, no extra delta cycle), so the VCD records the
  values in the point's time step. The repro shows this: `#0` for point 1, `#1` for point 2.
- Table order within a point: the MLPROBE rows come after the other probes' rows, MLPROBEs in name order, each
  one's rows in wavelength order (documented in `doc/netlist.md`).

### Predictions vs results (gcc, strict `--rtol 0 --atol 0 --print-slack 0`)
- Existing cases: predicted none would change, because none has an MLPROBE in a `.dc` (scan of the 274 cases:
  all 15 MLPROBE netlists run a `.tran`). Result: 275 passed, 2 new; after recording, 277 passed (2 new cases +
  gtest).
- Repro, as predicted: point 1 (t = 0) gives 1.548 um 0.5 mW, 1.549 um 0, 1.55 um 1 mW. Point 2 (t = 1e-13 s)
  gives 1.548 um 0, 1.549 um 0.5 mW, 1.55 um 1 mW. PROBE1 is unchanged: the swept wavelength at each point.
- WDM power sweep (add-drop ring, 1550 nm on resonance swept 1/2/3 mW, 1552 nm fixed 1 mW, merger halves both):
  - At 1550 nm the drop port reads 0.497, 0.994 and 1.491 mW, and the through port a few nW.
  - At 1552 nm the drop port reads 0.160 mW and the through port 0.339 mW at every point. They are equal to about
    1e-4 relative, the reltol of the fixed point: each point iterates from the previous point's values.
  - probe1 traces 1550 nm (the smallest wavelength).
- `stats-compare` (gcc, before vs after): predicted identical for all existing cases. Result: all 276 cases
  identical (0 differ), **including the 2 new ones**. The MLPROBE thread wakes on the same events whether it is
  enabled or not, and the trace-signal update adds no delta cycle.
- clang, asan-gcc, asan-clang: before/after `compare` (strict, records of the old and new binaries of each
  toolchain) differ in the same set on all three: only the 2 new cases (274 identical). In each, the old binary
  records no MLPROBE value.

### `.op` (scope item 4)
In `.op`, the MLPROBEs are disabled by `runOPAnalysis()`, not by the `.dc` switch. That code also disables the
PROBEs, and it is the operating point of every `.tran` too. A `.op` run of the repro netlist writes an empty CSV:
no PROBE row either. "The probes in `.op`" is therefore a separate question that also concerns PROBE, so I left
it as is (no change to `.op` or to the `.tran` OP). See "Found, not done".

## Decisions I had to take
1. **Events only, during a point.** An MLPROBE in `.dc` records what its net received at the point, not a
   sample of the net's current value (the B-048 enable sampling). With the fresh steady state of B-185 the two
   are the same for every carried wavelength, and an event-only rule cannot carry an earlier point's value
   into this one (decision item 2).
2. **Zero rows in the tables.** Decision item 2 asks for 0 "in the VCD and in the CSV/npz tables": a listed
   wavelength absent at a point gets a row with `power_W = field_re = field_im = 0` and `wavelength_m` = the
   listed value. For a carried wavelength the row keeps the net's wavelength, as in `.tran` (it can differ from
   the listed value within 1e-12 relative).
3. **Table row order** in a `.dc` point: other probes first, then the MLPROBEs (name order, wavelength order).
   This follows from tracing at the end of the point and is deterministic.
4. **Warnings.** An MLPROBE in `.dc` now reports the unlisted wavelengths of its net at the end of the run, as in
   `.tran` (before it was disabled and said nothing). This is new stderr only for netlists with an MLPROBE in a
   `.dc`; no existing case has one.
5. The repro is copied to `tests/golden/netlists/` (golden cases only read from there), with a header comment
   giving the expected values; `backlog/repro/` keeps its copy (comment updated: "before the fix").

## Found, not done
- **`doc/netlist.md` says a PROBE latches "in `.op` ... the smallest wavelength written to its net at that
  point"**, but in `.op` the probes are disabled and record nothing (the CSV is empty; the result is the stdout
  listing). Either the doc or the behaviour should change: maintainer's call, together with MLPROBE in `.op`.
- **`.op` writes no VCD file**, but the run manifest's `trace_file` names one (`traces/<name>.vcd`); seen with
  the old binary too (`specs -f op.cir -o traces/x`: only `x.manifest.json`, or `x.csv` with `--format csv`).
  Not investigated.

## Final results (after 314727a)
- `pixi run test gcc`, `clang`, `asan-gcc`, `asan-clang`: Ok 16, Fail 0 each. No sanitizer message in
  the asan logs (`runtime error`, `AddressSanitizer`, `LeakSanitizer`: none). The golden suite inside asan-clang:
  277 passed. `tests/sanitizers/lsan.supp` unchanged.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: `Summary: 277 passed (0 with notes), 0 failed, 0
  new, 0 missing, 0 skipped`.
- Baseline re-record: `--only '^(regress/b186-|gtest/)'` (the 2 new cases and the gtest list with the new test),
  note "B-186: ...".
