# B-061: `prepareSimulation()` re-inits every spx_module, and composite `init()`s are not re-entrant
severity: med | category: memory/UB | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/specs.cpp:358-361, src/devices/mesh_col.cpp:53-69,85-98, src/devices/mzi.cpp:10-25, src/devices/clements.cpp:39-40, src/devices/generic_transmission_device.cpp:80-97, src/devices/detector.cpp:15
## Problem
`prepareSimulation()` calls `init()` on a snapshot of all `spx_module`s, iterated in pointer order (`std::set<spx_module*>`). Any module the user (C++ API, testbenches) has already initialised gets initialised again:
- `MZI::init` and `Clements::init` destroy and re-create their children, which may still be in the snapshot. This is the use-after-free of B-047.
- `MeshCol::init` appends duplicate MZIs and re-binds the old ones.
- `GenericTransmissionDevice::init` would spawn a second set of processes.
- `Detector::init` re-seeds the RNG; the detector thread also calls it (detector.cpp:15).

The init order depends on heap addresses, so any hidden ordering dependency is nondeterministic. Netlist flows are safe today only because the parser never calls `init()`.
## Evidence
B-047 (valgrind trace). Code: mesh_col.cpp:85-98 pushes into `m_mzi` and then indexes `m_mzi[i]` (old elements) on a second call; mzi.cpp:10-15 `reset()`s its children.
## Expected
Calling `init()` twice is harmless. The framework init order is deterministic.
## Suggested fix
Options:
- A non-virtual `spx_module::initialize()` with an `m_initialized` guard, called by `prepareSimulation()` and by composites.
- Per-class guards.
- Have `prepareSimulation()` skip modules that are children of an initialised composite.

In addition, iterate in a deterministic order (for example by `name()`).
## Golden impact
None expected if the guard only suppresses second calls.
## Related
B-047, B-055.

- Discovery seeds: S2.16 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-15.
## Verification
Code reading and the B-047 traces.
