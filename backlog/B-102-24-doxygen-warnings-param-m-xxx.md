# B-102: 27 Doxygen warnings from wrong `@param m_xxx` names and "Constructor for Waveguide" titles in mzi_active.h
severity: low | category: docs | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 9b2422b
location: src/devices/mzi_active.h:38-45, :86-93, :149-156 (title also "Constructor for Waveguide" at :36, :84, :147)
## Problem
The three constructor comments document the members (`@param m_length_cm`) instead of the parameters (`length_cm`), so Doxygen emits 24 "argument ... of command @param is not found" warnings and drops the descriptions.
## Evidence
```
src/devices/mzi_active.h:37: warning: argument 'm_ps_sens_rad_v' of command @param is not found in the argument list of MZIActiveBase::MZIActiveBase(sc_module_name name, double ps_sens_rad_v=1, double length_cm=0, ...)
```
## Expected
Parameter names match.
## Suggested fix
`sed -i 's/@param m_/@param /' src/devices/mzi_active.h` (24 lines) and "Constructor for Waveguide" -> "Constructor for MZIActive".

Correction from the second check: the titles are at `mzi_active.h:35, :83, :146` (not :36/:84/:147). `sed -i 's/@param m_/@param /; s/Constructor for Waveguide/Constructor for MZIActive/' src/devices/mzi_active.h` changes exactly 27 lines (24 `@param` + 3 titles) and removes 27 Doxygen warnings (77 -> 50).
## Golden impact
none (comments only).
## Related
B-083, B-103.

- Review ids: F-A6-27.
## Verification
doxygen 1.9.1 warning list; parameter names of the three constructors compared line by line.

**Independent second check (V5, of F-A6-27): ADJUST.** Method: applied sed on a scratch copy; doxygen out of tree before/after

Real and fixed by the suggested sed. Before: 27 doxygen warnings mention mzi_active.h (24 "argument 'm_xxx' of command @param is not found" plus 3 "The following parameters of MZIActive*::MZIActive* are not documented"); after the fix all 27 disappear (total 77 -> 50). Adjust numbers: "24 warnings" should be "27 (24 + 3 'not documented' warnings)". Adjust lines: the "Constructor for Waveguide" titles are at :35, :83, :146 (not :36, :84, :147). The `@param m_` lines are at 38-45, 86-93, 149-156 as stated. `sed -i 's/@param m_/@param /; s/Constructor for Waveguide/Constructor for MZIActive/' src/devices/mzi_active.h` changes exactly 27 lines (24 params + 3 titles) and nothing else. Parameter names in all three constructors match the new @param names. Comment-only, so no golden or build impact.

**Orchestrator:** Counts and line numbers corrected (27 warnings, titles at :35/:83/:146).
