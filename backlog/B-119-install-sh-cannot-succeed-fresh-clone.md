# B-119: `install.sh` cannot succeed on a fresh clone (args never installed, `all` builds the non-compiling unit_tests)
severity: low | category: build | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: obsolete (meson migration, 57714e7: install.sh rewritten for Meson, validated end to end in discover/meson/VALIDATION.md)
location: install.sh:9-29, CMakeLists.txt:15
## Problem
obsoleted-by-meson: yes (PLAN.md:95 rewrites `install.sh`; do not fix separately).
1. install.sh builds only SystemC into `thirdparty/systemc/build/install` but CMakeLists.txt:15 requires an installed `args` CMake package in `thirdparty/args/build/install`. Verified: with a prefix containing SystemC only, configure fails with `Could not find a package configuration file provided by "args"`.
2. Line 28 `cmake .. -DBUILD_TB=1` then `make -j` builds `all`, which includes `unit_tests`; it does not compile (`src/test/utils.h:81 static_assert(false)`), so `set -e` aborts.
3. `make -j` without a number is an unbounded job count (OOM risk with SystemC headers); no build type (B-118); `specs_dir` (line 25) is unused; it must be run from the repo root (`SRC_DIR=$(pwd)`); typo "Dowloading".
## Evidence
```
$ cmake -S <repo> -B cfg2 -DCMAKE_PREFIX_PATH=<prefix with SystemC only> -DBUILD_TB=OFF
CMake Error at CMakeLists.txt:15 (find_package): Could not find a package configuration file provided by "args" ...
```
## Expected
`./install.sh` yields a built, tested `specs`.
## Suggested fix
Replaced wholesale by the Meson `install.sh` (args is header-only: `declare_dependency` on `thirdparty/args`).
## Golden impact
none.
## Related
B-118, B-120, B-121. `src/test/utils.h:81` belongs to the unit-test reviewer.

- Discovery seeds: S5.03 (see `backlog/README.md` for seed numbering).
- Review ids: F-A7-16.
## Verification
Configure runs as above; items 2/3 by code reading and discover/04 §2.
