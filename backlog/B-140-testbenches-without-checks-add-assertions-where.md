# B-140: testbenches without checks: add assertions where the expected value is unambiguous
severity: low | category: test-gap | effort: M | fix-class: needs-design | verdict: PLAUSIBLE
location: src/tb/mzi_tb.cpp:19-30, src/tb/ring_tb.cpp:19-30, src/tb/lambda_tb.cpp, src/tb/freqsweep_tb.cpp:58-66, src/tb/crow_tb.cpp:139-157, src/tb/mesh_tb.cpp
## Problem
mzi, ring, lambda, ac_add_drop, ac_crow and mesh have a `monitor()` that only prints (ac_* have an empty one), so only the golden diff protects them.
## Evidence
Observed steady states that match closed forms: mzi (1550 nm, 1 W): theta=0 -> OUT1/OUT2 = 0/1 W, theta=pi/2 -> 0.5/0.5, theta=pi -> 1/0 (i.e. P_out1 = sin^2(theta/2)), tolerance 1e-6; ring (lossless all-pass): |T|^2 = 1 but the simulator gives 0.9951 (B-124) so any assertion needs 1e-2; ac_add_drop FD: see B-137 (2e-3).
## Expected
Checks use the helper from B-019 and fail the process.
## Suggested fix
After B-019: add the mzi table and the add-drop FD check; leave ring until B-124 is decided; lambda/ac_crow/mesh need derived expected values (not available).
## Golden impact
none (stdout lines unchanged unless a check is printed).
## Related
B-124, B-019, B-137.

- Discovery seeds: S2.29 (see `backlog/README.md` for seed numbering).
- Review ids: F-A5-20.
## Verification
Read the monitors; mzi values from the output of `specs -t mzi`; add-drop numbers from B-137. PLAUSIBLE because the mzi table was not turned into code.
