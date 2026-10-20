# Changelog

## [unreleased]

### New

* Switched to the **Meson** build system; it replaces both CMake and the `Makefile`
  * `meson setup build && meson compile -C build` (release build by default; CMake built without
    optimisation unless a build type was given). Options: `native`, `old_tb`, `tests`, `golden`, `docs`
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
  * `meson install` installs `specs` (and `libspx.so` when built)
  * Asserts stay enabled by default (`b_ndebug=false`); `-Db_ndebug=true` matches the old CMake Release
* CI: GitHub Actions and GitLab CI on Ubuntu 24.04 with Meson (gcc and clang, golden check);
  `install.sh` rewritten for Meson
* The unit tests compile again (`src/test/utils.h` placeholder `static_assert` removed); the unfinished
  `Waveguide.OperatingPoint` check is a known failure

## v0.1.0

* Initial release