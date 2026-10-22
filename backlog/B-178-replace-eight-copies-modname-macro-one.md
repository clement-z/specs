# B-178: Replace the eight copies of the `__modname` macro with one helper in spx_module.h
severity: low | category: style/maintainability | effort: S | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in e1b7e9a (ws-cleanup C-07)
location: src/devices/probe.cpp, src/devices/octane_cell.cpp, src/devices/octane_segment.cpp, src/devices/octane_matrix.cpp, src/devices/clements.cpp, src/devices/mesh_col.cpp, src/devices/mzi.cpp, src/devices/generic_transmission_device.cpp, src/devices/spx_module.h
## Problem
The `__modname` macro (child-object name builder) is redefined in 8 `.cpp` files (76 call sites). Identifiers with a
leading double underscore are reserved in C++, and the copies can drift. Split out of B-073 by the orchestrator
because it is a refactor, not a dead-code removal, and because its safety check differs (object names).
## Evidence
`grep -n -A2 "define __modname" src/devices/*.cpp` lists the 8 definitions. Seven build
`name() + "_" + SUFFIX + to_string(IDX)`; the GTD one (`generic_transmission_device.cpp:3-4`) builds
`name() + SUFFIX + "_" + to_string(IDX)`. In `mzi.cpp` the macro is defined but never used.
## Expected
One helper, identical object names.
## Suggested fix
Add to `src/devices/spx_module.h`:
`inline std::string spx_child_name(const sc_core::sc_object *o, const std::string &sep_suffix, size_t idx)`
returning `std::string(o->name()) + sep_suffix + std::to_string(idx)`. Fold the separator into `sep_suffix`: pass
`"_" + SUFFIX` where the generic macro was used, and `SUFFIX + "_"` in GTD. Use it as
`spx_child_name(this, std::string("_") + SUFFIX, i).c_str()` inside the same full-expression as before (the macro also returned the
`c_str()` of a temporary). Remove the 8 `#define __modname` blocks (the one in `mzi.cpp` is unused). Keep every
generated name byte-identical.
## Golden impact
None, provided names are identical. **Land after B-047** (tb/mesh is the only golden case that exercises these
names, through its VCD scope names, and it crashes until B-047 is fixed); OCTANE and GTD are unreachable.
## Related
B-073 (the cleanup this was split from), B-047.

- Review ids: F-Z-02.
## Verification
The second checker (V1) applied this refactor on top of the B-073 cleanups and the B-047 fix, instrumented the
base and refactored builds to dump every `sc_object` name after `prepareSimulation`, and found all 1003 names in
tb/mesh and all tb/lambda names identical; golden 44/44.
