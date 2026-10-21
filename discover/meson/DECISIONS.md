# Meson migration — decisions

Decisions taken while implementing `PLAN.md` (unattended run, 2026-10-20). Each one says what was
chosen and why. Items a maintainer may want to revisit are marked **[review]**.

## Layout and project settings

1. *(Superseded by 34: `subprojects/`, wraps.)* **`subproject_dir: 'thirdparty'`.** The existing submodules (`thirdparty/systemc`, `thirdparty/args`)
   double as subprojects, so there is one place for third-party code. The gtest wrap lives at
   `thirdparty/gtest.wrap`; its downloads (`thirdparty/googletest-*/`, `thirdparty/packagecache/`,
   `thirdparty/.wraplock`) are git-ignored. The cost is that wrap downloads sit next to tracked submodules,
   which the ignore rules handle.
2. **`meson.options`** (not `meson_options.txt`): requires Meson ≥ 1.1, which `meson_version: '>=1.3.0'` already implies.
3. **`meson_version: '>=1.3.0'`**: Ubuntu 24.04 ships 1.3.2. A configure with 1.3.2 was tested (VALIDATION.md).
   CI pins Meson 1.11.2 from pip (the version tested here); Meson 1.12 will need Python ≥ 3.10.
4. **`cpp_std=gnu++20`**, as in PLAN.md: CMake's `CMAKE_CXX_STANDARD 20` with extensions compiled with
   `-std=gnu++20`, and the GNU mode affects FP contraction on GCC. The outputs are bit-identical to the
   CMake baseline (VALIDATION.md).
5. **`buildtype=release`** by default (CMake had no default and built at `-O0` from `install.sh`).
6. **`b_ndebug=false` by default** (PLAN.md): the code validates user input with `assert`
   (`SPECSConfig::verifyConfig`, the parser), so the shipped build keeps the asserts.
   **Side effect [review]:** with `b_ndebug=false`, Meson ≥ 1.4 also defines `-D_GLIBCXX_ASSERTIONS=1`
   (libstdc++ bounds and precondition checks; libc++ gets `_LIBCPP_HARDENING_MODE_FAST` instead). This only
   changes behaviour on undefined behaviour, which then aborts instead of continuing silently. The golden
   outputs are bit-identical with and without it. On a 6,000-point DC sweep (`add_drop.cir` with
   `dlambda=5 pm`), user CPU time is the same 0.32 s for CMake Release, Meson default and Meson
   `-Db_ndebug=true` (VALIDATION.md §g). Users who want the exact CMake Release
   configuration can pass `-Db_ndebug=true`.
7. **`-march=native` is the option `native` (default true)**, so the bit-compatibility with the baseline
   holds on the build host. CI uses `-Dnative=false` because its artefacts must run on other machines.
   The golden check also passes with `-Dnative=false` (VALIDATION.md).
8. **`warning_level=2`** (`-Wall -Wextra`), **`-Wfatal-errors`**, and **`-iquote <repo>/src`** are project
   arguments, as in CMake's `common` interface target. `-pedantic` is only added to `unit_tests`, as before.
9. **`implicit_include_directories: false`** on every target: CMake did not add `-I<src dir>`/`-I<build dir>`
   either, and an extra `-I<repo>/src` would change how angle-bracket includes resolve.

## Targets

10. **Parser as a separate non-PIC static library, linked with `link_whole`.** CMake had an OBJECT library
    without `BUILD_TB=1`. A static library with `link_whole` keeps every object, like an object library, and
    keeps the per-TU flags identical. `pic: false` matches the old non-PIC objects; Meson's
    `b_staticpic` default would otherwise add `-fPIC`. The generated headers are reached via
    `-I<builddir>/src/parser` (build dir only). `include_directories('.')` would also add the source dir.
11. **Testbenches are compiled as part of `specs`** (with `BUILD_TB=1`, like the core). CMake had a separate
    tb OBJECT library, so tb TUs now also see `-I<builddir>/src/parser`. This is harmless: no tb file
    includes a generated header. It was kept to avoid a third library.
