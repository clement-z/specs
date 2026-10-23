# PD5 follow-up (after the gate)

Branch `ws/probes`, on top of the gated tip bc598dc. Details: `PD5-REPORT.md`, section "Follow-up".

## What changed
1. **No PROBE wavelength selection in v0.3.0** (1f44df4):
   - PROBE's `wl` / `wavelength` keyword is parsed exactly as on e5d6c21 (boolean: trace the wavelength or not);
   - removed the selection code (`Probe::m_wavelength_m`), the gtest
     `Probe.SelectedWavelengthIsTheOnlyOneTraced` and the golden case `err/probe-wl-invalid`;
   - docs and CHANGELOG no longer mention selecting a wavelength; `doc/netlist.md` points to MLPROBE for a given
     wavelength;
   - the latch, its warnings and B-184 are unchanged.
2. **B-185** (ddd59de): backlog item (open, needs-design) for the `.dc` limit. A net whose value does not change
   between points is not written again, so a probe sees a non-swept wavelength only at the first point. Evidence:
   `regress/probe-latch-dc`; documented in `doc/netlist.md`.
3. Prediction d145ceb; report follow-up section 4733ba5.

## Golden impact (strict, gcc)
- `err/probe-wl-invalid` removed (netlist, baseline directory, manifest case list); gtest list re-recorded.
- **Not in the coordinator's list:** `regress/probe-latch-tran` and `regress/probe-latch-dc` (added in PD5) used
  `probe2 ... wl=<wavelength>`. `probe2` is removed from both netlists and the two cases are re-recorded.
  - Proof: VCD, CSV and stderr equal the previous recording minus every `PROBE2` signal, row and warning.
  - stdout differs only in the parse listing (PROBE2's line, element and reader counts).
  - The run manifest differs only in the netlist hash.
- All other cases are strictly identical: `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0` gives 232
  passed, 0 failed/new/missing.

## Checks (tip 1f44df4)
- `pixi run test gcc`: 12/12. `pixi run test asan-clang`: 12/12, no sanitizer message.
- Event counters: 228 same vs e5d6c21; 231 same vs the gated tip.
- clang release and asan-gcc, strict recordings vs the gated tip: 229 identical. Only `regress/probe-latch-tran`
  and `regress/probe-latch-dc` differ. The 5 new or re-recorded PD5 cases pass strict against the gcc baseline
  on both.
- Old-style `wl=` values (`-1.55e-6`, `1.56e-6`, `0`): same exit status, stdout and traced variables as e5d6c21.

No push, no merge.
