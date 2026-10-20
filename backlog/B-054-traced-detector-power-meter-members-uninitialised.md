# B-054: Traced detector/power-meter members are uninitialised (garbage at t=0 in the VCD)
severity: med | category: determinism | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 2997ba2
location: src/devices/detector.h:18-19, src/devices/power_meter.h:17, src/specs.cpp:301-313
## Problem
`Detector::m_cur_readout`, `m_cur_readout_no_interf` and `PowerMeter::m_cur_power` are plain `double` members with no initialiser. They are registered with `sc_trace` in `applyDefaultTraceFileToAllSignals()` (specs.cpp:301-313) and dumped in `$dumpvars` before anything writes them.
- The value depends on the memory: for example 6.9e-310 in tb/det.
- This is the reason `tests/golden/cases.json` has to exclude `uut*.readout*` in tb/det and tb/detector.
- A PDET that is never enabled (tb flows, `.op`/`.dc`) keeps the garbage for the whole run.
## Evidence
See the golden note on tb/det and tb/detector in cases.json ("garbage such as 6.9e-310"). By code reading, detector.h:18-19 and power_meter.h:17 have no initialiser and the constructors (detector.h:61-79, power_meter.h:28-35) do not set them.
## Expected
A deterministic 0 at t=0.
## Suggested fix
```
src/devices/detector.h:18-19
-    double m_cur_readout;
-    double m_cur_readout_no_interf;
+    double m_cur_readout = 0;
+    double m_cur_readout_no_interf = 0;
src/devices/power_meter.h:17
-    double m_cur_power;
+    double m_cur_power = 0;
```
## Golden impact
None for the comparison, since these signals are currently ignored. After the fix, the `vcd_ignore_values` entries and the NONDETERMINISTIC part of the tb/det and tb/detector notes can be dropped and the two cases re-recorded. The baseline VCDs contain garbage at t=0 for these signals.
## Related
B-055, B-056 (other detector nondeterminism).

- Discovery seeds: S4.03 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-08.
## Verification
I patched the detector part in my experimental build and the golden check passed. The fix is safe because the only observable change is the t=0 value of a variable that is read before being written today.

**Independent second check (V1, of F-A3-08): CONFIRMED.** Method: applied the fix on the scratch copy and ran the golden check (default and --atol 0: 44/44 pass with the existing vcd_ignore_values). I then made a cases.json copy without the vcd_ignore_values for tb/det and tb/detector, recorded twice and compared the two recordings

- **Baseline:** the tb/det VCD has `uut2.readout` = 6.905e-310 and `readout_no_interference` = 2.47e-323 at #0.
- **After the fix:**
  - Both are 0. Two recordings compare identical at --atol 0.
  - Against the current baseline without the ignore, tb/det and tb/detector fail at --atol 0 on exactly these t=0 values. They pass at the default atol 1e-12, because the garbage here happens to be denormal.
  - So the claimed impact holds: the comparison is unchanged now, and a re-record is needed to drop the ignores.
- **Initial value:** 0 is the neutral choice, and no equation or tolerance changes. Nothing reads these members before writing them except sc_trace.
- **Minor:** the constructor range is detector.h:61-88, not 61-79.
