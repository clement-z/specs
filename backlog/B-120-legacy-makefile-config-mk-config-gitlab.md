# B-120: Legacy Makefile, config.mk, .config.gitlab-ci.mk and .gitlab-ci.yml are broken and drifted from CMake
severity: low | category: build | effort: M | fix-class: needs-design | verdict: CONFIRMED | status: obsolete (meson migration, b86a745: Makefile, config.mk, .config.gitlab-ci.mk removed; .gitlab-ci.yml rewritten)
location: Makefile:25-26,65,279-280, config.mk:6,20, .config.gitlab-ci.mk:16, .gitlab-ci.yml:26-42,55-64, CHANGELOG.md:9
## Problem
obsoleted-by-meson: yes (PLAN.md:125 deletes all four files and rewrites the CI for GitLab and GitHub). Do not fix individually; make sure the port covers the points below.
- Makefile:25-26: `SOURCES_BIN`/`SOURCES_LIB` still `find src -name '*.cpp'` without excluding `src/test/`, so `specs` and `libspecs.so` get `src/test/main.cpp` and `devices/waveguide.cpp` (gtest). Verified: `make print-SOURCES_BIN` lists `src/test/devices/waveguide.cpp` and `src/test/main.cpp`, `print-SOURCES_LIB` lists `src/test/devices/waveguide.cpp`.
- config.mk:20 / .config.gitlab-ci.mk:16 use `-std=c++17` against a C++20-built SystemC (undefined `sc_api_version_3_0_1_cxx201703L` at link, per discover/04 §7); config.mk:6 expects `thirdparty/systemc/sc_install` while install.sh produces `build/install`.
- Makefile:65 `ADDITIONAL_DEPS = Makefile $(CONFIG_MK_FILE)` is cwd-relative (breaks out-of-tree `make -C`); `rc` (279-280) runs the non-existent `circuits/rc_mohab.cir`; `.SECONDARY: $(BUILD_PATH)/parser.h)` has a stray `)`; `test` runs `specs -t` whose exit status is always 0.
- CHANGELOG.md:9 still says "Original Makefile should still function for now": false.
- .gitlab-ci.yml: builds with the Makefile on `alpine:latest` + `edge/testing` (unpinned, unknown SystemC/C++ standard), runs only `./specs -t wg`, `doc` expects `doc/pcm-doc.tar.xz` that is never produced (Makefile:212 commented out). YAML itself parses.
## Evidence
See the cited lines; `make print-SOURCES_BIN | tr ' ' '\n' | grep src/test`.
## Expected
One build system, CI that builds C++20 SystemC and runs the golden suite.
## Suggested fix
Delete the four files in the Meson cleanup commit, add a CHANGELOG entry that replaces line 9, and have the new CI install `python3` and keep the Doxygen/pages jobs (they exist today) or document their removal.
## Golden impact
none.
## Related
B-116, B-119.

- Discovery seeds: S4.04, S4.05, S4.23, S5.10, S5.11, S5.20 (see `backlog/README.md` for seed numbering).
- Review ids: F-A7-17.
## Verification
`make print-*` (no files written); code reading of the other lines; seeds S4.05/S5.10 "GitHub remote is dead" not verifiable offline.
