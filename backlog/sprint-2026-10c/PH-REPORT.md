# PH — `.dc` points as fresh steady states, wavelength matching: report

Branch `ws/dcnum` (from `sprint/2026-10c` at c975167). Scope: "Decisions of 2026-10-23, third round" in
`backlog/DECISIONS-2026-10-21.md`, rows `.dc` points and Wavelength matching. Progress and the predictions
written before each re-record: `PH-progress.md`.

## Items

| Item | Outcome | Commit | Tests | Golden impact |
|---|---|---|---|---|
| 1 each `.dc` point is a fresh steady state (B-003, B-185) | done | b6488a3 | gtest `OpticalOutputPort.DcPointWritesFirstValueOfEachWavelengthWithAnEvent`, `OpticalNet.WriteWithEventNotifiesAnUnchangedValue`; golden `regress/b003-dc-dark-point` | as predicted: `regress/probe-latch-dc-swept-wl` (probe2 samples its fixed-wavelength net at every point), `regress/probe-latch-dc` (netlist comment only) |
| 2 wavelength matching within a relative 1e-12 (B-010, B-062) | done | 59479b7 | gtest `WavelengthMatching.*` (new `src/test/optical_signal.cpp`), `MLambdaProbe.ReportsIgnoredWavelengthsOnce` updated; golden `regress/b010-near-duplicate-lambda`, `regress/b062-mlprobe-computed-wavelength` | as predicted: 8 add_drop/f05 `.dc` cases at their one drifted "1550 nm" point, `regress/b062-mlprobe-unlisted-wavelength` |
| 3 statuses B-003, B-010, B-062, B-185 (+ B-186 note, new B-187) | done | 727646b (B-187: 59479b7) | | |

### 1. `.dc` points (B-003, B-185)
- **How the sources write at each point.** At each point `runDCAnalysis()` resets every output port (pending
  entries dropped, emitted values forgotten) and pulses the CW sources' `reset`, so every CW source re-emits its
  value through its own port. That port's gate compares it with 0, so it is written; but a source that is not
  swept writes the value its net already holds, and an `sc_signal` notifies only a new value: nothing downstream
  runs again (B-185). A device output whose first value at the point is <= abstol is gated against 0 and not
  written at all (B-003).
- **Change (smallest that guarantees the rule for every net).** The optical nets are `OpticalNet`
  (`src/optical_net.h`; `spx::oa_signal_type`), an `sc_signal<OpticalSignal, SC_MANY_WRITERS>` with
  `writeWithEvent()`, whose `update()` notifies the value-change event also for an unchanged value (as
  `sc_buffer` does) when that write asked for it. `OpticalOutputPort::m_emit_each_wavelength` (set only by
  `runDCAnalysis()`): after a reset, the first entry of **each wavelength** (the `try_emplace` into
  `m_emitted_fields` creates it) is written whatever the gate says, snapped like any emitted value, with
  `writeWithEvent()`. Later entries of the point go through the gate as before. So every port emits each of its
  wavelengths at least once per point, with an event, and the rule holds for every net reachable from a source.
- **`.op` and `.tran`**: never set the flag; bit-identical on all 243 cases, counters identical.
- **Event counters** (`stats-compare`, before vs item 1): only the 3 multi-source `.dc` cases differ: their fixed
  wavelengths are propagated again at each point (probe-latch-dc: merger 5 -> 8 entries over 4 points;
  -swept-f 6 -> 10; -swept-wl: splitter 2 -> 8, merger1 5 -> 8, merger2 2 -> 8). 236 cases identical: in the
  single-source sweeps each point has a new wavelength, so every first write already passed the gate and changed
  the net.
