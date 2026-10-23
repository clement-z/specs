# PF03 progress (S-parameter device `SPAR`, branch `ws/spar`)

Heartbeat: `388e4984` (PF03-heartbeat, every 5 min, expires in 48 h). Delete it after the report is committed.

Baseline before any change (cb3a5c3): `pixi run test gcc` OK, `pixi run golden gcc -- --rtol 0 --atol 0
--print-slack 0`: 245 passed; `pixi run test asan-clang` OK.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 1 | Loader (`src/io/sparam_file.{h,cpp}`): .sparam, Touchstone 1/2, CSV, Taylor | todo | | |
| 2 | Phase convention per format + proof test | todo | | Lumerical: physics exp(-iwt), no conjugation; Touchstone: engineering exp(+jwt), conjugated |
| 3 | `SParamDevice` (uni/bi, per-wavelength cache, delays) | todo | | |
| 4 | Netlist element `SPAR`, docs, KiCad symbol | todo | | |
| 5 | Export of a `.dc` sweep (`utils/sparam_export.py`) | todo | | |
| 6 | Tests: loader gtest, phase convention, add_drop round trip, passivity, golden | todo | | |
| 7 | Docs: `doc/netlist.md`, CHANGELOG, F-03 brief status | todo | | |

## Decisions taken so far (go into the report)
- Lumerical `.sparam` phase convention: physics (exp(-i w t)); evidence: Lumerical's FFT convention (forward
  transform with exp(+i w t)) and the SiEPIC `EBeam_1550_TE_BDC.sparam` (Simphony repository) whose
  transmission phases grow with frequency (dphi/domega = +0.96 ps). Same propagation sign as SPECS: kept.
- Touchstone: RF engineering convention (exp(+j w t), delay line S21 = exp(-j w tau), scikit-rf): conjugated.
- Third-party fixture `EBeam_1550_TE_BDC.sparam`: not committed. Simphony is MIT but its LICENSE puts
  "resources" under other terms in a NOTICE.txt that no longer exists (404). Used only in /tmp as a local check.
