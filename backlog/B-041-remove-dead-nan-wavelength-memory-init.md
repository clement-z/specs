# B-041: Remove dead "nan wavelength" memory init in DirectionalCoupler and Merger
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 39e7734 (ws-cleanup C-02)
location: src/devices/directional_coupler.cpp:11, src/devices/directional_coupler.cpp:75, src/devices/directional_coupler.cpp:119, src/devices/directional_coupler.cpp:193, src/devices/directional_coupler.cpp:237, src/devices/directional_coupler.cpp:281, src/devices/merger.cpp:9, src/devices/merger.cpp:47
## Problem
`m_memory_inX[0] = 0; // initializing for nan wavelength` refers to a "NaN wavelength" id that does not exist: id 0
is the first registered wavelength (optical_signal.h:31-40). The line is also redundant, because these devices only
access the maps through `operator[]`, which default-inserts 0, and they never iterate them.
## Evidence
`grep -n "initializing for nan" src/devices/*.cpp`. The DC and Merger maps are only read with `[cur_wavelength_id]`.
## Expected
No misleading dead code.
## Suggested fix
Delete the 8 listed lines.
## Golden impact
None. I verified this with a scratch build where all 11 such lines (these 8 plus the 3 in B-042) were removed:
- every DC/Merger case passed (tb/dc, tb/directional_coupler, tb/merger, tb/merg, tb/lambda, tb/ring, tb/ac_*,
  tb/mzi, cir/*, opt/*);
- the only failures were tb/ps and tb/phaseshifter, due to the PS lines (B-042).

These 8 lines construct no OpticalSignal and schedule no event.
## Related
B-042 (same pattern in PhaseShifter, but not neutral for golden stdout). Discover 02 §3 "NaN wavelength sentinel".

- Review ids: F-A2-11.
## Verification
Golden check with the removal: 42 PASS, 2 FAIL (PS only, IDs only). Code reading confirms no other use.

**Independent second check (V4, of F-A2-11): CONFIRMED.** Method: Deleted the 8 lines (DC :11/:75/:119/:193/:237/:281, Merger :9/:47), rebuilt. Golden check with --atol 0 --print-slack 0 --strict-order: 44/44 PASS.

- The maps are `std::map<uint32_t, complex>` (directional_coupler.h:48-49 and :98-101, merger.h:28-29). They are only accessed with `operator[](cur_wavelength_id)`, which value-initialises to 0, and never iterated.
- Each map is written only by its own thread, and there is no dont_initialize.
- So the change is strictly neutral.
- The 3 PhaseShifter lines (B-042) were left untouched.
