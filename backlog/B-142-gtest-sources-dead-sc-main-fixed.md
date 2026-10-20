# B-142: gtest sources: dead `sc_main`, fixed /tmp trace path, duplicate/dead code, FLOAT_EQ on doubles, non-statement macros
severity: low | category: style/maintainability | effort: S | fix-class: easy-unattended | verdict: CONFIRMED | status: deferred to ws-cleanup (style); B-123/B-125 rewrote the utils.h macros, recheck what remains
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

Correction from the second check: **do not delete `sc_main`** from `src/test/main.cpp`: `libsystemc.so` references it and the link fails. Replace the body with a stub, e.g. `int sc_main(int, char*[]) { return 0; } // required by libsystemc, never called: GTest::gtest_main supplies main()`. Note that per-PID trace files (`specs_test_<pid>.vcd`) accumulate in `testing::TempDir()`. Depends on B-123.
## Golden impact
none (test names and results unchanged; `A.B` kept).
## Related
B-123, B-125.

- Discovery seeds: S4.20 (see `backlog/README.md` for seed numbering).
- Review ids: F-A5-22.
## Verification
Built the macros patch together with the other two and ran `Waveguide.*`: PASS. The b_out check passes at instantiation (default value is zero).

**Independent second check (V6, of F-A5-22): ADJUST.** Method: applied all bullets on the scratch copy incl. the macros patch, rebuilt unit_tests, ran it and the golden check

Bullet 1 is WRONG and breaks the build: deleting `sc_main` from src/test/main.cpp gives `ld: libsystemc.so.3.0.1: undefined reference to 'sc_main'` (libsystemc.so defines `main` and needs `sc_main`; `nm -D` shows `T main` / `U sc_main`). The function is never called (the binary prints "Running main() from .../gtest_main.cc"), but a definition is required: replace the body with a stub, e.g. `int sc_main(int, char*[]) { return 0; }` with a comment "required by libsystemc, never called: GTest::gtest_main supplies main()" (verified: links, 9/9 tests pass). The other bullets work and keep the result set: delete waveguide.cpp `#if 0` block (lines 15-78) and `<sys/wait.h>`, `tb.b_out` at :131 (passes), drop the debug cout, trace_filename = testing::TempDir() + "specs_test_" + std::to_string(getpid()) (getpid comes via utils.h <unistd.h>), macros patch applies cleanly after angle+harness patches and compiles (also with `std::complex(0,0)` args and in unbraced if/else, -Wall -Wextra -pedantic clean). Side effect to mention: one `specs_test_<pid>.vcd` per test per run accumulates in TempDir() (previously one shared file). Golden: unchanged apart from the B-123 gtest notes; A.B kept.

**Orchestrator:** sc_main must be stubbed, not deleted (link failure).
