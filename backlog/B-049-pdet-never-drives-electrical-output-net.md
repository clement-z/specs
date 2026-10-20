# B-049: PDET never drives its electrical output net (`p_readout->write` commented out)
severity: high | category: bug | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/devices/detector.cpp:82-84, src/parser/parse_element.cpp:1261, src/tb/detector_tb.cpp:89-95
## Problem
`Detector::on_time_tick()` computes `m_cur_readout` but the write to `p_readout` is commented out (detector.cpp:84). The PDET's second net, which the parser binds as a writer (parse_element.cpp:1261), therefore stays 0 forever. The readout reaches only the VCD, through `Detector::trace()`. Any electrical consumer of a photodetector sees 0: a PSHIFT, an MZI `vin`, the OCTANE `p_readout` chain, or a subcircuit port.

S4.10 also says the `detector_tb` self-checks fail because of this. That is only part of the story: the tb never sets `uut.enable`/`uut2.enable`, so its detectors never sample at all. With the write restored, `specs -t det` still prints "Failure!".
## Evidence
`backlog/repro/a3-pdet-readout.cir`: `pdet1 in vout` with 1 mW on `in`, and `pshift1 in2 out2 vout sensitivity=1000`.
- VCD: `ROOT/PDET1.readout` = 0.001 from 1 ps, but the `out2` phase stays 0 after the 10 ps pulse.
- With line 84 uncommented (patched build), the `out2` phase = 0.9999999999999999 rad.
## Expected
The PDET output net carries the photocurrent.
## Suggested fix
Restore `p_readout->write(m_cur_readout);`. This is a design decision rather than a one-liner, for two reasons:
1. The readout is recomputed every Ts (1 ps) and jitters in the last ulp (B-050). Writing it as-is wakes every downstream reader every Ts.
   - Option: write only on change beyond a tolerance.
   - Option: write only on input events.
2. Detectors are only enabled by `runTRANAnalysis()`. C++/testbench users (detector_tb, OctaneCell) must enable them, or `Detector` should default to enabled outside the analysis flow.

Also fix detector_tb to set `enable`.
## Golden impact
None: the full golden check with the write restored passed (tb/det and tb/detector detectors are never enabled, and no golden netlist has a PDET).
## Related
B-050, B-054, B-069 (OCTANE uses `p_readout`).

- Discovery seeds: S2.02, S4.10 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-03.
## Verification
I ran the repro with the release binary and with a patched build: the `out2` phase is 0 versus 1 rad. I ran `specs -t det` with the patched build: still "Failure!", with READOUT 0.

**Independent second check (V1, of F-A3-03): CONFIRMED.** Method: ran a3-pdet-readout.cir with the release binary; uncommented detector.cpp:84 on the scratch copy, rebuilt, ran the golden check, the repro and `specs -t det`

- **Before:** `ROOT/PDET1.readout` = 0.001 from #10, but `PROBE{ROOT/out2}.phase` stays 0.
- **Code:**
  - parse_element.cpp:1261 binds p_readout AS_WRITER.
  - detector.cpp:84 is commented out.
- **With the write restored:**
  - The out2 phase = 0.9999999999999999 rad.
  - Golden 44/44 pass (also with --atol 0), so the "None" impact holds.
  - `specs -t det` still prints "Failure!": detector_tb.cpp never touches `enable`.
- **Severity and class:** high and needs-design are justified. The readout jitter is also visible: 0.0009999999999999998 / 0.001 / 0.0009999999999999996.
