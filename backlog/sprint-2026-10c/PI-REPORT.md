# PI report — pixi builds must not pick up the host's gtest (branch `ws/pixigtest`)

## Outcome
Fixed in `78ca588` (`build: pixi builds use the googletest wrap, never the host's gtest`).

- **Cause.** The report had it right except for the lookup method. Meson looks up gtest with pkg-config, then its
  "system" method (`find_library('gtest')` with no include path, or the sources in `/usr/src/gtest`,
  `/usr/src/googletest`). It does not use CMake for gtest. The environment's pkg-config (conda-forge 0.29.2) reports
  `pc_path` = the env's `lib/pkgconfig:share/pkgconfig` only, but it still reads the host's `/usr/lib/pkgconfig`
  after those (`pkg-config --debug`: `Reading 'gtest' from file '/usr/lib/pkgconfig/gtest.pc'`, "Path position 4").
  Setting `PKG_CONFIG_LIBDIR` to the env's directories **does not stop it**, so no activation variable can fix it.
  `gtest.pc` gives `-DGTEST_HAS_PTHREAD=1 -lgtest` (pkg-config drops `-I/usr/include`), and the conda compilers do
  not search `/usr/include` (their sysroot replaces it): `gtest/gtest.h: No such file or directory`.
- **Change.** `utils/pixi-setup.sh` passes `-Dforce_fallback_for=systemc,args,gtest` to `meson setup`. It goes before
  the options given after `--`, so a user's `-Dforce_fallback_for=...` still wins (checked: with two `-D` values,
  Meson keeps the last). The plain route is unchanged: `meson.build` keeps `force_fallback_for=systemc,args`,
  so an installed gtest is still used when there is one.
- **Docs.** README pixi section (why the option is there, and that a plain `meson setup` inside `pixi shell` needs it
  too); CHANGELOG `[unreleased]` Fixed (also: remove a `build-pixi/<toolchain>` configured before the fix).
- **Golden impact.** None. This is build configuration only. The `specs` and `unit_tests` binaries of the clang build
  are byte-identical with the old and new option values on this host (sha256 `ebda52cc…` / `fdb04e9a…`).

## Decisions taken
1. **The WrapDB gtest (forced fallback), not gtest from conda-forge.** Reasons:
   - It is one line, with the same mechanism the project already uses for SystemC and args.
   - It needs no `pixi.lock` re-solve, and the gtest version is the one CI and the plain route use without a
     system gtest (1.17.0).
   - The wrap is already in the shared wrap cache.
   - It does not depend on how conda-forge's pkg-config orders its search: an env `gtest.pc` would come first
     today, but the host one would still be reachable if the env package were ever missing.
   - It also blocks Meson's "system" method, which would otherwise compile a host gtest from
     `/usr/src/googletest` (Debian/Ubuntu `googletest` package).
2. **The fix lives in `pixi-setup.sh`, not in `meson.build`'s default options**, because the task requires the plain
   route to keep preferring an installed gtest.
3. **Existing `build-pixi/<toolchain>` directories are not migrated automatically.** `pixi-setup.sh` still accepts
   an existing directory as is; the CHANGELOG says to remove one configured before the fix (or
   `meson configure build-pixi/<tc> -Dforce_fallback_for=systemc,args,gtest`). On an affected host such a directory
   never built its unit tests, so nothing is lost.

## Other dependency lookups in a pixi build (checked)
- **SystemC, args:** already forced to their wraps (project default, kept by the script). Only an explicit
  `-Dforce_fallback_for=` makes Meson look for installed copies.
- **threads; `m` (`cxx.find_library`):** resolved by the compiler and linker. Their search directories are the env and
  its sysroot only (`g++/clang++ -print-search-dirs`, `ld --verbose` with `=` sysroot-relative dirs).
- **SystemC CMake subproject:** its only `find_package` is `Threads`, which the compiler resolves. CMake's
  `CMAKE_SYSTEM_PREFIX_PATH` does include `/usr`, but no default lookup goes through CMake.
- **Programs** (`flex`, `bison`, `python3`, `cmake`, `ninja`, `pkg-config`): taken from the env first on the PATH.
  `sh` and `git` come from the host, but they are programs, not libraries the build links.

