# Meson migration — parity validation (PLAN.md §5 steps 2 and 6)

**Result: all checks pass.** The acceptance gate (the golden check) passes for every configuration.
The GCC builds are **bit-identical** to the CMake baseline: shipped defaults, `-Db_ndebug=true`, and the
SystemC-from-submodule fallback.

Final run (step 6) on the final tree, `ws/meson` at d5d46ea plus this file and DECISIONS.md, with fresh
build dirs `/tmp/ws-meson-final-*`. Script: `/tmp/ws-meson-validate.sh` (the commands are reproduced
below). Run on 2026-10-20 on the sprint host:
- i5-6600K, 4 cores, under heavy load from the other workstreams (load average about 12);
- GCC 11.5, clang 19.1.7, Meson 1.11.2, Ninja 1.10.2, CMake 3.26.5, flex 2.6.4, bison 3.7.4;
- Doxygen 1.9.1, Python 3.9.21;
- SystemC 3.0.1 (C++20) + args from `~/.local/opt/specs-deps`.

Reference build for the comparisons: CMake Release of the same sources at `sprint/2026-10`, built in
`/tmp/ws-meson-cmake`:
`cmake -S . -B /tmp/ws-meson-cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$DEPS -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && ninja -C /tmp/ws-meson-cmake specs spx`.

```sh
DEPS=$HOME/.local/opt/specs-deps
GOLDEN="python3 -I tests/golden/run_golden.py check --baseline tests/golden/baseline -j 4"
```

## Source edits outside the build system

Only one, the minimal fix PLAN.md §1 allows: commit 9493407 (dropped when rebasing onto upstream dev: superseded by upstream b7d6fff) `build: make unit_tests compile (minimal patch)`.
It applies exactly `discover/meson/unit-tests-minimal.patch` to `src/test/utils.h`:
- removes the `static_assert(false, …)`;
- `EXPECT_ANGLE_EQ` → `EXPECT_PRED2`.

Behaviour of the product (`specs`, `libspx.so`) is unchanged. For backlog cross-referencing: this is the
`src/test/utils.h:81` seed in `discover/04-build-test-ci.md` §11. `Waveguide.OperatingPoint` still fails
(unfinished angle check) and is registered as the `should_fail` test `unit-known-failure`.

## a. Clean builds (GCC and clang)

