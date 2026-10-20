# B-072: CROW children are not prefixed with the instance name; dead `m_converger` store
severity: low | category: bug | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/devices/crow.h:63-69,75, src/devices/crow.cpp:96
## Problem
- `CROW::init()` names its signals and submodules `S_TL_i`, `wg_top_i`, `dc_i`, and so on, without the instance name. A second CROW in the same simulation produces W505 duplicate-name warnings and auto-renamed objects.
- `dc.at(0)->m_out1_writer.m_converger = false;` (crow.cpp:96) assigns the default value to a field that nothing reads (`m_converger` is only declared at optical_output_port.h:78).
## Evidence
crow.h:63-69 and :75 use `string("S_TL") + "_" + _i` etc. A grep for `m_converger` finds only the declaration and this store.
## Expected
Hierarchical names, as the other composites do, and no dead stores.
## Suggested fix
At crow.h:63-69 and :75, prefix each name with the instance name: `(string(name()) + "_S_TL_" + _i).c_str()`, and the same for `S_BL`, `S_TR`, `S_BR`, `wg_bot`, `wg_top`, `dc`. Delete crow.cpp:96.
## Golden impact
None if B-065 lands first. Otherwise the tb/ac_crow stdout child names printed at crow.cpp:33/39/45 change, for example `crow_wg_top_0`. These objects are not in the tb/ac_crow VCD (checked).
## Related
B-065, B-071.

- Overlap: the `crow.cpp:96` deletion is also part of B-027.

- Discovery seeds: S1.30 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-26.
## Verification
Code reading and inspection of the baseline VCD scopes for tb/ac_crow (only `pcross`/`pthrough`). This is safe because only object names change and nothing looks them up.
