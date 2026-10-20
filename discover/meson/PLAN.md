# Meson migration — evaluation & implementation plan

Status: **approved** by the maintainer (2026-10-20): Meson *replaces* both CMake and the legacy Makefile; CI is
ported to **both** GitLab CI and GitHub Actions. Implementer: paseo workspace `ws-meson` (Opus 5.5), branch `ws/meson`.
Inputs: `discover/04-build-test-ci.md` (esp. §9 "Constraints for a meson migration"), `tests/golden/README.md`.

## 1. Evaluation

| Criterion | Today (CMake + Makefile) | Meson |
|---|---|---|
| Single source of truth | two build systems drifting apart (C++17 vs C++20, different source sets, `-O0` default) | one |
| Default build | CMake: no build type → **unoptimised** `install.sh` builds (paper speed claims irreproducible) | `default_options: buildtype=release` |
| flex/bison | `FindBISON/FindFLEX` (CMake), hand rules (Make) | `custom_target` with explicit outputs, straightforward |
| Source lists | `GLOB_RECURSE` (fragile, silently picks up stray files) | explicit lists (reviewable; a test can check for unlisted `.cpp`) |
| Tests | `gtest_discover_tests` + ad-hoc `make test` | `meson test`: gtest protocol, suites, timeouts, golden + tb smoke tests as first-class tests |
| Coverage / sanitizers | custom `coverage.cmake` | built-in `-Db_coverage=true` + `ninja coverage`, `-Db_sanitize=address,undefined` |
| Deps | `find_package(... PATHS thirdparty/*/build/install)`; gtest via FetchContent at configure | `dependency()` chain: pkg-config → CMake package → **CMake subproject fallback from the submodule**; gtest via WrapDB wrap |
| compile_commands.json | Makefile needs `compiledb` | always generated |

**Risks and mitigations**
- *SystemC ships no `.pc` when built with CMake.* Use `dependency('SystemCLanguage', method: 'cmake',
  modules: ['SystemC::systemc'])`, honouring `-Dcmake_prefix_path=…`. Fallback: `cmake.subproject()` on
  `thirdparty/systemc` with `CMAKE_CXX_STANDARD=20`. Also accept a pkg-config `systemc` (distro/autotools installs).
- *C++ standard/ABI mismatch with SystemC* (SystemC encodes the standard in a link-time API symbol): pin the
  same standard everywhere and document it.
- *Numerical drift between build systems.* CMake's `CMAKE_CXX_STANDARD 20` with extensions ON compiles with
  **`-std=gnu++20`**, which on GCC implies `-ffp-contract=fast` (FMA contraction under `-march=native`), while ISO
  `-std=c++20` disables contraction. Event gating (abstol/reltol) can amplify last-bit differences into different
  event counts. ⇒ Use `cpp_std=gnu++20` and keep `-march=native` (as an option, default on) so the Meson build is
  bit-compatible with the CMake baseline. Verify by golden check **and** by diffing per-TU flags from both
  `compile_commands.json` files.
- *`NDEBUG`.* CMake Release adds `-DNDEBUG`; the baseline was recorded with it (`-O3 -DNDEBUG -march=native`,
  gnu++20). Probe 4 verified the same baseline also passes with a CMake Debug build (asserts on, `-O0`) and with clang.
  ⇒ Ship Meson's default `b_ndebug=false` (asserts on: the code validates user input with `assert`) and document
  why in DECISIONS.md.
- *Unit tests don't compile in-tree* (`src/test/utils.h:81` `static_assert(false)`, plus a broken `EXPECT_ANGLE_EQ`).
  The baseline's gtest results were recorded with the minimal patch in `discover/meson/unit-tests-minimal.patch`
  (expected: `A.B` PASS, `Waveguide.Instantiation` PASS, `Waveguide.OperatingPoint` FAIL). Apply exactly that patch
  as its own `build:` commit so `unit_tests` builds. Register `Waveguide.OperatingPoint` as a known failure (gtest
  filter or `should_fail` split) with a comment pointing to the backlog, so `meson test` is green.
- *Lost features.* Every Makefile/CMake target must be accounted for (§3, checklist).

**Verdict: switch.** About 300 lines of Meson plus CI replace about 450 lines across two systems, fix the
unoptimised default, make tests first-class, and remove configure-time network access for normal builds.

