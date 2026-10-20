# B-047: `specs -t mesh` segfaults: Clements is init()'d twice (use-after-free)
severity: high | category: memory/UB | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/tb/mesh_tb.cpp:91, src/specs.cpp:358-361, src/devices/mzi.cpp:10-25, src/devices/mesh_col.cpp:53-69,85-98
## Problem
`mesh_tb_run()` calls `mesh1.init()` (mesh_tb.cpp:91), and then `SPECSConfig::prepareSimulation()` calls `init()` a second time on every `spx_module`. It first takes a snapshot of them in a `std::set<spx_module*>`, ordered by pointer value (specs.cpp:359-361). The second pass re-enters the composite inits:
- `MeshCol::init()` does not clear `m_mzi`/`m_wg`. It push_backs N/2 new MZIs with the same names, which causes the 81 W505 warnings, and it re-binds and re-inits the *old* `m_mzi[i]`.
- `MZI::init()` then `reset()`s and re-creates its DC/PS/WG children (mzi.cpp:10-25). Those children are still in the snapshot.
- The loop's next `mod->init()` dispatches through a freed vtable, which is a jump to address 0.

The root cause is therefore not `Clements::init()` itself. In the observed run it is never reached before the crash. The trigger is the second init cascade freeing objects that are still in the snapshot.
## Evidence
```
$ cd run && specs -t mesh     -> exit 139, 81 x W505, then SIGSEGV
gdb: #0 0x0000000000000000 in ?? ()  #1 SPECSConfig::prepareSimulation()  #2 mesh_tb_run()
gdb breakpoints during prepareSimulation: MeshCol::init this=0x573830 -> MZI::init 0x576330,
  MZI::init 0x5889b0 (called from MeshCol::init) -> MZI::init 0x576330 (from the loop) -> SIGSEGV
valgrind: Invalid read of size 8 at SPECSConfig::prepareSimulation()
  Address ... 0 bytes inside a block of size 1,392 free'd at operator delete
  by MZI::init() by MeshCol::init_N_odd_j_even() by SPECSConfig::prepareSimulation()
  Block was alloc'd ... by MZI::init() by MeshCol::init_N_odd_j_even() by Clements::init() by mesh_tb_run()
```
## Expected
`specs -t mesh` runs. Each composite is initialised exactly once.
## Suggested fix
Delete the manual call. `prepareSimulation()` already performs it, and `Clements::init()` initialises its MeshCol and MZI children, which are created after the snapshot and so are not re-initialised:
```
--- src/tb/mesh_tb.cpp:91
-    mesh1.init();
```
Hardening `init()` against re-entry is a separate design item (B-061).
## Golden impact
`tb/mesh` changes from `signal:SIGSEGV` to exit 0, with no W505 warnings and a new `traces/mesh_tb.vcd`. Re-record that case and remove `"expect": "crash"` and the note from `tests/golden/cases.json`. No other case changes: I ran the full golden check with a build that carried this patch plus other experimental ones, and 43/44 passed, only tb/mesh differed.
## Related
B-061 (non-idempotent `init()` / snapshot hazard), B-070 (Clements physics). The golden note in cases.json ("Clements::init() re-creates children") is slightly inaccurate: the first free is done by `MZI::init` via `MeshCol::init`.

- Discovery seeds: S4.02, S2.16 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-01.
## Verification
I patched a copy (`/tmp/ws-review-A3/exp`, Release build) and ran `specs -t mesh`:
- exit 0, 0 W505.
- At 167 ps OUT1..OUT5 = 1 W each, from 5 x 1 W inputs, so power is conserved.

This is safe because the only effect of line 91 is the redundant first init. `prepareSimulation()` is always called by the tb (mesh_tb.cpp:121).
