# B-121: Misc dead or fragile CMake code (misnamed BUILD_TB option, ctest, network gtest, unrequired BISON/FLEX, dead coverage.cmake)
severity: low | category: build | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: CMakeLists.txt:5-6, src/test/CMakeLists.txt:1,5,27, cmake/googletest.cmake:6-16, src/parser/CMakeLists.txt:10-11,34-38, cmake/coverage.cmake, src/tb/CMakeLists.txt:5-35
## Problem
obsoleted-by-meson: yes for all (PLAN.md:85-91,125 replaces them with `-Dtests`/`-Dold_tb`, WrapDB gtest, `meson test`, `-Db_coverage`; `cmake/` is deleted). Listed so the port keeps the behaviour that matters:
- Option `BUILD_TB` (CMakeLists.txt:6) gates the gtest `unit_tests` (src/test/CMakeLists.txt:1) while `BUILD_OLD_TB` gates the testbenches, and the preprocessor macro `BUILD_TB=1` (src/tb/CMakeLists.txt:11, `main.cpp:342`) means "testbenches enabled": three meanings of one name (the Makefile's `BUILD_TB` is the macro).
- `enable_testing()` is only in `src/test`: `ctest` in the build root finds nothing (verified after a full configure: no `CTestTestfile.cmake` in the root, `Total Tests: 0`; tests are visible only from `build/src/test`).
- `BUILD_TB` defaults ON, so a default configure needs network (git clone of googletest) or a system GTest. Verified: with `-DFETCHCONTENT_FULLY_DISCONNECTED=ON` and no system GTest, configure fails with `Target "unit_tests" links to GTest::gtest_main but the target was not found`. `FIND_PACKAGE_ARGS` (googletest.cmake:13) needs CMake >= 3.24 while `cmake_minimum_required` is 3.14 (from the CMake docs; not run on an old CMake).
- `find_package(BISON/FLEX)` is not `REQUIRED`: missing bison -> `Unknown CMake command "bison_target"` (see B-116 item 2).
- `cmake/coverage.cmake` is never `include()`d and its only call is commented out (src/test/CMakeLists.txt:27); as written it also has bugs (`DEPENDS ${TARGET}` undefined, `./${arg_TARGET}` run from the build root where `unit_tests` is not, `if(NOT ${CMAKE_BUILD_TYPE} IN_LIST ...)` breaks with an empty build type) and would instrument only `unit_tests`, not `spx`.
- src/parser/CMakeLists.txt:34-38 leaves a debugging `get_cmake_property` + `message(STATUS "HH_FILES=...")` that prints on every configure; src/tb/CMakeLists.txt lists the sources twice (`add_library(... ${SRC_FILES})` then `target_sources`).
## Evidence
Commands above (cfg4/cfg5 scratch configures).

Additional evidence (independent report F-A5-23, A5 reviewer):

Code reading; `unit_tests` failed to compile in `all` until B-123; the discovery doc (04-build-test-ci.md section 2) lists `-DBUILD_TB=OFF` as "without gtest".
## Expected
n/a (deleted with CMake).
## Suggested fix
No CMake changes; delete with the migration. Keep in Meson: tests off by default or auto (`-Dtests=auto`), no configure-time network for the default build.
## Golden impact
none.
## Related
B-117, B-118, B-119, B-120.

- Discovery seeds: S4.12, S4.19, S5.27, S5.28, S1.04 (see `backlog/README.md` for seed numbering).
- Merged duplicate reports: F-A5-23 (A5).
- Review ids: F-A7-18, F-A5-23.
## Verification
Configures in `/tmp/ws-review-A7/cfg*`; `ctest -N` in root and in `src/test`.
