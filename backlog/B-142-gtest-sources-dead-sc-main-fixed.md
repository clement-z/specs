# B-142: gtest sources: dead `sc_main`, fixed /tmp trace path, duplicate/dead code, FLOAT_EQ on doubles, non-statement macros
severity: low | category: style/maintainability | effort: S | fix-class: easy-unattended | verdict: CONFIRMED
location: src/test/main.cpp:9-15; src/test/devices/waveguide.cpp:15-78,122,130-131,142,167; src/test/utils.h:13-19,79
## Problem
- `sc_main` (main.cpp:9-15) is never called: `GTest::gtest_main` supplies `main` ("Running main() from .../gtest_main.cc" is printed by the binary).
- Both tests use `trace_filename = "/tmp/specs_test2"` (waveguide.cpp:122,142): fixed shared path (the run shows `tracing ... (/tmp/specs_test2.vcd)`), so concurrent runs by different users collide with E701.
- waveguide.cpp:15-78 is a 64-line `#if 0` copy of obsolete macros (references undefined `i_am_parent`, `SpecsTest`); :130-131 checks `a_out` twice and never `b_out`; :167 leaves a debug `std::cout << wl`; `<sys/wait.h>` is unused.
- `EXPECT_CPLX_EQ`/`ASSERT_CPLX_EQ` (utils.h:13-19) use `EXPECT_FLOAT_EQ` on doubles (float precision, 4 float ulps) and expand to two unbraced statements with unparenthesised arguments (breaks in `if/else`).
- main.cpp:18-24 `TEST(A, B)` is an empty placeholder (keep it unless the baseline is re-recorded: `compare_gtest` fails on a missing test).
## Evidence
`unit_tests` output (patched copy) shows `tracing timescale unit set: 1 fs (/tmp/specs_test2.vcd)` and "Running main() from ...gtest_main.cc".
## Expected
No dead code; per-process temp trace files; double-precision macros usable as statements.
## Suggested fix
- delete main.cpp:9-15;
- trace file: `specsGlobalConfig.trace_filename = testing::TempDir() + "specs_test_" + std::to_string(getpid());` (used in `backlog/repro/a5-test-devices.cpp`);
- delete waveguide.cpp:15-78, change :131 to `tb.b_out`, delete :167 and the `<sys/wait.h>` include;
- apply `backlog/repro/a5-test-utils-macros.patch` (DOUBLE_EQ in `do { } while (0)` with parenthesised arguments; applies after the angle and harness patches).
## Golden impact
none (test names and results unchanged; `A.B` kept).
## Related
B-123, B-125.

- Discovery seeds: S4.20 (see `backlog/README.md` for seed numbering).
- Review ids: F-A5-22.
## Verification
Built the macros patch together with the other two and ran `Waveguide.*`: PASS. The b_out check passes at instantiation (default value is zero).
