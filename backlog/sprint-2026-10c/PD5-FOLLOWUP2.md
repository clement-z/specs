# PD5 follow-up 2: `.dc` latch on the swept wavelength

Branch `ws/probes`, on top of 79a3580 (follow-up 1). Details: `PD5-REPORT.md`, section "Follow-up 2".

## What changed (3152794)
- **Rule.** In a `.dc` that sweeps a source's wavelength or frequency (`WL(src)`, `F(src)`, `/src/WL`, `/src/F`),
  every plain PROBE latches, at each point, the swept wavelength when its net carries it at that point; otherwise
  the smallest wavelength present. Power sweeps, `.op` and `.tran` are unchanged.
- **Explicit API, no global:**
  - the `.dc` sweep order is a struct (`CWSweepOrder`) that holds the swept `CWSource` and whether the sweep
    changes its wavelength;
  - `runDCAnalysis()` calls `Probe::setSweptWavelength()` on every probe before each point.
- **Tests:** gtest `Probe.LatchPerTimeStepPrefersTheSweptWavelengthOfADcPoint`; golden cases
  `regress/probe-latch-dc-swept-wl` and `regress/probe-latch-dc-swept-f` (a fixed 1530 nm next to the swept range).
  The first point now shows the swept wavelength; 1f44df4 showed the fixed one.
- **Docs:** `doc/netlist.md` (latch rule in `.dc`), CHANGELOG `[unreleased]`; B-185 updated (first-point part
  solved, unchanged-net part still open) and its `backlog.md` row.
- Prediction 17b9959; report section 2a5adef.

## Golden impact (predicted before the change)
- **No existing case changes**, `regress/probe-latch-dc` included: at its first point the swept 1541 nm is already
  the smallest (the fixed source is at 1550 nm), and later points carry only the swept wavelength. All WL-sweep
  cases and `cir/add_drop*` have a single source.
- New cases: the two above. gtest list re-recorded.

## Checks (tip 3152794)
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: 234 passed, 0 failed/new/missing.
- `pixi run test gcc`: 12/12. `pixi run test asan-clang`: 12/12, no sanitizer message.
- Event counters: 231 same vs 1f44df4 (+2 new); 228 same vs e5d6c21.
- clang release and asan-gcc, strict recordings vs 1f44df4: 231/231 identical (+2 new). The 7 new or re-recorded
  PD5 cases pass strict against the gcc baseline on both.

No push, no merge.