| build dir | setup | result |
|---|---|---|
| `final-gcc` | `meson setup … -Dcmake_prefix_path=$DEPS` (shipped defaults) | OK, 0 errors |
| `final-clang` | `CXX=clang++ meson setup … -Dcmake_prefix_path=$DEPS` | OK, 0 errors |
| `final-ndebug` | `… -Db_ndebug=true` (the baseline's configuration) | OK, 0 errors |
| `final-portable` | `… -Dnative=false` (CI configuration) | OK, 0 errors |

GCC warnings are the same classes as under CMake (discover/04 §7):
- 8 `-Wunused-variable`;
- 8 `-Wstrict-aliasing` (4 sites, compiled twice: `specs` and `spx`);
- 4 bison `-Wother`;
- 2 `-Wmaybe-uninitialized`;
- 1 `-Wunused-function` (generated scanner).

Clang warnings:
- 433 `-Wdeprecated-declarations` (`SC_HAS_PROCESS`; more than discover/04's 344 because `spx` and
  `unit_tests` are now compiled too);
- the same `-Wunused-*`;
- `-Wnon-c-typedef-for-linkage` and `-Wmisleading-indentation`, as in discover/04.

## b. Per-TU compile flags vs CMake

`python3 -I discover/meson/flagdiff.py /tmp/ws-meson-cmake/compile_commands.json /tmp/ws-meson-final-gcc/compile_commands.json`

The script matches TUs by (product, source) and compares flag sets after normalising paths. Every TU exists
in both builds: 45 in `specs` (31 core + main, 14 tb), 6 in the parser, 30 in `spx`, 2 in `unit_tests`.

All of the following are identical in both builds:
- `-O3 -std=gnu++20 -Wall -Wextra -march=native -Wfatal-errors -iquote <repo>/src`;
- `-isystem <deps>/include`;
- `-DBUILD_TB=1` exactly on the `specs` core + tb TUs (not on the parser and not on `spx`);
- `-I <generated parser dir>` on the `specs` core and parser TUs;
- `-fPIC` exactly on `spx`;
- `-pedantic` exactly on `unit_tests`.

The remaining differences:

| flag | where | why |
|---|---|---|
| `-DNDEBUG` only in CMake | all | intended: `b_ndebug=false` default (DECISIONS #6). With `-Db_ndebug=true` (`final-ndebug`) both builds have `-DNDEBUG` and neither has `_GLIBCXX_ASSERTIONS` |
| `-D_GLIBCXX_ASSERTIONS=1` only in Meson | all | Meson adds it when `b_ndebug=false` (DECISIONS #6); absent with `-Db_ndebug=true` |
| `-D_FILE_OFFSET_BITS=64` only in Meson | all | Meson default on Linux; nothing in `src/` uses `off_t`; outputs bit-identical |
| `-I <BUILD>/<target>.p` | all | Meson's private dir per target (empty of headers we include) |
| `-I <GEN>` on the 14 tb TUs | `specs` tb | tb objects are part of `specs` now (DECISIONS #11); no tb file includes a generated header |
| `-Dspx_EXPORTS` only in CMake | `spx` | CMake's automatic export macro; unused in `src/` |
| `-Winvalid-pch -fdiagnostics-color=always` | all | Meson defaults; no codegen effect |
| gtest include paths, `-pthread` | `unit_tests` | googletest 1.17.0 wrap instead of the FetchContent 1.12.0 (DECISIONS #19) |

Link (`build.ninja`): `specs` links `libsystemc.so.3.0.1` and `-lm` with the same build rpath
(`<deps>/lib64`). Meson adds `-Wl,--as-needed -Wl,--no-undefined -Wl,-O1`. The object order differs:
core, then tb, then the parser archive (whole-archive), against CMake's tb, parser, core (DECISIONS #14).
The outputs below are nevertheless bit-identical, and the pointer order is unchanged
(`--strict-order` passes).

## c. Golden check: acceptance gate

All runs pass all **45 cases** (44 `specs` cases + `gtest/unit_tests`, which runs with `--unit-tests`),
with nothing skipped or missing:

| build | `--atol 0` | `--rtol 0 --atol 0 --print-slack 0` (bit-exact) |
|---|---|---|
| gcc, shipped defaults | **45 passed**, 0 notes | **45 passed** (`--strict-order` too) |
| gcc, `-Db_ndebug=true` | **45 passed**, 0 notes | **45 passed** (`--strict-order` too) |
| gcc, SystemC fallback (e.) | — | **45 passed** (`--strict-order` too) |
| gcc, `-Dnative=false` | **45 passed** | not bit-exact: passes `--rtol 1e-12`, 1 case fails at `1e-13` (no FMA contraction without `-march=native`) |
| clang 19 | **45 passed**, 4 notes | not bit-exact (9 cases differ, as clang did under CMake) |

The 4 clang notes are reorder-only stdout differences in `cir/subckt_tests`, `opt/vci`, `tb/det` and
`tb/detector`: the known pointer-ordered `std::set` iteration (tests/golden/README.md).

gtest per test, in every build: `A.B` PASS, `Waveguide.Instantiation` PASS, `Waveguide.OperatingPoint`
FAIL, identical to the baseline. That holds with the wrap's googletest 1.17.0 and SystemC's `main` →
`sc_main` entry point (DECISIONS #20b).

Example: `$GOLDEN --specs /tmp/ws-meson-final-gcc/specs --unit-tests /tmp/ws-meson-final-gcc/src/test/unit_tests --atol 0 --rtol 0 --print-slack 0 --strict-order`

## d. `meson test`, all suites

`meson test -C <builddir> --print-errorlogs` gives **Ok: 10, Expected Fail: 1, Fail: 0** for gcc, clang,
ndebug, portable and fallback:
- `unit`: gtest protocol, `-Waveguide.OperatingPoint`;
- `unit-known-failure`: EXPECTEDFAIL;
- 8 `tb` smoke tests;
- `golden`.

## e. SystemC fallback (no `cmake_prefix_path`)

`meson setup /tmp/ws-meson-final-fallback && meson compile -C /tmp/ws-meson-final-fallback` takes 286 s
under load. Meson reports `systemc` not found (pkg-config) and `systemclanguage` not found (CMake), then
`Building SystemC from the thirdparty/systemc submodule (CMake subproject)`. It builds SystemC 3.0.1 as
C++20 (shared, QuickThreads) and links it. `specs` has rpath `$ORIGIN/thirdparty/systemc`.
`meson test`: green; golden: bit-exact (see c).

The first attempts uncovered three problems, all fixed in 6ab418e (DECISIONS #16b, #17):
- the CMake *user package registry* had found a stale SystemC build tree;
- a static SystemC could not be linked into `libspx.so`;
- the cmake translation put `-xassembler-with-cpp` on `qt.c`, and `--no-undefined` broke on `sc_main`.

## f. Docs and install

- `ninja -C /tmp/ws-meson-final-gcc doc`: exit 0. It produces 459 HTML files in `build/doc/html`, plus
  `resources/` (cheatsheet.pdf, delayedwriter.svg, gdrsoc_poster.pdf, signals.svg). The log has 80 lines
  matching `warning`; discover/04 counted 78 doxygen warnings for `make doc` (stale `QUICKSTART.md`,
  obsolete tags; discover/04 §11). The Doxyfile is unchanged.
- `meson install -C /tmp/ws-meson-final-gcc --destdir /tmp/ws-meson-dest` installs `bin/specs` only (the
  default `-Dspx=false`). After `meson configure -Dspx=true` it also installs `lib64/libspx.so`. The
  installed `specs` has `RPATH <deps>/lib64` and runs (`specs -t wg`, from a dir with `traces/`).
- `meson compile -C build spx` builds `libspx.so` with the default options.
- Fallback install: `lib64/libsystemc.so.3.0.1` (+ symlinks), `bin/specs` with `RPATH /usr/local/lib64`.

## g. Extra checks

- **Option combinations.** `-Dold_tb=false -Dtests=disabled -Ddocs=disabled -Dgolden=false` builds; no
  tests or `doc` target are registered, and `specs -p` prints `# No test available #`. With
  `-Dtests=disabled`, `libspx` is not in `all` unless `-Dspx=true`. The reviewer subagent also built the
  first combination and the fallback.
- **Minimum Meson version.** Meson **1.3.2** (pip venv; Ubuntu 24.04's version) configures the tree
  without warnings.
- **Coverage.** `-Db_coverage=true -Dbuildtype=debug`, then `meson test --suite unit --suite tb` and
  `ninja coverage-html`. This produces `meson-logs/coveragereport/index.html` with lcov + genhtml
  (24.2 % lines, 32.3 % functions).
- **Timing.** The workload is `add_drop.cir` with `.assign dlambda = 0.005e-9` (a 6,000-point DC sweep,
  4.9 MB VCD), 9 interleaved runs per build. Median user CPU: CMake Release **0.32 s**, Meson default
  **0.32 s**, Meson `-Db_ndebug=true` **0.32 s**. Wall time is 0.6–0.9 s, dominated by VCD I/O and host
  load. The three VCDs (minus `$date`) are byte-identical.
- **CI steps, run locally**, twice:
  - in a clean `git clone` of the branch: `/tmp/ws-meson-ciclone` at 713f1a2, and `/tmp/ws-meson-ciclone2`
    at d5d46ea (the final commit) with the SystemC "cache" restored;
  - the GitHub gcc-leg shell steps verbatim: `utils/build-systemc.sh` (a real SystemC build from the
    submodule on the first run; a stamp-file no-op on cache hit, including `c++` vs `g++`),
    `meson setup build -Dnative=false [-Dspx=true] -Dcmake_prefix_path=$SYSTEMC_PREFIX`, `meson compile`,
    `meson test --print-errorlogs` (green), `ninja -C build doc`;
  - `./install.sh` (with `BUILD_DIR=build-install`) runs end to end and leaves `git status` clean;
  - no new entries in `~/.cmake/packages` (the first dry run had added one, before the
    `CMAKE_EXPORT_NO_PACKAGE_REGISTRY` fix; I removed it).
- **YAML.** `.github/workflows/ci.yml` and `.gitlab-ci.yml` parse with PyYAML 6.0.2.

## Not verified

- **CI on real runners.** There was no push (sprint rule), so the Ubuntu 24.04 GCC 13 / clang 18
  toolchains, actions/cache and GitLab cache/artefact semantics were not exercised. Golden results on
  those compilers are only expected (clang 19 and `-Dnative=false` pass here), not proven.
- **The `sanitizers` job** (ASan + UBSan, non-gating) was not run locally.
- **Builds on non-Linux or non-x86_64 hosts**, in particular the fallback's `-xnone` QuickThreads
  workaround (DECISIONS #17).
