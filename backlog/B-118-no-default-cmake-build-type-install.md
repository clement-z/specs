# B-118: No default CMAKE_BUILD_TYPE: `install.sh` and plain `cmake` give an unoptimised binary
severity: low | category: build | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: obsolete (meson migration, b86a745: Meson defaults to buildtype=release; install.sh uses it)
location: CMakeLists.txt:1-12, install.sh:28
## Problem
`CMAKE_BUILD_TYPE` is empty by default, so no `-O` flag is passed at all; the documented `install.sh` build is therefore unoptimised (S5.01 measured CROW N=3 at 2.3 s for 20 ns). obsoleted-by-meson: yes (`buildtype=release` in `default_options`, PLAN.md:60); fix on the CMake side only if the merge slips. CLAUDE.md already tells users to always pass a build type.
## Evidence
```
$ cmake -S . -B cfg1 -G Ninja -DCMAKE_PREFIX_PATH=<deps> -DBUILD_TB=OFF
CMAKE_BUILD_TYPE:STRING=          CMAKE_CXX_FLAGS:STRING=
c++ -DBUILD_TB=1 -isystem <deps>/include -std=gnu++20 -Wall -Wextra -march=native -Wfatal-errors -iquote .../src ...   (no -O)
```
## Expected
Release by default.
## Suggested fix
After `project(...)` in CMakeLists.txt:
```
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (default: Release)" FORCE)
endif()
```
## Golden impact
none (the baseline was recorded with `-O3 -DNDEBUG`, which is what Release gives).
## Related
B-119, B-108.

- Discovery seeds: S1.20, S4.11, S5.01 (see `backlog/README.md` for seed numbering).
- Review ids: F-A7-15.
## Verification
Applied to a scratch copy: `ninja -t commands specs` shows `-O3 -DNDEBUG`.

**Independent second check (V2, of F-A7-15): CONFIRMED.** Method: applied the suggested block after `project(...)`, configured with no build type, with `-DCMAKE_BUILD_TYPE=Debug`, and with "Ninja Multi-Config"; inspected `ninja -t commands`

Without the fix `CMAKE_BUILD_TYPE:STRING=` and the compile line has no -O flag (reproduced). With the fix the cache gets `Release` and compile commands show `-O3 -DNDEBUG` (same flags the golden baseline was recorded with, and the same as my Release golden build, 44/44 pass). An explicit `-DCMAKE_BUILD_TYPE=Debug` is still honoured (`-g`), Multi-Config generators are unaffected (`CMAKE_CONFIGURATION_TYPES` set, guard skips). Caveats to record, none blocking: (1) Release defines NDEBUG, so `assert`s (e.g. DCAnalysis `assert(sweep_orders.size() > 0)`, `upgrade_signal`) are now compiled out for `install.sh` users, as they already are in the documented golden build; (2) install.sh:17 builds SystemC itself with a bare `cmake ..` (no build type), which this fix does not cover; (3) Release vs RelWithDebInfo as the default is a (small) choice, justified by CLAUDE.md and the golden baseline. obsoleted-by-meson as stated.
