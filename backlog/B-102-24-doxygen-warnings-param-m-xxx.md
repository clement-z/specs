# B-102: 24 Doxygen warnings from `@param m_xxx` names in mzi_active.h
severity: low | category: docs | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
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
## Golden impact
none (comments only).
## Related
B-083, B-103.

- Review ids: F-A6-27.
## Verification
doxygen 1.9.1 warning list; parameter names of the three constructors compared line by line.
