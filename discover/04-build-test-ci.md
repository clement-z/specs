# 04 — Build, tests & CI (discovery probe 4/5)

Probe scope: CMake / legacy Makefile builds, generated parser, testbenches, gtest harness, coverage, CI,
Doxygen. It also produced the golden-output baseline harness in `tests/golden/`.
Everything was built out of tree under `/tmp/specs-sprint/`, and no tracked file was modified.
Host: i5-6600K (4 cores, AVX2+FMA), GCC 11.5, clang, CMake 3.26.5, Ninja, flex 2.6.4, bison 3.7.4,
Doxygen 1.9.1 (+dot), Python 3.9.21. Deps (SystemC 3.0.1 C++20, taywee/args) come from `~/.local/opt/specs-deps`.

## 1. Summary

* **CMake builds `specs` and `libspx.so` cleanly**, with 17 distinct warnings and no errors, once the deps are
  supplied through `-DCMAKE_PREFIX_PATH`.
  **The default `all` target fails** with `BUILD_TB=ON` (the default): `unit_tests` is part of `all`, and it
  cannot compile because of `static_assert(false, "Work in progress…")` at `src/test/utils.h:81`. A second
  error behind it is masked by `-Wfatal-errors`: `EXPECT_ANGLE_EQ` misuses `EXPECT_PRED_FORMAT2`.
  Build `specs` and `spx` explicitly, or pass `-DBUILD_TB=OFF`.
* **The legacy Makefile is broken as shipped.**
  * It globs `src/test/*.cpp` (gtest) into `specs`.
  * `config.mk` uses `-std=c++17`, which fails to link against a C++20 SystemC (`sc_api_version_3_0_1_cxx201703L`).
  * `ADDITIONAL_DEPS` requires a literal `./Makefile` in the cwd.

  It does work from an out-of-tree cwd: symlink the Makefile, use a `config.local.mk` with `-std=c++20`, and
  override `SOURCES_BIN` / `SOURCES_LIB` on the command line.
* **CI (`.gitlab-ci.yml`) is stale.** It still uses the Makefile on Alpine with `-std=c++17` and runs only
  `./specs -t wg`. It would fail today because of the `src/test` glob.
