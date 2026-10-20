# Changelog

## [unreleased]

### New

* Switched to the **Meson** build system; it replaces both CMake and the `Makefile`
  * `meson setup build && meson compile -C build` (release build by default; CMake built without
    optimisation unless a build type was given). Options: `native`, `old_tb`, `spx`, `tests`, `golden`, `docs`
  * SystemC is found via pkg-config, its CMake package (`-Dcmake_prefix_path=...`), or built from the
    `thirdparty/systemc` submodule; googletest comes from a WrapDB wrap (no more configure-time git clone)
  * `meson test` runs the unit tests, the testbench smoke tests (former `make test`) and the golden-output
    regression check
  * `make lib` → `meson compile -C build spx` (`libspx.so`, as built by CMake; the Makefile's `libspecs.so`
    is gone); `make doc` → `ninja -C build doc` (output in `build/doc/html`); `make compiledb` → automatic
    `build/compile_commands.json`; `make waves/newwaves/cleantraces/cleanoldtraces/todos` → run targets
    of the same names; coverage → `-Db_coverage=true` and `ninja coverage-html`
  * Removed: `make format` (most files don't follow `.clang-format`; don't run `ninja clang-format`
    either), `make rc` (its netlist didn't exist), `config.mk`, `.config.gitlab-ci.mk`
  * `meson install` installs `specs` (and `libspx.so` with `-Dspx=true`)
  * Asserts stay enabled by default (`b_ndebug=false`); `-Db_ndebug=true` matches the old CMake Release
* CI: GitHub Actions and GitLab CI on Ubuntu 24.04 with Meson (gcc and clang, golden check);
  `install.sh` rewritten for Meson
* The unit tests compile again (`src/test/utils.h` placeholder `static_assert` removed)
* Golden-output regression harness (`tests/golden/`, `meson test --suite golden`): 80 `specs` cases
  (example netlists, CLI options, every testbench, bad-input errors, regressions) plus the gtest suite
* Paper reference (ECOC 2023, hal-04302200) in the README and `doc/papers/`

### Fixed

Details and repros in `backlog/` (one commit per item, see `backlog/FIXES-REPORT.md`).

* Crashes: bidirectional active MZI elaboration (B-031), `specs -t mesh` segfault (B-047), `-o ""` (B-006),
  `X1` without a subcircuit name (B-149), `--vci` on CROW (B-036), output-port error paths (B-020)
* Bad input now gives a clean error instead of an assert/core dump: positional-argument and config checks
  (B-148, B-018), bare `.dc` (B-008), bad `.options` values (B-153), out-of-range coupler `k`/`kp`/`t` and
  splitter ratio (B-034), stray `.op`/`.dc` arguments (B-165), `.ic` (B-161), unwritable `--json` path (B-017);
  file-open errors name the file (B-064)
* Parser: `.include` inside `.subckt` (B-150), files opened by `.include` are closed (B-156), memory leaks (B-170)
* `--nrings_crow` sets the number of CROW rings (B-131); CROW children are prefixed with the instance name (B-072)
* Detector readouts start at 0 instead of uninitialised memory (B-054); BitstreamSource saturation message (B-044)
* JSON export writes the writers count, not the readers count (B-154)
* The optical field is labelled √W, not V/m (B-023)
* `circuits/subckt_test.cir` parses again (B-132)
* pyspecs: works with pandas 3 and detects `.dc` sweeps (B-076, B-077); default output name, directory joins,
  error reporting, dependency manifest, notebook binary path, value shortening, bitstream helpers
  (B-085..B-089, B-091, B-092, B-099)
* Docs: Doxygen config/mainpage and stale `@param`/`@sa` references (B-083, B-100, B-102); KiCad symbol
  templates (B-082); `.gitignore` no longer hides new `circuits/*.cir` (B-114); flex/bison warnings (B-109, B-177)
* gtest fork harness: pipe deadlock above 64 KiB, escaping assertions/exceptions (B-125); angle check (B-123)

### Removed

* Dead code (see `backlog/cleanup/REPORT.md`): `do_circuit`/`raphael_main`, the unused `Ring`,
  `PowerProbe` and `PhaseProbe` devices, dead output-port and parser code, the duplicate
  `utils/draw_graph_from_json.py`, leftover debug prints on stdout

## v0.1.0

* Initial release