## 2. Target layout

```
meson.build                # project(), options, deps, top-level targets, tests, run_targets
meson.options              # (meson ≥1.1 name; or meson_options.txt) options below
src/meson.build            # core sources list (files()), subdir() calls
src/parser/meson.build     # flex/bison custom_targets + parser sources
src/devices/meson.build    # device sources list
src/tb/meson.build         # alltestbenches.cpp (+ *_tb.cpp when old_tb)
src/test/meson.build       # unit_tests
tests/golden/meson.build   # golden regression test(s)
thirdparty/gtest.wrap      # WrapDB googletest wrap (if subproject_dir = 'thirdparty'), else subprojects/gtest.wrap
```
- `project('specs', 'cpp', version: '0.2.0-dev', license: 'LGPL-3.0-or-later', meson_version: '>=1.3.0',
  default_options: ['cpp_std=gnu++20', 'buildtype=release', 'warning_level=2', 'b_ndebug=false'])`.
  Consider `subproject_dir: 'thirdparty'` so the existing submodules double as subprojects. Choose and justify.
- Global args: `-Wfatal-errors`, `-iquote <src>` (via `meson.project_source_root() / 'src'`), and `-march=native` when
  `get_option('native')`. Keep `-Wall -Wextra` (= `warning_level=2`).
- **Options**: `native` (bool, true), `old_tb` (bool, true; defines `BUILD_TB=1` for main.cpp + tb sources and
  adds `*_tb.cpp`), `tests` (feature, auto: gtest unit tests), `golden` (bool, true: register golden tests),
  `docs` (feature, auto: doxygen run_target).