- B-003 repro: point 1 now reads 0 W at 1557.63 nm (was 1 W at point 0's wavelength).
- Docs: `doc/netlist.md` (`.dc`: what a point is; PROBE in `.dc`), `doc/numerics.md` ("A `.dc` point" in
  "Convergence of `.op` and `.dc`"; `emitted` counter), CHANGELOG (Changed: B-185; Fixed: B-003).

### 2. Wavelength matching (B-010, B-062)
- `OpticalSignal::WAVELENGTH_MATCH_RTOL = 1e-12` (compile-time constant, documented in `src/optical_signal.h` and
  `doc/numerics.md` "Wavelength matching") and `sameWavelength(a, b)`: `a == b || |a-b| <= 1e-12*max(|a|,|b|)`.
- Registry: `getIDFromWavelength()` returns the first registered wavelength that matches (its value is kept).
  **Not on the per-event path** (confirmed by grep): callers are `CWSource`/`BitstreamSource::setWavelength()`
  (setup, and once per `.dc` point through the sweep's `apply`) and the `OpticalSignal(field, wavelength)`
  constructor (VLSRC values at parse time, legacy testbench stimuli). Devices work on ids.
- MLPROBE: `listedWavelength()` matches the list with the same rule once per wavelength id (cache
  `m_listed_by_id`), so the per-event cost stays one map lookup; samples are traced under the listed wavelength.
- Golden: predicted with a temporary instrumented build (reported every exact miss that the tolerance would match)
  before any output changed: 9 cases. Proof: in the 8 sweeps only the one point that drifted (B-009) to within
  1e-12 of the source's registered `wl=1.55e-6` changes; it now takes 1.55e-06, and its values are
  **bit-identical to the old binary's at exactly 1.55e-6** (checked on add_drop, 40 probe signals, and f05, 2).
- New backlog item **B-187** (low, open, not studied further unless an issue points to it): the tolerance can merge
  two genuinely different wavelengths closer than 1e-12 relative, or miss computed values that drift more.

## Decisions I had to take
1. **Snap on forced emissions.** The first value of a wavelength at a `.dc` point is written even below abstol;
   I apply the snap to it like to any emitted value, so a null reads exactly 0 (the B-003 one-line fix wrote the
   raw 1e-27 W). Most conservative reading of B-021 ("emitted values below snapfactor·abstol become 0").
2. **Forced emissions count as `emitted`** in `--stats` (not as `suppressed.snap` even when snapped to 0); no new
   counter, so the stats format is unchanged. Documented in `doc/numerics.md`.
3. **Event on unchanged values through a net subclass**, not by writing a placeholder value first (which would wake
   devices and probes with a fake value) and not by switching all nets to `sc_buffer` (which would change `.tran`).
4. `m_skip_next_convergence_check` (never set, B-027) now also uses `writeWithEvent()`/the snap if it is ever set;
   left in place, still dead.
5. **MLPROBE table rows keep the net's wavelength** (e.g. 1.5499999999999998e-06 in
   `regress/b062-mlprobe-computed-wavelength`), the VCD signals are the listed ones. Least change to the table.
   If several listed wavelengths match one value (only possible if the list itself has values within 2e-12), the
   smallest one is used.
6. **No transitive merging**: a value matches the first registered one within the tolerance, not a cluster (B-187).
7. Golden netlists whose comments described the old behaviour were updated (`probe-latch-dc.cir`,
   `probe-latch-dc-swept-wl.cir`, `b062-mlprobe-unlisted-wavelength.cir`); `regress/probe-latch-dc` was re-recorded
   for its manifest hash only. `b062-mlprobe-unlisted-wavelength.cir` gained `mlprobe0` (a near miss beyond the
   tolerance) so the case still checks the full-precision warning and its name order.
8. The swept-wavelength latch makes B-185's original expectation for `regress/probe-latch-dc` ("1550, 1550 nm")
   obsolete: that probe latches the swept wavelength (PD5 follow-up 2), so its output did not change.

## Found, not done
- `OpticalOutputPort::m_skip_next_convergence_check` is still never set (dead code, B-027).
- The ports' wavelength lists in `--stats` follow wavelength-id order, so a merged `.dc` point (registered first,
  by the source) is listed first; the order is not documented as meaningful.
- B-009 (accumulated `.dc` points) is what puts a sweep point within 1e-12 of the source's initial wavelength in
  `circuits/add_drop.cir`; computing start + i·step would make it exact (out of scope).
- MLPROBE in `.dc` stays disabled (B-186, separate decision).

## Final results
At 727646b (HEAD before this report):
- `pixi run test gcc`: 12/12 OK. `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: **243/243**
  (242 cases: 239 at c975167 + 3 new, b003-dc-dark-point, b010-near-duplicate-lambda, b062-mlprobe-computed-wavelength;
  plus the gtest case, 126 tests).
- `pixi run test asan-clang` and `pixi run test asan-gcc`: 12/12 OK, no sanitizer message in the test logs
  (`lsan.supp` unchanged). `pixi run test clang`: 12/12 OK.
- clang and asan-gcc, before (c975167, built in a temporary worktree) vs after, strict: the differences are
  exactly the predicted cases (the 8 add_drop/f05 sweeps, b062-mlprobe-unlisted-wavelength,
  probe-latch-dc-swept-wl) and the 3 new cases. Against the committed (gcc) baseline: 243/243 at the default
  tolerances; strict, the same 7 cases differ before and after (the known last-bit compiler differences, B-183).
- Counters (`stats-compare`, gcc): item 1 changes only the 3 multi-source `.dc` cases (explained above); item 2
  changes no counter (only the order of the ports' wavelength lists in 9 `.dc` cases).
