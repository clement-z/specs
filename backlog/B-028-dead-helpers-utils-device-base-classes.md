# B-028: Dead helpers in utils and device base classes
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/utils/sysc_utils.h:11, src/utils/sysc_utils.cpp:6-8, src/utils/general_utils.cpp:1, src/devices/spx_module.h:22-31, src/devices/subcircuit_instance.h:15-17
## Problem
- `sc_object_is_module` is declared in the header but defined `inline` in the .cpp (unusable elsewhere) and unused.
- `general_utils.cpp` is an empty TU.
- `spx_module::flags`/`ModuleFlags` and `describe()` are never used (only GTD overrides `describe`; no caller).
- `SubcircuitInstance` is never instantiated (`XElement::create` exits), and its `signals`/`modules` members are unused.
- PQueue's `size()` shadows the base, and its iterator typedefs go through `std::priority_queue<T>`. Harmless; add a
  comment that mutating entries through `begin()` must not change the time key.
## Evidence
`grep -rn "sc_object_is_module\|ModuleFlags\|flags\b\|describe()" src`.
## Expected
none of the above.
## Suggested fix
Delete `sc_object_is_module` (h+cpp), `general_utils.cpp` (CMake globs sources; check the Meson list), `ModuleFlags`/`flags`,
and the unused `SubcircuitInstance` members (keep the class while parse_element.h:100 references it).
## Golden impact
None.
## Related
B-027.

- Review ids: F-A1-28.
## Verification
Grep. Compile-only change.
