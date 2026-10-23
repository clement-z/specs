# PF03 progress (S-parameter device `SPAR`, branch `ws/spar`)

Heartbeat: `388e4984` (PF03-heartbeat, every 5 min, expires in 48 h). Delete it after the report is committed.

Baseline before any change (cb3a5c3): `pixi run test gcc` OK, `pixi run golden gcc -- --rtol 0 --atol 0
--print-slack 0`: 245 passed; `pixi run test asan-clang` OK.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 1 | Loader (`src/io/sparam_file.{h,cpp}`): .sparam, Touchstone 1/2, CSV, Taylor | done | 2ecab2f | 13 gtest cases |
| 2 | Phase convention per format + proof test | done | 9aa4624 | fixtures b802d70 (subagent); test_sparam.py phase-dc/phase-tran |
| 3 | `SParamDevice` (uni/bi, per-wavelength cache, delays) | done | 9aa4624, 44a830a | warnings fixed in 44a830a |
| 4 | Netlist element `SPAR`, docs, KiCad symbol | done | 9aa4624, bb19f8d | |
| 5 | Export of a `.dc` sweep (`utils/sparam_export.py`) | done | 838b917 | subagent, reviewed |
| 6 | Tests: loader gtest, phase convention, add_drop round trip, passivity, golden | done | 44a830a | 11 golden cases + gtest re-recorded (new only) |
| 7 | Docs: `doc/netlist.md`, CHANGELOG, F-03 brief status | done | bb19f8d | |

## Decisions taken so far (go into the report)
- Lumerical `.sparam` phase convention: physics (exp(-i w t)); evidence: Lumerical's FFT convention (forward
  transform with exp(+i w t)) and the SiEPIC `EBeam_1550_TE_BDC.sparam` (Simphony repository) whose
  transmission phases grow with frequency (dphi/domega = +0.96 ps). Same propagation sign as SPECS: kept.
- Touchstone: RF engineering convention (exp(+j w t), delay line S21 = exp(-j w tau), scikit-rf): conjugated.
- Third-party fixture `EBeam_1550_TE_BDC.sparam`: not committed. Simphony is MIT but its LICENSE puts
  "resources" under other terms in a NOTICE.txt that no longer exists (404). Used only in /tmp as a local check.

## Gate (in progress)
- gcc: meson test 13/13, golden strict 256 (245 + 11 new) after recording.
- asan-clang: meson test 13/13 OK. clang: meson test OK; golden strict: the same 7 cases fail before (cb3a5c3)
  and after (last-bit clang differences), the 11 new cases pass.
- Remaining: asan-gcc; before/after `compare` of the clang and asan-gcc recordings; report.
