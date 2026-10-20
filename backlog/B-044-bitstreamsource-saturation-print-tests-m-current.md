# B-044: BitstreamSource saturation print tests m_current_value instead of val
severity: low | category: bug | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/devices/bitstream_source.cpp:29
## Problem
In the bit dump loop over `m_values` (:26-35), saturation tests the wrong variable:
`if (m_current_value > max_val) val_saturated = max_val;`. The real emission path (:45) is correct. Only the
unconditional startup printout is affected, and `BitstreamSource` is not used anywhere (no tb, no parser element).
## Evidence
bitstream_source.cpp:26-30. `grep -rln BitstreamSource src` lists only its own two files.
## Expected
`if (val > max_val)`.
## Suggested fix
bitstream_source.cpp:29: `if (m_current_value > max_val)` becomes `if (val > max_val)`.
## Golden impact
None. The class is never instantiated.
## Related
B-043. Cross-area: bitstream_source.cpp:37 uses the same `--vci`-style cast (included in B-036).

- Discovery seeds: S2.26 (see `backlog/README.md` for seed numbering).
- Review ids: F-A2-14.
## Verification
Code reading.