No other lookup needed the same fix.

## Verification
- **Reproduction (rootless podman, `archlinux:latest` + `pacman -S gtest pixi git`, gtest 1.18.0, pixi 0.83.0).**
  The repository was passed in read-only as a git bundle, cloned inside, then `pixi install --locked` and
  `pixi run test gcc`.
  - Before (0e0993d): build fails at `src/test/devices/crossing.cpp: fatal error: gtest/gtest.h: No such file or
    directory`. The compile line has no gtest include path, as in the maintainer's report. `meson introspect
    --dependencies` shows `gtest pkgconfig 1.18.0`, and the meson log shows `pkg-config --modversion gtest` -> found.
  - After (78ca588): `pixi run test gcc` 12/12 OK and `pixi run test clang` 12/12 OK. The log shows
    `Dependency gtest ... from subproject subprojects/googletest-1.17.0 found: YES 1.17.0`.
  - Plain route in the same image plus Arch's gcc 16.2.1, meson 1.12.1, cmake, pkgconf, flex and bison:
    `meson setup build` gives `Run-time dependency GTest found: YES 1.18.0`, and `unit_tests` links
    `/usr/lib/libgtest.so`. Build OK, `meson test --suite unit` 1/1 OK. The installed gtest is still preferred.
- **Local (no host gtest), fresh build directories:**
  - `pixi run test gcc`: 12/12 OK. `pixi run test clang`: 12/12 OK. `pixi run test asan-clang`: 12/12 OK, with no
    sanitizer message in the test log. `force_fallback_for` = `[systemc, args, gtest]` in all three.
  - `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: 240 passed, 0 failed/new/missing (also before the
    change).
  - `pixi run golden clang`: 240 passed at the default tolerances and with `--atol 0`. In strict mode, 7 fail; this
    happens before the fix as well (same binaries), see below.
  - Plain route (system gcc 11.5, meson 1.11.2, `/tmp` build dir): gtest is not installed here, so the wrap is used;
    `force_fallback_for` = `[systemc, args]`; `meson test` 12/12 OK.
- **`tests/sanitizers/lsan.supp`:** unchanged.

## Found, not done
- **AGENTS.md says clang release differs from the baseline in 6 cases under strict tolerances; it is now 7.** The
  cases are `cir/add_drop`, `opt/tol-resolution-overrides`, `opt/mode-fd`, `opt/multi-file`,
  `regress/b004-mode-td-dc`, `regress/f05-format-vcd-csv-dc` and `regress/f15-stats-dc`. All pass at the default
  tolerances and with `--atol 0`, and this change does not cause it (the binaries are identical). Most likely a case
  added since that note was written (e.g. `f15-stats-dc`) runs the same sweep point. The count in AGENTS.md (and
  wherever else it is quoted) needs updating.
- **Inside `pixi shell`, a plain `meson setup` can still pick up a host gtest.** It is documented in the README, not
  enforced. Making Meson itself refuse it would mean changing `meson.build` for every route.
- **An explicitly requested installed SystemC (`-Dforce_fallback_for=`) in a pixi build can find the host's
  `/usr/lib/pkgconfig/systemc.pc` the same way.** The ABI link check then fails, because `<systemc.h>` is not on the
  conda compilers' path, and Meson falls back to the wrap. The warning then names the wrong cause ("built with
  another C++ standard"), but the build is correct.
- **The prompt suspected CMake, but CMake is not involved in the gtest lookup.** It is pkg-config, and the
  maintainer's report fits that too.

## Final results (tip of `ws/pixigtest`)
- `pixi run test gcc` / `clang` / `asan-clang`: 12/12 OK each.
- Golden, gcc, strict (`--rtol 0 --atol 0 --print-slack 0`): **240 passed, 0 failed, 0 new, 0 missing, 0 skipped**.
- Arch container with a host gtest: `pixi run test gcc` and `pixi run test clang` 12/12 OK; plain route uses the
  host gtest and passes.
- Scratch files outside the repository: `/tmp/pi-repro` (scripts, logs, bundle), `/tmp/pi-plain`. The podman images
  `specs-pi-arch` and `specs-pi-arch-plain` were removed after the report.
