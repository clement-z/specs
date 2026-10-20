# B-073: Behaviour-neutral cleanups in probes, detector, sources and specs.cpp
severity: low | category: style/maintainability | effort: S | fix-class: easy-unattended | verdict: CONFIRMED | status: deferred to ws-cleanup (style); after B-068
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

Corrections from the second check (apply them):
- `probe.cpp:115-117` must be **115-116**: line 117 (`m_lambda_signals.insert(...)`) is live code; deleting it breaks MLPROBE (tb/lambda fails).
- **Keep** the `isnan(s.getWavelength())` guard at `probe.cpp:35`: a NaN wavelength can reach `wavelengths_vector` from a netlist (`.assign big = 1e308*10`, `.assign x = {big}-{big}`, `cwsrc1 in wl={x}`).
- Removing `MLambdaProbe::m_trace_sig_power/modulus/phase` removes 3 auto-named child `sc_signal`s; no observable output changes.
- **The `__modname` refactor is split out as F-Z-02**; do not do it here.
- Apply B-068 first: `probe.cpp:52, 58, 65, 71` are inside the code B-068 deletes.
## Golden impact
None. All of these are compile-time-only or dead paths; generated names are unchanged.
## Related
B-065, B-068.

- Review ids: F-A3-27.
## Verification
Code reading. This is safe because no executed statement changes.

**Independent second check (V1, of F-A3-27): ADJUST.** Method: applied all the cleanups on the scratch copy (golden 44/44 pass, also with --atol 0), then the `__modname` -> `spx_child_name` refactor on top (76 call sites in 8 files; golden 44/44 pass). I instrumented the base and refactored builds to dump every sc_object name after prepareSimulation, and compared tb/mesh (with the B-047 fix) and tb/lambda. I also tested deleting probe.cpp:117 and tested a NaN wavelength

three things in the finding are wrong or unsafe, and the suggested fix must be corrected:
1. **Location "probe.cpp:115-117" must be "115-116".**
   - Line 117 is live code (`m_lambda_signals.insert({wl, std::move(temp_vector)});`), not a comment.
   - Deleting it breaks MLPROBE: no wavelength is ever registered, every event prints "received unsupported signal" and nothing is traced.
   - The golden check does catch this (tb/lambda: stdout 174 -> 226 lines), but the finding as written instructs the deletion.
2. **The `isnan(s.getWavelength())` guard at probe.cpp:35 is not dead.**
   - getWavelength() returns the stored value, and a NaN wavelength can enter `wavelengths_vector` from a netlist. For example, `.assign big = 1e308*10` / `.assign x = {big}-{big}` / `cwsrc1 in wl={x} power=1e-3` emits `@ -nan` (CWSource::setWavelength -> getIDFromWavelength pushes the NaN).
   - Change the problem text and state explicitly: "keep the isnan check in Probe::on_port_in_changed". The copies at :58 and :71 disappear with B-068.
3. **MLambdaProbe::m_trace_sig_power/modulus/phase are sc_signal objects, not compile-time-only.**
   - Removing them removes 3 auto-named child objects (`<mlprobe>.signal_0..2`). This is verified on tb/lambda: 89 -> 86 objects, with otherwise identical names and stdout.
   - There is no observable output change, but the golden-impact wording "compile-time-only" should say "no observable output change".

Other notes:
- **`__modname` refactor:**
  - The suggested helper `spx_child_name(const sc_object*, const std::string &sep_suffix, size_t idx)` works. Fold the separator into sep_suffix: "_X_" for the generic macro and "_IN_"/"_OUT_"/"_OOP_" for GTD.
  - With that, all 1003 object names in tb/mesh and all tb/lambda names are identical to the base build.
  - The golden suite cannot check these names on its own: tb/mesh crashes until B-047, and OCTANE and GTD are not reachable. Either land it after B-047 and diff object names, or split it into its own item (it is a refactor, not dead-code removal).
- **Ordering:** probe.cpp:52, 58, 65 and 71 overlap with B-068. Apply B-068 first.
- **Includes:** all the removed includes (`<cstdlib>` x3, `<fstream>` x3, `<random>` in power_meter.h and power_meter.cpp) compile cleanly. The specs target builds with no new warnings. unit_tests was not built, because it does not compile at HEAD.
- **Other locations:** all the other cited lines are correct, including the specs.cpp:132 comment and the 269-273 block.
- **Fix-class:** easy-unattended is acceptable once (1) and (2) are corrected. Effort S is fine without the refactor; with it, M.

**Orchestrator:** Suggested fix corrected (keep probe.cpp:117 and the isnan guard); `__modname` refactor split out to F-Z-02.
