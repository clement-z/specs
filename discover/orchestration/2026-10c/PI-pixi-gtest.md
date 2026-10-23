# Phase I — pixi builds must not pick up the host's gtest (branch `ws/pixigtest`)

Follow `COMMON.md` (heartbeat, progress, commits, report, never push). Phase id `PI`. Small fix, maintainer report
of 2026-10-23.

## Report (Arch Linux, a fresh clone, `pixi run test`)
```
FAILED: src/test/unit_tests.p/devices_crossing.cpp.o
<env>/bin/ccache <env>/bin/g++ ... -isystem../../subprojects/systemc-3.0.2 ... -c ../../src/test/devices/crossing.cpp
../../src/test/devices/crossing.cpp:3:10: fatal error: gtest/gtest.h: No such file or directory
```
No gtest include path on the command line. Likely cause: `dependency('gtest')` (`src/test/meson.build:5`) finds the
host's gtest (Arch's `gtest` package: `/usr/include/gtest`, `/usr/lib/cmake/GTest`, `/usr/lib/pkgconfig/gtest.pc`),
probably through the environment's CMake, which also searches `/usr`. Meson drops `-I/usr/include` as a system
directory, but the conda compiler uses its own sysroot and does not search `/usr/include`. SystemC and args do
not have this problem because the project forces them to their wraps (`force_fallback_for=systemc,args`); gtest is not.
This host has no system gtest, so the failure does not show here.

## To do
1. Make pixi builds self-contained for gtest: the minimal, robust fix (e.g. `utils/pixi-setup.sh` passes
   `-Dforce_fallback_for=systemc,args,gtest`, so the WrapDB gtest is used; or gtest from conda-forge in
   `pixi.toml`). Choose, and say why. The plain route (system compilers) keeps preferring an installed gtest.
   Check whether any other dependency lookup in a pixi build can reach host packages the same way, and fix it the
   same way if so.
2. Verify:
   - reproduce the failure first if possible: a rootless container (`podman`, image `archlinux`, `pacman -S gtest`,
     pixi installed inside, the repository mounted read-only and cloned inside) is the best evidence; if containers
     cannot run here, simulate a host gtest visible to the environment's CMake (e.g. a gtest installed under a
     prefix found by CMake's default search) and show with `meson introspect --dependencies` which gtest is used
     before and after;
   - after the fix: `pixi run test gcc`, `pixi run test clang`, and the reproduction passes;
   - the plain route still finds an installed gtest (if one is available) or the wrap.
3. Docs: one line in the README pixi section or `discover/meson/DECISIONS.md`; CHANGELOG `[unreleased]` (Fixed).

Outputs and counters bit-identical (no simulation change).
