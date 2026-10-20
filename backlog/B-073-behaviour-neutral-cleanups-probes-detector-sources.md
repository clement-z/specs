# B-073: Behaviour-neutral cleanups in probes, detector, sources and specs.cpp
severity: low | category: style/maintainability | effort: S | fix-class: easy-unattended | verdict: CONFIRMED
location: src/devices/probe.h:107-109,115, src/devices/probe.cpp:3,35,44,52,58,65,71,76,93-95,115-117,124, src/devices/detector.h:4,48, src/devices/detector.cpp:3,82,114-130, src/devices/power_meter.h:4-5,10, src/devices/power_meter.cpp:3-4, src/devices/cw_source.h:4, src/devices/value_list_source.cpp:14-19, src/devices/electrical_value_list_source.cpp:14-19, src/specs.cpp:132-136,269-273, src/devices/{probe,octane_cell,octane_segment,octane_matrix,clements,mesh_col,mzi,generic_transmission_device}.cpp (`__modname`)
## Problem
Dead code and noise in my area:
- **probe.h/probe.cpp:**
  - `MLambdaProbe::m_trace_sig_power/modulus/phase` (probe.h:107-109) are unused.
  - `prepare()` is declared (probe.h:115) but never defined.
  - The `isnan(s.getWavelength())` checks (probe.cpp:35) can never be true, because `getWavelength()` exits on unknown ids.
  - Commented-out code at probe.cpp:44, 52, 65, 93-95, 115-117, 124.
- **detector.cpp:**
  - `wavelength_dependent_responsivity()` (detector.cpp:114-130) is never called.
  - The commented-out line at detector.cpp:82.
- **Unused includes:** `<cstdlib> // system()` (probe.cpp:3, detector.cpp:3, power_meter.cpp:3); `<fstream>` (cw_source.h:4, detector.h:4, power_meter.h:4); `<random>` (power_meter.h:5).
- **Include style:** power_meter.h:10 includes `"spx_module.h"` instead of `"devices/spx_module.h"`.
- **`if (false)` blocks** in VL/EVL runner (value_list_source.cpp:14-19, electrical_value_list_source.cpp:14-19).
- **specs.cpp:**
  - The comment "Enable photodetectors" at specs.cpp:132 sits above code that disables them.
  - The commented MLPROBE block at specs.cpp:269-273.
- **The `__modname` macro** is redefined in 8 .cpp files. The GTD variant has a different suffix order.
## Evidence
grep, and the code at the cited lines.
## Expected
Less noise and one helper.
## Suggested fix
- Delete the listed dead members, declarations, includes, comments and blocks.
- Fix the specs.cpp:132 comment to "Disable photodetectors".
- Replace `__modname` with one inline helper in spx_module.h, keeping the GTD name format: `inline std::string spx_child_name(const sc_core::sc_object *o, const std::string &sep_suffix, size_t idx)`, used as `spx_child_name(this, "_IN_", i).c_str()`.
- Do not touch `isnan` semantics elsewhere (A1).
## Golden impact
None. All of these are compile-time-only or dead paths; generated names are unchanged.
## Related
B-065, B-068.

- Review ids: F-A3-27.
## Verification
Code reading. This is safe because no executed statement changes.
