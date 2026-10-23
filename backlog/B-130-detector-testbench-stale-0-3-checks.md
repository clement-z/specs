# B-130: Detector testbench is stale: 0 of 3 checks run, "Failure!" printed on every run, no summary line
severity: low | category: test-gap | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: fixed in be96f26 (B-049/B-050 event-driven readout; the tb enables the detector): tb/detector prints Success rate 3/3
location: src/tb/detector_tb.cpp:20-79 (checks at :35,:47,:59), :87-123 (setup), src/devices/detector.cpp:40-45,84
## Problem
`specs -t detector` prints `Failure! Expected 1A as both output currents!` at event 2 and never reaches the "Success rate" line. The testbench expects `p_readout` events, but `Detector::on_time_tick` has the `p_readout->write(...)` commented out (detector.cpp:84), and the tb never sets `uut.enable` (the thread waits for `enable.posedge_event()`, detector.cpp:41-45), because it calls `prepareSimulation()` + `sc_start()` instead of the TRAN flow that enables detectors. The monitor therefore only sees the 3 input events, so event 2 (IN falling to 0) is compared with the expected 1 A. The failure is recorded as the golden baseline of tb/det and tb/detector. The same two detectors also dump uninitialised `m_cur_readout*` (A3 area).
## Evidence
```
0 s: IN [0006] 1 W ... COUNT: 1
100 ms: IN 0 W ... COUNT: 2
-----------------/! \---------------
Failure!
Expected 1A as both output currents!
200 ms: ... COUNT: 3          (end; no "Test finished!", no "Success rate")
```
## Expected
The testbench passes (3/3) or is removed from the registry.
## Suggested fix
Depends on the intended detector semantics (is the commented `p_readout->write` intentional?). If the detector is meant to write: set `uut.enable = sc_logic(1)` in the tb, re-enable the write, re-derive the event numbers or use B-019's `tb_check` with time-based checks. Otherwise delete `detector`/`det` from `tb_map`.
## Golden impact
tb/det, tb/detector stdout and VCD change; re-record.
## Related
B-019; reviewer A3 (detector.cpp:84, uninitialised members, `a3-pdet-readout.cir`).

- Review ids: F-A5-10.
## Verification
Ran `specs -t detector` (output above); read detector.cpp.
