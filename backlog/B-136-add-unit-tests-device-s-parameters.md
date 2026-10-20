# B-136: add unit tests for device S-parameters (waveguide loss/phase/delay, coupler, splitter, merger)
severity: low | category: test-gap | effort: S | fix-class: easy-unattended | verdict: CONFIRMED
location: new file src/test/devices/models.cpp (source: backlog/repro/a5-test-devices.cpp), src/test/CMakeLists.txt:7-11
## Problem
The only device test (`src/test/devices/waveguide.cpp`) checks the phase of a zero-slope waveguide only. Loss, group delay, coupler S-matrix and unitarity, splitter and merger are covered only by the testbenches (which cannot fail the process, see B-019).
## Evidence
`backlog/repro/a5-test-devices.cpp` (6 tests, OP analysis through the same `SPECS_TEST_HEADER()` pattern) was built and run in the scratch copy with the B-123/05/22 header patches: 6/6 PASS, whole binary 11 PASS (+2 deliberate probe failures from B-125). Expected values:
- `WaveguideModel.LossAndPhaseZeroSlope`: WaveguideBi 1 cm, neff=ng=1, 3.0103 dB/cm, 1 V/m at 1550 nm -> p1_out power 0.5 +-1e-4, phase = 2 pi * 1e-2 / 1550e-9 mod 2 pi (-2.4322 rad) within 1e-9, nothing at p0_out (1e-12).
- `WaveguideModel.GroupDelay`: neff 2.0, ng 4.3, 1 cm, lossless, event-driven: output still 0 after `sc_start(143, SC_PS)` and 1 W after one more ps (L*ng/c = 143.43 ps).
- `DirectionalCouplerModel.Smatrix`: DirectionalCouplerBi through power 0.1, loss 3.0103 dB, 1 V/m on p0 -> p2 power 0.05 (phase 0), p3 power 0.45 (phase pi/2), p0/p1 outputs 0.
- `DirectionalCouplerModel.LosslessUnitarity`: through power 0.3, loss 0, inputs (0.6+0.2i) and (-0.1+0.5i) -> sum of output powers equals sum of input powers within 1e-12.
- `SplitterModel.EqualSplitWithLoss`: 3.0103 dB, 3 W in -> 0.75 W on both outputs (1e-4), phase 0.
- `MergerModel.CoherentSum`: attenuation 0: one 1 W input -> 0.5 W; two in-phase 1 W inputs -> 2 W; opposite phase -> 0 W (1e-4).
These numbers are the same ones the legacy testbench self-checks assert (wg 3/3, dc 3/3, splitter 3/3, merger 4/4) and are consistent with the closed forms.
## Expected
Regressions in device S-parameters fail `unit_tests`.
## Suggested fix
Copy the repro to `src/test/devices/models.cpp`, add it to `add_executable(unit_tests ...)` (src/test/CMakeLists.txt:7-11). It uses `EXPECT_ANGLE_EQ(sig, phi)` from B-123 and a per-process trace file under `testing::TempDir()`. Not covered here (need a physics decision, not unambiguous): dispersion (D != 0) and neff != ng phase slope, crossing, PCM, phase shifter.
## Golden impact
gtest case only: 6 new tests reported as non-fatal "new test" notes; re-record with `--unit-tests`.
## Related
B-123, B-125, B-140.

- Discovery seeds: S2.29 (see `backlog/README.md` for seed numbering).
- Review ids: F-A5-16.
## Verification
Built and ran as described.