12. **Objects are not shared between `specs` and `libspx.so`** (each source is compiled twice, as before):
    sharing would mean `-fPIC` code in `specs`, which PLAN.md flags as a possible numerical change.
13. **`spx`** mirrors CMake: core + devices, no `main.cpp`, no parser, no testbenches. SONAME is
    `libspx.so` (CMake had none); it is not versioned, which is a separate decision. It is not built by
    default, like CMake's `EXCLUDE_FROM_ALL`. Meson puts every `install: true` target into `all`, so
    "install when built" is impossible. A new boolean option **`spx`** (default false) therefore controls
    both `build_by_default` and `install`. Without it, `meson compile -C build spx` still builds the
    library. `unit_tests` links it, so with the tests enabled it gets built anyway, but it is not
    installed. Both CIs pass `-Dspx=true`, so the library is an artefact.
14. **Link order.** Meson links the core objects first, then the testbenches, then the parser archive
    (CMake: tb, parser, core). Static-initialisation order could depend on it, but the golden outputs are
    bit-identical (`--strict-order` included), so it was not forced.
15. **`install_rpath`** points to the SystemC library directory when SystemC comes from a non-system prefix
    (pkg-config `libdir`, or the first `cmake_prefix_path` entry with `lib64/` or `lib/libsystemc.so`). Meson
    drops build-tree rpaths on install, so an installed `specs` could not otherwise find a SystemC in e.g.
    `~/.local/opt/specs-deps`. Meson's CMake dependency does not expose the package prefix, hence the lookup.
    **Limitation:** a SystemC found through the `CMAKE_PREFIX_PATH` environment variable or `SystemC_DIR`
    gets no install rpath (the Meson DSL cannot read the environment). Pass `-Dcmake_prefix_path` instead,
    or set `LD_LIBRARY_PATH`. The build-tree rpath is always correct.

## Dependencies

16. *(Superseded by 35: the wrap by default, an installed SystemC on request.)* **SystemC lookup order:** pkg-config `systemc` → CMake package `SystemCLanguage` (`SystemC::systemc`,
    honours `-Dcmake_prefix_path`) → CMake subproject `thirdparty/systemc`. For the first two, a `cxx.links()`
    probe checks that SystemC was built with the same C++ standard (the `sc_api_version_*_cxx<std>`
    symbol). On a mismatch, Meson prints a warning and moves to the next candidate, instead of failing late
    at link time.
16b. **The CMake user package registry is ignored.** SystemC's CMake build `export(PACKAGE)`s its build
    tree into `~/.cmake/packages`. Lookups pass `-DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF` (and the pre-3.16
    variable), so a stale scratch build is never picked instead of the fallback. `utils/build-systemc.sh`
    and the subproject also don't register their build trees.
