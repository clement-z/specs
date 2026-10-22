# B-065: Remove unconditional debug prints from my devices and elements
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 4a95b53, baseline 09d5304 (ws-cleanup C-08)
location: src/devices/crow.h:62,74, src/devices/crow.cpp:33,39,45, src/devices/mesh_col.cpp:176,220, src/devices/generic_transmission_device.cpp:157, src/parser/parse_element.cpp:1058,1247, src/specs.cpp:303-304,310-311
## Problem
Leftover debug output on stdout:
- CROW prints ring indices and child names.
- MeshCol prints "odd-even"/"odd-odd".
- GTD prints "i, j is active".
- PROBE creation prints "created <name>".
- PCMCELL creation prints "-----".
- `applyDefaultTraceFileToAllSignals()` prints every detector and power-meter name. These come out in pointer order, which is why tb/det and tb/detector need `unordered_streams`.
## Evidence
Golden stdouts:
- tb/ac_crow: 13 lines `0..3`, `wg_top_i`, `wg_bot_i`, `dc_i`.
- tb/det, tb/detector: `uut`, `uut2`.
- cir/add_drop_pyspecs, cir-json/add_drop_pyspecs, opt/mode-sampled-time, opt/tracefile-o: 3 x `created ROOT/PROBEn`.
- tb/mesh: 6 x `odd-even`/`odd-odd`.
## Expected
No debug output, or output gated on `specsGlobalConfig.verbose_component_initialization`.
## Suggested fix
Delete these statements: crow.h:62 and :74 (`cout << _i << endl;`), crow.cpp:33/39/45, mesh_col.cpp:176/220, generic_transmission_device.cpp:157, parse_element.cpp:1058 and :1247. In specs.cpp:301-313, delete the `string detname...; cout << detname` and `pwr_meter_name` lines and keep the `trace()` calls.

Skip `parse_element.cpp:1058` and `:1247`: they are covered by B-173.
## Golden impact
Removed debug noise only. Re-record the stdout of tb/ac_crow, tb/det, tb/detector, cir/add_drop_pyspecs, cir-json/add_drop_pyspecs, opt/mode-sampled-time, opt/tracefile-o, and tb/mesh (with B-047). `unordered_streams` can then be removed for tb/det and tb/detector.
## Related
B-066, B-072.

- Discovery seeds: S2.27 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-19.
## Verification
Golden stdouts grepped. This is safe because the statements only write to stdout.

**Independent second check (V1, of F-A3-19): CONFIRMED.** Method: deleted the 10 listed statements and the 4 specs.cpp lines on the scratch copy (each line verified by content before deletion), rebuilt, ran the golden check, and diffed the kept candidate against the baseline

- **Failing cases:** exactly the 8 claimed:
  - cir/add_drop_pyspecs, cir-json/add_drop_pyspecs, opt/tracefile-o, opt/mode-sampled-time: 3 x `created ROOT/PROBEn` removed.
  - tb/det and tb/detector: `uut`, `uut2` removed.
  - tb/ac_crow: 13 lines removed (0..3, wg_top/wg_bot/dc_0..2).
  - tb/mesh: 4 x odd-even, 2 x odd-odd and one leading blank line removed.
- Every diff line is a pure removal. Stderr and output files are identical, except the tb/det VCD, which differs only in the uninitialised t=0 garbage that is already ignored (B-054).
- The remaining tb/det stdout has no pointer-order-dependent lines, so dropping `unordered_streams` is plausible.
- With B-047 applied first, tb/mesh will show 5 such lines, not 6.