- **Targets** (parity with CMake):
  - `specs` executable: core + devices + parser (generated sources) + tb.
  - `spx` shared library (`libspx.so`): core + devices without `main.cpp`, parser and tb. Mirror the CMake
    contents (the Makefile's `libspecs.so` differed). Not built by default (`build_by_default: false`), like
    CMake's EXCLUDE_FROM_ALL. Install it when built.
  - `unit_tests` executable: gtest_main + spx + the same compile args. Register with `test(..., protocol: 'gtest')`.
- **Parser generation**: `flex --header-file=@OUTPUT1@ -o @OUTPUT0@ @INPUT@` → `scanner.cpp/.h`;
  `bison --warnings -Wall --defines=@OUTPUT1@ -o @OUTPUT0@ @INPUT@` → `parser.cpp/.h` (mirror the existing
  flags exactly). Generated headers must be reachable as `"parser.h"`/`"scanner.h"` from `src/parser/*.cpp` and
  `src/main.cpp`. Order dependencies are needed: the scanner includes `parser.h`.
- **Dependencies**: `args` is header-only: `declare_dependency(include_directories: include_directories('thirdparty/args', is_system: true))`
  (or the CMake package if found). `m` via `cc.find_library('m')`. `threads` if SystemC needs it.

## 3. Feature parity checklist (CMake + Makefile → Meson)
| Old | New |
|---|---|
| `make` / `cmake --build` (bin) | `meson compile -C build` |
| `make lib` / `spx` target | `meson compile -C build spx` |
| `BUILD_TB` / `BUILD_OLD_TB` | `-Dtests=` / `-Dold_tb=` |
| `make test` (tb smoke list) | `meson test --suite tb` (one `test()` per tb name: `specs -t <name>`; keep the Makefile's disabled ones disabled, with a comment) |
| gtest (`gtest_discover_tests`) | `meson test --suite unit` |
| golden harness | `meson test --suite golden` (`python3 -I tests/golden/run_golden.py check --specs <specs> [--unit-tests <unit_tests>] --baseline …`) |
| `make doc` | `ninja -C build doc` (`run_target`, copies `doc/resources`, runs doxygen with the existing Doxyfile; fix paths if the Doxyfile assumes the repo root as cwd) |
| coverage.cmake | `-Db_coverage=true` + `ninja coverage` (document gcovr/lcov requirement) |
| `make compiledb` | automatic `build/compile_commands.json` |
| `make waves/newwaves/cleantraces/cleanoldtraces/todos` | `run_target`s that call small scripts in `utils/` (port the logic; keep behaviour) or document their removal in CHANGELOG with a reason |
| `make format` | **drop** (dangerous: 104/108 files non-compliant). Meson's automatic `clang-format` target exists; document "don't run it" in CLAUDE.md |
| `make rc` | drop (`circuits/rc_mohab.cir` does not exist) |
| `install.sh` | rewrite: submodules → build SystemC (CMake, C++20) into a prefix → `meson setup build -Dcmake_prefix_path=…` → `meson compile` → `meson test` |
| `config.mk`, `.config.gitlab-ci.mk`, `config.local.mk` | removed; replaced by Meson options / native files |

## 4. CI
- **GitHub Actions** `.github/workflows/ci.yml`: `ubuntu-latest` (pin a version), `submodules: recursive`; install
  `ninja-build flex bison doxygen` from apt and `meson` from pip (recent); cache the SystemC install keyed on the
  submodule SHA plus compiler; build SystemC with CMake if the cache misses; `meson setup -Dnative=false`;
  compile; `meson test --print-errorlogs`; upload test logs on failure. Matrix: gcc, plus clang if it builds cleanly.
  Optional job: `buildtype=debug -Db_sanitize=address,undefined` with `continue-on-error: true` until the backlog
  is fixed.
- **GitLab CI** `.gitlab-ci.yml`: same steps. Choose an image with the toolchain (e.g. `debian:bookworm` or
  `ubuntu:24.04` + apt; Alpine's `systemc-dev` may not match C++20, so build from the submodule with cache); stages
  build → test; artefacts `build/specs`, `build/libspx.so` (if built), test logs.
- No pushing, so CI can't be run for real. Validate YAML syntax locally (`python3 -c 'import yaml'` if available)
  and **run the exact CI shell steps locally** in a clean clone / clean build dir to prove them.

## 5. Implementation steps (one logical commit each, on `ws/meson`)
1. Add Meson files alongside CMake. Build with `meson setup /tmp/ws-meson-build -Dcmake_prefix_path=$HOME/.local/opt/specs-deps`.
2. Parity validation (all must pass, results recorded in `discover/meson/VALIDATION.md`):
   a. `meson compile` clean build with gcc and with clang (`CXX=clang++`);
   b. `compile_commands.json` flag diff vs a CMake build of the same tree: identical flags per TU except
      ordering/paths/intended changes (list them);
   c. `tests/golden/run_golden.py check --atol 0` against `tests/golden/baseline` passes with the Meson `specs`
      **and** `--unit-tests` (all 45 cases, nothing skipped), both for the shipped defaults and for
      `-Db_ndebug=true` (the baseline's configuration);
   d. `meson test` all suites green (or known-failing tests marked `should_fail`/skipped with a backlog note);
   e. SystemC fallback: configure **without** `cmake_prefix_path` in a fresh build dir → the cmake-subproject
      fallback builds SystemC from `thirdparty/systemc` and links (slow; run once);
   f. `ninja doc` works; `meson install --destdir /tmp/ws-meson-dest` installs `specs` (+ `libspx.so` when built).
3. Port CI (both files) + new `install.sh`.
4. Remove CMake (`CMakeLists.txt`, `src/**/CMakeLists.txt`, `cmake/`), the Makefile, `config.mk`,
   `.config.gitlab-ci.mk`; update `.gitignore` (build dirs, `thirdparty/packagecache`/`subprojects/packagecache`, wrap
   download dirs).
5. Docs: README "Building" section, `CLAUDE.md` build/test commands, `discover/ONBOARDING.md` "Common tasks",
   `tests/golden/README.md` commands, CHANGELOG entry under `[unreleased]`.
6. Re-run the whole step-2 validation on the final tree.

## 6. Rules
- Do not change product behaviour. The only allowed source edits are ones strictly required to build under Meson
  (e.g. an include path). If something in the code is broken in a way that blocks the build (e.g. the
  `static_assert(false)` in `src/test/utils.h` if it really fails to compile), make the **minimal** fix in its own
  commit `build: …` and mention it in VALIDATION.md so the review backlog can cross-reference it.
- Prebuilt deps live in `~/.local/opt/specs-deps`. Do not rebuild them there or modify that prefix.
  Scratch builds go in `/tmp/ws-meson-*`.
- Never `git push`.