* **Tests that exist today:**
  * 13 legacy testbench functions under 19 names (`specs -t <name>`). They print self-check results, but
    `specs -t` always exits 0.
  * A 3-test gtest suite with a fork-per-test harness (in-tree it doesn't compile).
  * No netlist regression tests.
* **The golden harness is in place:** `tests/golden/run_golden.py` (stdlib only), 44 `specs` cases + 1 gtest case.
  * The baseline is 1.79 MiB, committed in full.
  * `check` passes against CMake Release, CMake Debug (`-O0`), the Makefile build and a clang build.
  * It fails on deliberate perturbations.
  * Runs are bit-reproducible except for uninitialised detector members (2 testbench names).
    Object listings come out in pointer order and so vary across builds; that is handled.
* Notable runtime defects found while building the case list:
  * `specs -t mesh` segfaults (use-after-free).
  * `-m/--mode` is silently overridden by `.tran`/`.dc`/`.op`.
  * Five of the nine shipped netlists don't run.

## 2. How to build (commands that worked)

```sh
R=~/.paseo/worktrees/31cq0kv6/gigantic-donkey     # repo
DEPS=$HOME/.local/opt/specs-deps                   # SystemC 3.0.1 (C++20) + args, CMake config packages

# --- CMake (recommended) -----------------------------------------------------------
cmake -S $R -B /tmp/specs-sprint/build-cmake -G Ninja \
      -DCMAKE_PREFIX_PATH=$DEPS -DCMAKE_BUILD_TYPE=Release -DBUILD_TB=ON \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON            # ~8 s (git-clones googletest 1.12.0)
ninja -C /tmp/specs-sprint/build-cmake specs        # OK, ~30 s wall / 106 s CPU at -j4
ninja -C /tmp/specs-sprint/build-cmake spx          # OK, libspx.so (EXCLUDE_FROM_ALL)
ninja -C /tmp/specs-sprint/build-cmake              # FAILS: unit_tests (utils.h:81 static_assert)
# Without gtest: -DBUILD_TB=OFF  -> configure 0.35 s, `ninja` (all = specs) OK in 32 s.
# Without legacy testbenches: -DBUILD_OLD_TB=OFF -> `specs -p` prints "# No test available #".

# --- unit_tests (needs an out-of-tree patched copy; see section 5) --------------------
cp -r $R/{CMakeLists.txt,cmake,src} /tmp/specs-sprint/patched-repo/
#   utils.h:81  static_assert(false, ...)  -> removed
#   utils.h:22  EXPECT_PRED_FORMAT2(angleIsEqual, x, y) ->
#               EXPECT_PRED2([](const auto &a_, const auto &b_){ return angleIsEqual(a_.m_field, b_); }, x, y)
cmake -S /tmp/specs-sprint/patched-repo -B /tmp/specs-sprint/patched-repo/build -G Ninja \
      -DCMAKE_PREFIX_PATH=$DEPS -DCMAKE_BUILD_TYPE=Release && ninja -C ... unit_tests   # OK

# --- Legacy Makefile (out of tree, no repo file touched) ------------------------------
W=/tmp/specs-sprint/make-wd20; mkdir -p $W && cd $W
ln -s $R/Makefile Makefile                 # ADDITIONAL_DEPS = Makefile $(CONFIG_MK_FILE) is cwd-relative
cat > config.local.mk <<EOF                # picked up from the cwd, not the repo root
BIN_NAME = specs
LIB_NAME = libspecs.so
SYSTEMC_PATH_ROOT = $DEPS
SYSTEMC_PATH_LIBS = \${SYSTEMC_PATH_ROOT}/lib64
SYSTEMC_PATH_INCLUDE = \${SYSTEMC_PATH_ROOT}/include
CXX = g++
SRC_PATH = $R/src
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -march=native -g     # c++17 (as in config.mk) fails at link
INCLUDES = -I\${SYSTEMC_PATH_INCLUDE}                         # args.hxx is in the same prefix
LDFLAGS += -L\${SYSTEMC_PATH_LIBS} -lsystemc -lm -Wl,-rpath -Wl,\${SYSTEMC_PATH_LIBS}
BUILD_PATH = build
BUILD_TB ?= 1
EOF
make -j4 bin lib \
  SOURCES_BIN="$(find $R/src -name '*.cpp' -not -ipath '*/tb/*' -not -ipath '*/test/*')" \
  SOURCES_LIB="$(find $R/src -name '*.cpp' -not -ipath '*/tb/*' -not -ipath '*/test/*' -not -name main.cpp -not -ipath '*/parser/*.cpp')"
# OK: 55.6 s wall / 186 s CPU; specs 37.6 MB, libspecs.so 17.6 MB (-g).
# Its golden outputs are bit-identical to the CMake -O3 build.

# --- Doxygen (out of tree) ------------------------------------------------------------
cd $R && ( cat doc/Doxyfile; echo "OUTPUT_DIRECTORY = /tmp/specs-sprint/doxygen" ) | doxygen -   # 8 s, 78 warnings
```

Other compile times (clean, i5-6600K):
* `specs` + `spx` + gtest with keep-going: 68.5 s wall / 236 s CPU.
* Debug `specs`: 76 s at -j3.
* clang Release `specs`: 59 s at -j3.

Every source is compiled **twice** (once for `specs`, once with `-fPIC` for `spx`), and the same applies to
the Makefile.

Artifacts: `specs` 1.6 MB and `libspx.so` 0.9 MB (Release, no `-g`). The build-tree RPATH points to `$DEPS/lib64`.

## 3. Targets & options

**CMake** (`CMakeLists.txt`, `src/{tb,test,parser}/CMakeLists.txt`):

| target | kind | in `all`? | sources / notes |
|---|---|---|---|
| `specs` | executable | yes | `GLOB_RECURSE src/*.cpp` (CONFIGURE_DEPENDS) minus `tb/`, `parser/`, `test/`. Links `common`, `taywee::args`, `m`, `specs_parser`, `specs_tb` |
| `specs_parser` | OBJECT lib | via specs | `src/parser/*.cpp` + generated `parser.cpp`/`scanner.cpp`. PUBLIC `-I build/generated/parser` |
| `specs_tb` | OBJECT lib | via specs | `alltestbenches.cpp` (+ `*_tb.cpp` if `BUILD_OLD_TB`). PUBLIC `-DBUILD_TB=1`, which propagates to *all* `specs` TUs |
| `spx` (`libspx.so`) | SHARED | **no** (EXCLUDE_FROM_ALL) | same as `specs` minus `main.cpp`. **No parser, no tb.** Leaves `sc_main` undefined (provided by the consumer). No SONAME/version |
| `unit_tests` | executable | **yes** (if `BUILD_TB`) | `src/test/{main,devices/waveguide}.cpp`, `-pedantic`. Links `GTest::gtest_main`, `common`, `spx`. `gtest_discover_tests()` |
| `common` | INTERFACE | — | `-Wall -Wextra -march=native -Wfatal-errors -iquote src`, `SystemC::systemc` |
| `gtest`, `gtest_main`, `gmock*` | FetchContent | no | googletest 1.12.0 (`GIT_TAG 15460959…`, `FIND_PACKAGE_ARGS NAMES GTest`) |

| option | default | effect |
|---|---|---|
| `BUILD_OLD_TB` | ON | compile `src/tb/*_tb.cpp` and define `BUILD_TB=1`, which fills `tb_map` (`specs -t/-p`) |
| `BUILD_TB` | ON | **misnamed**: fetches googletest and defines `unit_tests` (it has nothing to do with testbenches) |
| `CMAKE_BUILD_TYPE` | *(empty)* | **no default**: an empty build type gives no `-O` flag at all. `install.sh` configures without one, so its build is unoptimised |
| C++ standard | 20, REQUIRED | extensions ON, so `-std=gnu++20` |

* Dependency discovery: `find_package(SystemCLanguage CONFIG REQUIRED PATHS thirdparty/systemc/build/install)` and
  `find_package(args CONFIG REQUIRED PATHS thirdparty/args/build/install)`. These are the in-tree prefixes
  produced by `install.sh`. `CMAKE_PREFIX_PATH` overrides them.
* SystemC headers arrive as `-isystem`, through the imported target.
* There are **no `install()` rules** and no `install` target.
* `enable_testing()` is only called in `src/test`, so `ctest` must be run from `build/src/test`. The build
  root has no `CTestTestfile.cmake`.

**Makefile** (+ `config.mk`, or `config.local.mk` if present in the **cwd**):
* Targets: `all`/`bin` (`specs`), `lib` (`libspecs.so`, `-fpic -shared -soname libspecs.so`), `test`
  (runs 8 `specs -t` names), `doc`, `clean`/`cleanall`/`cleantraces`, `format` (clang-format -i), `todos`,
  `compiledb`, `waves`/`newwaves` (GTKWave), `rc` (needs `circuits/rc_mohab.cir`, which doesn't exist),
  `print-%`, `help`.
* Flags: `-std=c++17 -O2 -march=native -g`, every `src/` sub-dir passed as `-I`, `-DBUILD_TB` when `BUILD_TB=1`.
* `SOURCES_BIN` = every `.cpp` outside `tb/`, **including `src/test/`**. That was harmless before gtest was
  added (Feb 2025) and breaks the build now.
* `.config.gitlab-ci.mk` is the CI variant: system SystemC, `-O2`, no `-g`.

## 4. Generated code (flex/bison)

* Sources: `src/parser/parser.y` and `src/parser/parser.l`.
  * Bison: `%define api.pure full`, `%param {yyscan_t scanner}`, `%parse-param {ParseTree *pt}`.
  * Flex: `%option reentrant bison-bridge stack caseless yylineno noyywrap extra-type="void *"`.
* **CMake** (`src/parser/CMakeLists.txt`):
  * `find_package(BISON)` and `find_package(FLEX)`, **not REQUIRED**, so a missing tool produces a late,
    confusing error.
  * `bison_target(Parser parser.y build/generated/parser/parser.cpp COMPILE_FLAGS "--warnings -Wall" DEFINES_FILE parser.h)`.
  * `flex_target(Scanner parser.l build/generated/parser/scanner.cpp DEFINES_FILE scanner.h)`.
  * `add_flex_bison_dependency(Scanner Parser)`.
  * The output dir is a PUBLIC `-I` of `specs_parser`, because `main.cpp` includes `"parser.h"` and `"scanner.h"`.
* **Makefile**: `flex --header-file=build/parser/scanner.h -o build/parser/scanner.cpp`, then
  `bison --warnings -Wall --defines=build/parser/parser.h -o build/parser/parser.cpp -d`, included through
  `-iquote build/parser`. Every `.o` has an order-only dependency on the generated sources.
* Generator diagnostics:
  * bison: `parser.y:622 net.oa.bidir` and `parser.y:683 net.edigital` are useless nonterminals
    (2 nonterminals, 3 rules useless).
  * flex: `parser.l:483: dangerous trailing context` (×2).
  * The generated scanner triggers `-Wunused-function yy_top_state`, plus `-Wmisleading-indentation` and
    `-Wunused-but-set-variable yynerrs` under clang.
* The parser is **not** in `libspx.so`, so `pyspecs` (and any library user) can only drive SPECS through the CLI.

## 5. Tests: testbenches, gtest harness, coverage

**Legacy testbenches (`src/tb`).**
* Each `X_tb.{h,cpp}` defines an `SC_MODULE` with a stimulus thread `run_1()` and a `monitor()`. It also
  defines a free function `X_tb_run()`, which:
  1. instantiates the device(s) and the tb module;
  2. sets `specsGlobalConfig.trace_filename = "traces/<x>_tb"` and `trace_all_optical_nets = 0`;
  3. adds `Probe`s, calls `prepareSimulation()` and `sc_start()`;
  4. closes the VCD.
* `alltestbenches.cpp` maps names to functions in `std::map tb_map` (guarded by `#if BUILD_TB == 1`).
* `main.cpp do_test()` looks the name up in `tb_map` and runs it, then prints `Total runtime: X ms`.
* It **always returns 0**: self-check failures (`Failure!`, `Success rate: n/m`) are only printed.
* The 18 names (12 functions; `ac_crow` was removed in v0.4.0: `tests/bench/crow_gen.py --preset crow_tb`
  writes its circuit as a netlist):
  * `wg`/`waveguide`, `merg`/`merger`, `dc`/`directional_coupler`, `spli`/`splitter`, `det`/`detector`
  * `pcm`, `mzi`, `ring`, `ac_add_drop` (freqsweep), `lambda`, `phaseshifter`/`ps`, `mesh`
* Results today:
  * 12 run fine (each in under 0.1 s), writing `traces/<name>.vcd`. `traces/` must exist in the cwd;
    otherwise SystemC raises `E701` and exits 1.
  * `wg`, `merger`, `dc`, `splitter`, `pcm`, `ps` report full success.
  * `detector` prints `Failure!`: `Detector::on_time_tick` never writes `p_readout` (`detector.cpp:84` is
    commented out), and the testbench never enables the detectors.
  * **`mesh` segfaults** (see section 11).

**gtest (`src/test`).**
* `SPECS_TEST_HEADER()` / `SPECS_TEST_FOOTER()` in `utils.h` wrap each test body:
  * the test `fork()`s;
  * the child redirects stdout into a pipe, runs the body inside `try`, and `exit(HasFailure())`s;
  * the parent `waitpid`s, then relays the child's stdout prefixed with `"> "`, and `FAIL()`s on a non-zero
    or abnormal exit.
* **Why fork:**
  * a SystemC kernel can only elaborate and simulate once per process;
  * modules can't be created after elaboration, and `sc_start` can't restart after the end of the simulation;
  * SPECS also keeps global state (`specsGlobalConfig` is a static `sc_module` named "SPX").

  Each test therefore runs in a pristine copy-on-write child.
* Weaknesses:
  * The parent calls `waitpid` *before* draining the pipe, so a child that writes more than about 64 KiB
    deadlocks.
  * An `ASSERT_*` failure inside the child `return`s from the body, skips the footer's `exit()`, and so
    **the child goes on to run the rest of the suite**.
  * The stderr pipe is created but never used.
  * The return values of `pipe`/`write` are ignored.
  * `EXPECT_CPLX_EQ` compares doubles with `EXPECT_FLOAT_EQ`.
  * Tests write `/tmp/specs_test2.vcd` (a fixed path, so parallel runs race).
* Linking `GTest::gtest_main` means gtest's `main()` wins over SystemC's, so `sc_main` in `src/test/main.cpp`
  is dead code. It still works because SystemC creates its context lazily.
* Patched results: `A.B` PASS, `Waveguide.Instantiation` PASS, `Waveguide.OperatingPoint` FAIL (the angle
  check is unfinished: `EXPECT_ANGLE_EQ` → `angleIsEqual`).

**Coverage (`cmake/coverage.cmake`).**
* `add_coverage_target(TARGET … EXCLUDE_PATH … BUILD_TYPES …)` works with GNU only and in Debug by default.
  It needs gcov, lcov and genhtml (all present here).
* It adds `-fprofile-arcs -ftest-coverage` **to the given target only**, runs it, then calls `lcov`/`genhtml`
  and produces a `<target>_cov` custom target.
* It is **not wired**: `coverage.cmake` is never `include()`d, and the call is commented out in
  `src/test/CMakeLists.txt`.
* Even if enabled on `unit_tests`, the SPECS sources live in `spx` (uninstrumented) and `*/test/*` is excluded,
  so coverage would be close to empty.

**Golden harness:** `tests/golden/` (section 8).

## 6. CI (`.gitlab-ci.yml`)

* Image `alpine:latest`, plus the `edge/testing` repo for `systemc-dev` (SystemC version and C++ standard
  unknown). `before_script` installs `bash findutils build-base bison flex doxygen graphviz systemc-dev` and
  copies `.config.gitlab-ci.mk` to `config.local.mk`.
* `GIT_SUBMODULE_STRATEGY: recursive` clones SystemC + args although the Makefile doesn't use them (only
  `-isystem thirdparty/args`).
* Stages:
  * `build`: `make bin V=1` and `make lib V=1`; artifacts `specs` and `libspecs.so`.
  * `test`: only `./specs -t wg` (the others are commented out); artifact `traces/*.vcd`.
  * `doc` (master only): `make doc`, with artifacts `doc/pcm-doc.tar.xz` (never produced, since the tar line
    is commented out) and `doc/html/`.
  * `pages` (master only): copies the HTML to `public/`.
* **Status: broken.**
  * The Makefile compiles `src/test/*.cpp` (no gtest available).
  * CMake is not used.
  * The test stage can't fail on testbench self-check failures, because the exit code is always 0.
* No GitHub Actions workflow.

## 7. Warnings / failures observed

GCC 11.5, CMake Release. 17 distinct warnings; they appear twice in the log because sources are compiled for
both `specs` and `spx`:

| class | where |
|---|---|
| `-Wunused-variable` ×8 | `devices/generic_2x2_coupler.h:35` `lambda0`; `parser/parse_tree.cpp:217` `has_reader`; `specs.cpp:203` `ic_order`; `tb/freqsweep_tb.cpp:60-62` ×3; `tb/lambda_tb.cpp:21-22` ×2 |
| `-Wstrict-aliasing` ×4 | `specs.cpp:385-388` (`(int&)` casts of `uint64`/enums in `sc_trace`). `resolution_multiplier` is also traced as 32-bit |
| `-Wmaybe-uninitialized` | `devices/generic_transmission_device.cpp:221` `delay` |
| `-Wunused-function` | generated `scanner.cpp` `yy_top_state` |
| bison `-Wother` ×4 | `parser.y:622`, `parser.y:683` (useless nonterminals/rules) |
| flex | `parser.l:483: dangerous trailing context` |

**Hard errors:** only `unit_tests`, from `src/test/utils.h:81` `static_assert(false…)`. The `EXPECT_PRED_FORMAT2`
misuse at `utils.h:22` is masked by `-Wfatal-errors`.

Extra warnings in other configurations:
* **clang:** 344× `-Wdeprecated-declarations` for `SC_HAS_PROCESS` (36 uses in `src/`), which is obsolete in
  IEEE 1666-2023. Also `-Wunused-but-set-variable` (`parse_analysis.cpp:39`), `-Wnon-c-typedef-for-linkage`
  (`parser.l:48`) and `-Wmisleading-indentation` (generated scanner).
* **Makefile:** 77× the same `SC_HAS_PROCESS` deprecation under GCC, because SystemC comes in through `-I`
  rather than `-isystem`.

Failures and workarounds (all out of tree or on the command line):

| failure | workaround |
|---|---|
| deps not under `thirdparty/*/build/install` | `-DCMAKE_PREFIX_PATH=$DEPS` |
| `ninja` (all) fails on `unit_tests` | build `specs spx` explicitly, or `-DBUILD_TB=OFF` |
| `unit_tests` does not compile | patched copy in `/tmp/specs-sprint/patched-repo` |
| Makefile: `No rule to make target 'Makefile'` | run from a cwd containing a `Makefile` symlink |
| Makefile: `gtest/gtest.h` not found | override `SOURCES_BIN`/`SOURCES_LIB` without `*/test/*` |
| Makefile `-std=c++17` vs C++20 SystemC: `undefined reference to sc_api_version_3_0_1_cxx201703L` | `-std=c++20` in a cwd-local `config.local.mk` (no file was created in the repo) |
| runtime: `E701 cannot open trace file` | create `traces/` in the cwd |

## 8. Golden harness (`tests/golden/`)

Files:
* `run_golden.py`: stdlib only, 3.9-compatible, run as `python3 -I`.
* `cases.json`: case list, with a note for every exceptional case.
* `README.md`: full usage and rules.
* `baseline/`: 165 files, 1,875,095 bytes, no gzip or summaries needed.
* `.gitignore`: re-includes `*.vcd`/`baseline/**`, which the root `.gitignore` ignores. Verified with
  `git status -uall` (all 168 files show as untracked/committable) and `git check-ignore -v` (the matching
  rule is the negation).

```sh
python3 -I tests/golden/run_golden.py check --specs BUILD/specs [--unit-tests BUILD/src/test/unit_tests] \
        --baseline tests/golden/baseline [-j4] [--strict] [--strict-order] [--rtol 1e-9 --atol 1e-12]
python3 -I tests/golden/run_golden.py record --specs BUILD/specs [--unit-tests …] --out DIR [--repeat 3] [--note …]
python3 -I tests/golden/run_golden.py compare --baseline A --candidate B     # e.g. two records
python3 -I tests/golden/run_golden.py list
```

* **How cases run:** each case runs `specs <args>` (argv[0]=`specs`) in a fresh temp dir that holds a copy
  of `circuits/` (needed because `.include circuits/…` resolves relative to the cwd) and an empty `traces/`.
* **What is captured:** exit status (signals become `signal:SIGSEGV`), normalised stdout and stderr, and every
  file created in the temp dir.
* **Normalisation:** the SystemC banner, `Total runtime`, absolute paths, SystemC `In file:` locations,
  `-nan`, and the VCD `$date`/`$version` blocks.
* **How outputs are compared:**
  * Text is numeric-aware, with ±1 unit allowed in the last printed digit.
  * VCDs are parsed and compared **by signal name, as piecewise-constant waveforms sampled at the union of
    record times**. This is required because SystemC only writes a record when a value changes bit-exactly,
    so `-O0` vs `-O3` adds or drops redundant records.
  * JSON is compared structurally.
  * gtest is compared per test (PASS→FAIL fails; FAIL→PASS is a non-fatal "improvement" note).

**Cases: 44 + gtest.**
* 9 `cir/*` (every `circuits/*.cir`).
* 4 `cir-json/*` (`-n --json`).
* 6 `opt/*` (`-o`, tolerance/`-R` overrides, `-m sampled-time`, `-m fd`, multi-file, `--vci`).
* 19 `tb/*` (every `tb_map` name).
* 6 `cli/*` (help, no args, `-p`, and the error paths).

Nothing is excluded. Expected-failure cases are kept and documented:
* 3 legacy-syntax netlists (`double_ring`, `triple_ring`, `paul_simple_ring`);
* `subckt_test` (`cwsrc` is unknown);
* `paul_3ring_puf` (its CSV is missing);
* `tb/mesh` (SIGSEGV);
* 3 CLI error cases.

**Baseline provenance:**
* `specs` comes from the CMake Release build at d9eb1e6.
* **gtest results come from the patched out-of-tree copy**, because in-tree `unit_tests` doesn't compile.
  This is noted in `manifest.json`.

**Determinism:**
* `record --repeat 3`, and two independent records compared at `rtol=atol=slack=0`, are bit-identical for all
  cases except `tb/det` and `tb/detector`.
* Those two dump the **uninitialised** `Detector::m_cur_readout` and `m_cur_readout_no_interf`
  (`detector.h:18-19`) at t=0 (e.g. `6.92e-310`). Those 4 signals are excluded via `vcd_ignore_values`.
* The `random_device` noise is never exercised by the testbench.

**Pointer-order iteration:**
* `sc_get_all_{object,module}_by_type` return `std::set<T*>`, so the OP listing, `--vci` output and detector
  names follow memory order.
* That order is stable per binary but differs between builds: Debug swaps `uut`/`uut2`, and clang reorders
  `cir/subckt_tests` and `opt/vci`.
* Reorder-only stdout differences fail by default since sprint 2026-10b (B-126), except in the streams a
  case declares in `unordered_streams` (8 cases until F-01 makes iteration name-ordered) or with
  `--lenient-order`.

**Validation:**
* `check` passes (exit 0) for:
  * CMake Release, including `-j4 --strict --strict-order`;
  * CMake Debug;
  * the Makefile `-O2 -g` build (bit-identical);
  * clang Release.
* Max deviation is 1.1e-13 relative; everything also passes with `--atol 0`.
* A wrapper in `/tmp/specs-sprint/perturb/` that perturbs outputs makes `check` **exit 1** with precise
  messages. Detected perturbations:
  * a VCD value ×(1+1e-6);
  * one stdout digit (`0.707107`→`0.707109`);
  * an exit status;
  * a deleted trace file;
  * an extra stdout line;
  * an injected gtest PASS→FAIL.
* A ×(1+1e-12) perturbation passes.
* **Caveat:** with `atol=1e-12`, values below about 1e-3 are compared mostly in absolute terms (for example,
  an `abstol=1e-8` perturbation went unnoticed). Prefer `--atol 0` for tight checks.
* Runtime of a full check: about 2 s.

## 9. Constraints for a meson migration

The meson build must reproduce or decide the following:

1. **Flags.**
   * `-std=gnu++20` (CMake's default with extensions; GNU mode also changes `-ffp-contract`).
     `cpp_std=gnu++20` is the safest choice for bit-identical output.
   * `-Wall -Wextra -march=native -Wfatal-errors`, plus `-pedantic` for tests.
   * `-iquote <src>`: headers are included as `"devices/x.h"` and `"tb/x.h"` from the source root.
   * Release = `-O3 -DNDEBUG` (`NDEBUG` disables `SPECSConfig::verifyConfig` asserts).
   * Choose a **default buildtype**: CMake has none, so it builds at `-O0`.
   * `-march=native` makes binaries non-portable (a CI artifact concern); consider making it an option.
2. **Generated parser.**
   * Generate `parser.cpp/.h` and `scanner.cpp/.h` into the build dir:
     * flex: `--header-file`;
     * bison: `--defines` and `--warnings -Wall`, version ≥ 3 for `api.pure full`.
   * Generated headers must be visible to `main.cpp` and `parser/*.cpp`.
   * Ordering: `scanner.cpp` needs `parser.h`.
   * Require flex and bison explicitly.
3. **Three link products.**
   * `specs`: everything plus parser plus tb.
   * `libspx.so`: no `main`, no parser, no tb, `-fPIC`, `sc_main` left undefined. No SONAME or version
     today; the Makefile names it `libspecs.so` with SONAME `libspecs.so`.
   * `unit_tests`: links the shared lib and SystemC.
   * Sources are compiled twice today; a meson `static_library` / `both_libraries` could share objects,
     but `-fPIC` codegen may alter numerics. Verify with the golden harness.
4. **Testbench toggles.**
   * `BUILD_OLD_TB` compiles `src/tb/*_tb.cpp` and adds `-DBUILD_TB=1` to **all `specs` TUs** (`main.cpp`
     uses it).
   * `BUILD_TB` gates gtest; rename it (e.g. `tests` / `testbenches` feature options).
   * The golden `tb/*` cases need testbenches ON.
5. **gtest.**
   * Dependency `gtest` (`gtest_main`), with a wrap fallback, since CMake fetched 1.12.0 from git.
   * Each test should be a separate meson `test()`, or use `protocol: 'gtest'`.
   * Note that gtest's `main` replaces SystemC's.
   * The in-tree test currently fails to compile, so tests must not be part of the default build until fixed.
6. **SystemC discovery without `.pc`.**
   * Use `dependency('SystemCLanguage', method: 'cmake', modules: ['SystemC::systemc'])`, which honours
     `CMAKE_PREFIX_PATH`; also allow pkg-config.
   * SystemC must be **C++20-built** (ABI symbol `sc_api_version_3_0_1_cxx202002L`); a C++17 consumer fails
     at link.
   * Use `include_type: 'system'` or the Makefile's 77 deprecation warnings come back.
   * Alternatively, define `SC_ALLOW_DEPRECATED_IEEE_API`, or fix the 36 `SC_HAS_PROCESS` uses.
7. **args.** taywee/args is header-only (`args.hxx`) and has a CMake config but no `.pc`. Use
   `method: 'cmake'` or an include-dir fallback (`thirdparty/args`).
8. **RPATH / install.**
   * The build tree currently has an RPATH to the SystemC lib dir; keep `build_rpath`/`install_rpath`.
   * There is no install today; decide what to install (`specs`, `libspx`, headers?).
   * `pyspecs` drives the `specs` binary via `subprocess`.
9. **Runtime cwd assumptions** (relevant to `meson test` / CI):
   * Traces go to `traces/…` relative to the cwd and fail if the dir is missing.
   * `.include` paths are cwd-relative.
   * The golden harness handles both by using temp dirs; wire it as a `test()`.
10. **Also replace:**
    * Doxygen `make doc`: Doxyfile `OUTPUT_DIRECTORY=doc`; it lists a missing `QUICKSTART.md` and two
      obsolete tags.
    * `make format` (clang-format) and `make todos`.
    * The CI that currently uses Makefile + Alpine.
    * `install.sh`, which builds SystemC in-tree.

## 10. Unknowns

* Which SystemC version and C++ standard Alpine `edge/testing systemc-dev` currently ships. That decides
  whether any Alpine-based CI can link at all.
* Whether `gtest_discover_tests` is expected to work with the fork harness under `ctest -j`, given the
  `/tmp/specs_test2.vcd` race.
* Intended semantics of `-m/--mode` versus the analysis directives (commit 1b0780d says "Give cmdline args
  priority over netlist").
* Why the Makefile `-O2` ISO-C++ build is bit-identical to CMake `-O3 gnu++20`, while `-O0` and clang differ
  by about 1e-13. FMA contraction is presumably not generated in the hot paths, but this was not verified at
  the asm level.
* `circuits/*.cir` are tracked although the root `.gitignore` ignores `*.cir` (they were force-added). It is
  unclear whether that is intended.

## 11. Seeds for backlog

- [severity: high] src/test/utils.h:81 — `static_assert(false, "Work in progress…")` makes `unit_tests` (and therefore the default `all` target when BUILD_TB=ON, the default) fail to compile. Behind it, utils.h:22 `EXPECT_ANGLE_EQ` passes a bool-returning 2-arg `angleIsEqual` to `EXPECT_PRED_FORMAT2`, which also fails. The `angleIsEqual` overloads (utils.h:83-107) use unqualified `is_floating_point_v`.
- [severity: high] src/specs.cpp:359-361 + src/devices/clements.cpp:39-40 — `prepareSimulation()` calls `init()` on every `spx_module` collected beforehand. `Clements::init()` clears and re-creates its `MeshCol` children, so later iterations call into freed objects. `specs -t mesh` (src/tb/mesh_tb.cpp, which also calls `mesh1.init()` itself) segfaults every time, after 81 W505 "object already exists" warnings.
- [severity: high] src/devices/detector.h:18-19 — `m_cur_readout` and `m_cur_readout_no_interf` are never initialised but are traced. The VCD dumps garbage at t=0 (e.g. 6.9e-310), which is the only run-to-run nondeterminism in the golden suite.
- [severity: high] Makefile:25-26 + config.mk:20 — the legacy Makefile globs `src/test/*.cpp` (gtest) into `specs`/`libspecs.so` and uses `-std=c++17`, which cannot link against the C++20 SystemC (`sc_api_version_3_0_1_cxx201703L`). As shipped it is broken, and it is what CI uses.
- [severity: high] .gitlab-ci.yml:31-32,42 — CI builds with the broken Makefile on Alpine, never uses CMake, and runs only `./specs -t wg`. The pipeline cannot be green and tests nothing meaningful.
- [severity: medium] src/parser/parse_analysis.cpp:26,32,85 — `.op`/`.tran` unconditionally overwrite `specsGlobalConfig.simulation_mode`. CLI `-m sampled-time` / `-m fd` print a message but yield bit-identical traces (golden `opt/mode-*` cases). This contradicts "cmdline args priority" (1b0780d).
- [severity: medium] src/main.cpp:56-62 — `do_test()` always returns 0, so testbench self-check failures (`Failure!`, e.g. `-t detector`) never fail `specs -t`, CI or the Makefile `test` target.
- [severity: medium] src/utils/sysc_utils.h:39-67 — `sc_get_all_{module,object}_by_type` return `std::set<T*>`, ordered by pointer. Init order, OP-result listing (specs.cpp:348), probe creation and `--vci` output depend on memory layout and change between -O0/-O3 and GCC/clang. Use name order or `std::vector` in hierarchy order.
- [severity: medium] src/test/utils.h:28-76 — fork harness: the parent `waitpid`s before draining the stdout pipe (deadlock when a child writes more than ~64 KiB). An `ASSERT_*` failure in the child returns past the `exit()` in `SPECS_TEST_FOOTER`, so the child keeps running the rest of the suite. The stderr pipe is unused and `pipe`/`write` results are ignored.
- [severity: medium] src/devices/detector.cpp:84 — `p_readout->write(...)` is commented out, so detector readouts never reach the output port and `detector_tb` self-checks fail.
- [severity: medium] CMakeLists.txt:11-12 (no default build type) / install.sh:28 — no default `CMAKE_BUILD_TYPE`. `install.sh` configures without one, giving an unoptimised (`-O0`-equivalent) build. `-march=native` is unconditional (non-portable binaries).
- [severity: medium] src/test/CMakeLists.txt:1 + CMakeLists.txt:5-6 — option `BUILD_TB` actually gates the gtest unit tests while `BUILD_OLD_TB` gates the testbenches. `unit_tests` is part of `all`, and `enable_testing()` is only in `src/test`, so `ctest` from the build root finds no tests.
- [severity: medium] src/specs.cpp:366-369 — `default_trace_file->set_time_unit(...)` is called without a null check. With an empty `trace_filename` (possible via API/tests) this dereferences null.
- [severity: low] src/specs.cpp:385-388 — `(int&)` casts in `sc_trace(SPECSConfig)` break strict aliasing (4 warnings). `default_resolution_multiplier` (uint64) is traced as 32 bits.
- [severity: low] src/devices/generic_transmission_device.cpp:221 — `delay` may be used uninitialised (-Wmaybe-uninitialized).
- [severity: low] src/parser/parser.y:622,683 + src/parser/parser.l:483 — useless nonterminals `net.oa.bidir` and `net.edigital` (3 useless rules), and a flex "dangerous trailing context" rule.
- [severity: low] src/**/*.h (36×) — `SC_HAS_PROCESS` is obsolete in IEEE 1666-2023 (344 clang warnings, 77 with the Makefile's `-I` include).
- [severity: low] circuits/double_ring.cir, triple_ring.cir, paul_simple_ring.cir, subckt_test.cir, paul_3ring_puf.cir — 5 of 9 shipped netlists do not run: legacy syntax, an unknown `cwsrc` element, and a missing `paul_puf_bitstream_158b_3wl.csv`.
- [severity: low] cmake/coverage.cmake — never included; the call is commented out (src/test/CMakeLists.txt:27). As written it would instrument only the test executable, not `spx`, giving near-empty coverage.
- [severity: low] src/test/main.cpp:9-15 — `sc_main` is dead code because `GTest::gtest_main` provides `main()`. Tests write the fixed path `/tmp/specs_test2.vcd` (waveguide.cpp:122,142).
- [severity: low] src/main.cpp:146 — `new ParserState()` passed to `yylex_init_extra` is never freed. The unused `do_circuit()` (main.cpp:65-141) and `raphael_main()` (187) are dead code.
- [severity: low] doc/Doxyfile — INPUT lists the removed `QUICKSTART.md`; obsolete tags `TCL_SUBST` and `COLS_IN_ALPHA_INDEX`; 78 warnings. `.gitlab-ci.yml` `doc` expects `doc/pcm-doc.tar.xz`, which is never produced (Makefile:212 commented out).
- [severity: low] Makefile:279-280 — `rc` target references the non-existent `circuits/rc_mohab.cir`. `ADDITIONAL_DEPS = Makefile` (Makefile:65) breaks out-of-tree use.