17. *(Now the default build, from `subprojects/systemc.wrap`; options unchanged, see 35.)* **CMake-subproject fallback** (`thirdparty/systemc` via Meson's `cmake` module). Options:
    `CMAKE_CXX_STANDARD=20`, `ENABLE_EXAMPLES=OFF`, `ENABLE_REGRESSION=OFF`,
    `CMAKE_EXPORT_NO_PACKAGE_REGISTRY=ON`, `warning_level=0`. Three non-obvious points:
    - **Shared** (`BUILD_SHARED_LIBS=ON`) and **installed** (`set_install(true)`, library only).
      A static SystemC cannot go into `libspx.so` without PIC. Even with PIC, `unit_tests` (which links
      both `libspx.so` and SystemC) would get two SystemC kernels. `install_rpath` is then
      `<prefix>/<libdir>`.
    - **`-xnone` appended to SystemC's C args.** Meson cannot evaluate SystemC's
      `$<COMPILE_LANGUAGE:ASM>` generator expression, so the `-xassembler-with-cpp` meant for the
      QuickThreads `.s` file lands on `qt.c` too, which then fails to compile. `-xnone` restores
      suffix-based languages. The `.s` file then skips cpp: its only directive (`#if __linux__ && __ELF__`
      around `.note.GNU-stack`) is a comment to gas, so the section is always emitted, which is right on
      Linux. **[review]** On non-Linux/non-ELF hosts, prefer an installed SystemC (`-Dcmake_prefix_path`).
      The alternative was `ENABLE_PTHREADS=ON` (no assembly), but OS threads as coroutines are much slower
      for SPECS's many `SC_THREAD`s.
    - **`b_lundef=false`** for the subproject, because `libsystemc.so` calls the user's `sc_main`.
    - **[review]** `meson install` puts this `libsystemc.so` into `<prefix>/<libdir>` (default
      `/usr/local/lib64`). There it can shadow, or be shadowed by, another SystemC, possibly one built
      with another C++ standard. Meson's `cmake` module cannot redirect the install dir. Installs that
      matter should use a separately installed SystemC (`install.sh` does).
18. *(Superseded by 36.)* **args:** CMake package `taywee::args`, else `-isystem thirdparty/args` (header-only).
19. **gtest: WrapDB `gtest 1.17.0-4`** (CMake used FetchContent 1.12.0). 1.17.0 is the newest release line
    with several WrapDB revisions (1.18.0 had just one), it supports C++17/20 and it builds cleanly with
    GCC 11 and clang 19. The gtest results (`A.B` PASS, `Waveguide.Instantiation` PASS,
    `Waveguide.OperatingPoint` FAIL) are identical to the baseline's.
20. **`tests` (feature, auto) uses `allow_fallback: true`.** Meson only falls back to a wrap for *required*
    dependencies, so without this an `auto` build on a machine without system gtest would silently drop the
    unit tests. CMake always fetched gtest. Offline builds: `-Dtests=disabled` or `--wrap-mode=nodownload`
    (with a pre-populated `thirdparty/packagecache`). **[review]** PLAN.md's "no configure-time network access
    for normal builds" holds for `-Dtests=disabled` or a system gtest. With the default `auto` and no
    system gtest, the first `meson setup` downloads gtest once (the download is cached in `thirdparty/`).

## Tests

20b. **`unit_tests` uses gtest without `gtest_main`** (`main: false`). The gtest wrap ignores
    `main: true`, so `main: true` would make the entry point depend on where gtest comes from: gtest's
    `main` with a system gtest, SystemC's `main` with the wrap. With `main: false` it is always SystemC's
    `main` → `sc_main` in `src/test/main.cpp` → `RUN_ALL_TESTS()`. Under CMake (`GTest::gtest_main`),
    that `sc_main` was dead code. The gtest results are identical (VALIDATION.md).
21. **Known failure `Waveguide.OperatingPoint`:** split into two tests. `unit` (gtest protocol,
    `--gtest_filter=-Waveguide.OperatingPoint`) and `unit-known-failure` (`should_fail: true`, only that
    test). When the angle check is fixed, `unit-known-failure` turns red, which prompts merging it back and
    re-recording the golden gtest baseline. `should_fail` is deprecated in Meson 1.11 in favour of
    `expected_fail`, but `expected_fail` does not exist in the 1.3 minimum, so `should_fail` stays (a NOTICE
    only). *Superseded (fix phase, B-123): the angle check is fixed, `unit-known-failure` is gone and `unit`
    runs every test.*
22. **`unit`, `unit-known-failure` and `golden` are `is_parallel: false`:** the unit tests write the fixed path
    `/tmp/specs_test2.vcd`, and the golden test runs `unit_tests` too. (`unit-known-failure` no longer
    exists since B-123.)
23. **`tb` suite:** one test per name of the old `make test` (`waveguide splitter merger detector
    directional_coupler phaseshifter mzi pcm`). `ring`, `ac_add_drop`, `ac_crow` and `mesh` stay disabled, as
    they were commented out in the Makefile; `mesh` segfaults (backlog). They run through `sh -c 'mkdir -p
    traces && exec specs -t <name>'` in the build root, so traces go to `<builddir>/traces/`. As before they
    only catch crashes, because `specs -t` exits 0 even when it prints `Failure!`. The golden suite covers all
    19 names with outputs. *Since B-047 `mesh` no longer segfaults and is in the `tb` suite.*
24. **`golden` test** = the CLAUDE.md gate command (default tolerances, `-j 4`, `--unit-tests` when built). It is
    only registered with `-Dold_tb=true`, because the `tb/*` cases need the testbenches.

## Helper targets (Makefile parity)

25. **`doc`**: `run_target` → `utils/doc.sh`. It runs doxygen from the repo root (the Doxyfile's paths are
    relative to it) with only `OUTPUT_DIRECTORY` overridden to `<builddir>/doc`, and copies `doc/resources`
    to `<builddir>/doc/html/`. The output is no longer written into the source tree (`doc/html`). The
    Doxyfile itself is unchanged; its stale entries (`QUICKSTART.md`, obsolete tags) are a backlog item.
26. **`cleantraces`, `cleanoldtraces`, `waves`, `newwaves`**: `run_target`s → `utils/traces.sh`, with the
    same logic as the Makefile. They act on `<repo>/traces`, where `build/specs …` run from the repo root writes.
27. **`todos`**: `run_target` → `utils/todos.sh`, with the same awk as the Makefile.
28. **Dropped:** `make format` (Meson's automatic `clang-format` target exists and must not be run: CLAUDE.md
    says so), `make rc` (its netlist doesn't exist), `make compiledb` (Meson always writes
    `compile_commands.json`), `make print-%`/`help` (`meson configure`/`meson introspect` cover them), and
    `clean`/`cleanall`/`cleandoc` (delete the build dir; docs are in it now).
29. **Coverage**: Meson built-ins (`-Db_coverage=true`, `ninja coverage-html`, which needs gcovr, or lcov +
    genhtml). `cmake/coverage.cmake` (never wired) is removed.

## CI and install

30. *(SystemC part superseded by 37.)* **CI image/runner: Ubuntu 24.04** for both GitHub Actions (`ubuntu-24.04`, pinned) and GitLab
    (`ubuntu:24.04`, replacing Alpine, whose `systemc-dev` standard is unknown). SystemC is built from the
    submodule as C++20 by `utils/build-systemc.sh` (shared by both CIs and `install.sh`). The script
    is idempotent: a stamp file holds the SystemC commit plus the compiler's `--version` line without
    the program name, so `c++` and `g++` match. It is cached under
    `.deps/systemc`: GitHub keys the cache on submodule SHA + compiler, GitLab uses a static key
    (`systemc-ubuntu24.04-gcc`) and relies on the stamp file for correctness.
31. **GitHub matrix: gcc + clang.** clang builds without errors; its only warnings are the known
    `SC_HAS_PROCESS` deprecations. A non-gating `sanitizers` job (debug, ASan + UBSan,
    `b_lundef=false`, `continue-on-error: true`) is included. Docs are built in the gcc leg and uploaded
    as an artefact. GitHub Pages is not deployed: publishing is left to the maintainer.
32. **GitLab:** `build` (`-Dspx=true`; artefacts `specs`, `libspx.so` and the `libsystemc.so*` they need) →
    `test`, which configures and builds its own `build/` and runs `meson test` (artefacts: meson logs, tb
    traces). A build dir passed as an artefact is not reusable: fresh-checkout timestamps, absolute
    paths, and gtest wrap sources outside `build/` trigger a full reconfigure, so the `test` job does
    this explicitly. `doc` + `pages` run on master, as before; the doc job configures with `-Dtests=disabled`.
    GitHub also ships `libsystemc.so*` with its binary artefacts. Downloaded binaries need
    `LD_LIBRARY_PATH`, because their rpath points into the runner workspace.
33. *(Superseded by 37.)* **`install.sh`:** submodules → `utils/build-systemc.sh .deps/systemc` → `meson setup build
    -Dcmake_prefix_path=.deps/systemc` → `meson compile` → `meson test`. SystemC is no longer built inside
    the submodule work tree (`thirdparty/systemc/build`), which used to leave the submodule dirty.

## Dependencies from wraps (2026-10-21, third round, before v0.2.0)

Maintainer decision in `backlog/DECISIONS-2026-10-21.md` ("third round"); implemented in phase PC2.

34. **`subprojects/` (Meson's default), no submodules.** `thirdparty/` is renamed and `subproject_dir` dropped.
    SystemC 3.0.2 and taywee/args 6.4.7 come from `[wrap-file]` wraps (GitHub tag tarballs, sha256), like
    gtest. Both tarballs were compared with `diff -r` to the former submodule commits (which are the tags):
    args is identical; SystemC lacks only the dotfiles its `.gitattributes` marks `export-ignore`
    (`.github/`, `.mailmap`, `.gitmodules`, `.gitattributes`, a regression-test marker), none used by its
    CMake build. `.gitignore` ignores `subprojects/*` except `*.wrap` and `packagefiles/`. Offline:
    `meson subprojects download`, then `--wrap-mode=nodownload`.
35. **SystemC from the wrap by default; an installed one only on explicit request.** `default_options`
    has `force_fallback_for=systemc,args`. `meson.build` reads `wrap_mode` and `force_fallback_for`:
    - default: `cmake.subproject('systemc')` with the options of 17, no lookup of an installed SystemC
      (a `-Dcmake_prefix_path` alone does not change that);
    - `-Dforce_fallback_for=` (without `systemc`): pkg-config, then the CMake package, each with the ABI
      check of 16; if none is usable, the wrap (Meson's usual fallback);
    - `--wrap-mode=nofallback`: the same lookup, and an error if none is usable. **nofallback wins over
      the project's default `force_fallback_for`**: Meson's own `dependency()` gives `force_fallback_for`
      precedence, but it cannot tell a project default from a user value, and a user who asks for no
      fallback means it. **[review]**
    - `--wrap-mode=forcefallback`: always the wrap.
    `dependency()`'s automatic fallback is not used because the CMake subproject needs options
    (`cmake.subproject_options()`) that `dependency()` cannot pass; the wraps still have `method = cmake`
    (Meson ≥ 1.3) and a `[provide]` section, so `meson subprojects` commands and a plain `subproject()`
    treat them correctly. Checked with Meson 1.3.0 and 1.11.2. The rpath handling of 15 is unchanged
    (prefix libdir for an installed SystemC, `<prefix>/<libdir>` for the subproject).
36. **args through its CMake build, not its `meson.build`.** args 6.4.7 ships a `meson.build` (providing
    `args_dep`), but it calls `install_headers('args.hxx')` unconditionally, so `meson install` would
    install `<prefix>/include/args.hxx`. Its CMake build is used instead (`cmake.subproject('args')`,
    `ARGS_BUILD_EXAMPLE/UNITTESTS=OFF`, `set_install(false)`): an INTERFACE target, nothing built or
    installed. An installed args (CMake package `taywee::args`) follows the same rule as SystemC (35).
37. **CI and `install.sh` build SystemC as a subproject.** No `utils/build-systemc.sh` (removed), no prefix
    or prefix cache, no submodules. CI keeps `CC`/`CXX` per matrix entry, prefixed with `ccache` (Meson only
    adds ccache itself when they are unset), caches `subprojects/packagecache` (GitHub: keyed on the
    `.wrap` files; GitLab: a fixed key, Meson checks the hashes) and `.ccache`, and clones the full
    history for `git describe`. The binaries artefact ships `build/subprojects/systemc-*/libsystemc.so.[0-9]*[0-9]`
    (`libsystemc.so*` would also match the `libsystemc.so.<version>.p/` object directory).
    `install.sh` is plain setup/compile/test.
38. **The SystemC subproject is never sanitized** (`set_override_option('b_sanitize', 'none')`). Sanitizer builds
    used to link an unsanitized, installed SystemC. Built from the wrap, SystemC inherited
    `-fsanitize=address,undefined`, and UBSan's vptr check fires in SystemC's own process dispatch
    (`sc_process.h:633`, a member-function pointer called on `sc_process_host`) for every process: with
    `meson test`'s `halt_on_error`, 11/12 tests and 123/212 golden cases failed (gcc). With the override, the
    SPECS objects are sanitized as before and SystemC's are not (checked in `compile_commands.json`, Meson
    1.3.0 and 1.11.2).
