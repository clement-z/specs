# B-118: No default CMAKE_BUILD_TYPE: `install.sh` and plain `cmake` give an unoptimised binary
severity: low | category: build | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
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
