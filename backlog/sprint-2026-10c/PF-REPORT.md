# Phase F (pixi development environment) — report

Branch `ws/pixi` (from `sprint/2026-10c`), Opus 5.5 high + one Sonnet worker (docs draft, effort medium, diff
reviewed and corrected). Scope: "Decisions of 2026-10-22 (development environment)". Nothing pushed.

## Items

| Item | Outcome | Commit |
|---|---|---|
| 1. `pixi.toml`, `pixi.lock`, tasks, `utils/pixi-setup.sh`, `.gitignore`, `.gitattributes` | done | 3a1408f |
| 2. Docs: README section, CLAUDE.md (pixi route + B-183 note), CHANGELOG `[unreleased]` New, `discover/meson/DECISIONS.md` 39–45 | done | fc7b176 |
| 3. CI: `pixi` job in `.github/workflows/ci.yml` (Ubuntu gcc blocking, macos-latest clang `continue-on-error`) | done | d0bff37 |
| 4. Worktree / ccache measurements | done (below) | — |
| 5. macOS: expected breakage | listed under "Found, not done" | — |
| Golden bit-identity across compilers (found while checking) | B-183, `needs-human`; nothing re-recorded | 65c6da3 |

No source, build-file (`meson.build`) or baseline change. GitLab CI unchanged (its jobs are apt-based and nothing
in them depends on pixi; adding a pixi job there was not asked for).

## Decisions taken

- **Compiler packages.** Plain `gcc`/`gxx` 15 + `libsanitizer` 15 (Linux) and `clang`/`clangxx`/`compiler-rt` 21
  (all platforms). `cxx-compiler`/`gxx_linux-64`/`clang_linux-64` are not used: they install activation scripts
  that export `CFLAGS`/`CXXFLAGS`/`LDFLAGS` (checked in `gxx_linux-64` 15.3.0's `activate-gxx_linux-64.sh`:
  `-march=nocona -mtune=haswell -ftree-vectorize -fPIC -fstack-protector-strong -fno-plt -O2 -ffunction-sections -pipe
  -isystem <env>/include`), which Meson would pick up. The plain packages have no activation script (checked: only `libxml2`'s is
  installed). Majors = conda-forge's global pinning on 2026-10-21 (`c_compiler_version` 15 on Linux, 21 on macOS,
  read from `conda-forge-pinning` 2026.10.21). Locked: gcc 15.3.0, clang 21.1.8, meson 1.12.1, cmake 4.4.4,
  ninja 1.13.2, ccache 4.14.1, bison 3.8.2, flex 2.6.4, python 3.12.15, sysroot glibc 2.28 (pixi's default
  system requirement), doxygen 1.18.0, graphviz 14.1.2.
- **What the compilers inject by themselves** (their own configuration, not visible in `compile_commands.json`):
  gcc's `specs` file adds `-isystem <env>/include`, `-L <env>/lib/stubs -L <env>/lib`,
  `-rpath <env>/lib -rpath-link <env>/lib`, `-disable-new-dtags` (→ `DT_RPATH`); clang's config files
  (`x86_64-conda-linux-gnu-clang++.cfg`) add `--sysroot`, `-isystem <env>/include`, `-L`/`-rpath`/`-rpath-link
  <env>/bin/../lib`. No optimisation, `-march` or warning flags. `<env>/include` holds no header that SPECS or
  its subprojects include (only flex's own `FlexLexer.h` overlaps with anything build-related).
- **rpath: none added by SPECS.** Because of the above, `specs`, `unit_tests` and `libspx.so` already carry
  `DT_RPATH` = `$ORIGIN/...:<env>/lib` and resolve `libstdc++.so.6`/`libgcc_s.so.1` from the environment; no
  `LDFLAGS` in `[activation.env]` was needed. The path is absolute (gcc) or `<env>/bin/../lib` (clang): the
  binaries depend on that worktree's `.pixi/` (README says so).
- **`CC`/`CXX` in `[target.linux.activation.env]` = `ccache gcc` / `ccache g++`** (macOS: `ccache clang[++]`).
  Cause: in the environment `cc` → gcc but `c++` → clang-21 (both packages ship a `c++`), so a plain
  `meson setup` in `pixi shell` would mix gcc for C and clang for C++. Naming `ccache` keeps it in use (Meson only
  adds it when `CC`/`CXX` are unset). The tasks override both with absolute paths per toolchain.
- **Build directories: `build-pixi/<toolchain>`** (`gcc`, `clang`, `asan-gcc`, `asan-clang`), never `build/`
  (ignored: `/build-pixi/`, `/.pixi/`). `utils/pixi-setup.sh` creates them (`CC="ccache <env>/bin/gcc"` …;
  asan: `-Dbuildtype=debug -Db_sanitize=address,undefined -Db_lundef=false -Dnative=false`) and refuses an
  existing directory whose C++ compiler (`meson-info/intro-compilers.json`) is not this environment's: checked
  with a `build-pixi/gcc` configured by system meson/gcc → message + exit 1, and `pixi run build` stops too.
- **Tasks** take the toolchain as a pixi task argument (default `gcc`): `setup`, `build`, `test`
  (`meson test --print-errorlogs`, includes the golden suite), `golden` (the CLAUDE.md `run_golden.py check`
  line, `-j 8`, with `LSAN_OPTIONS` = the suppressions so it also works on asan builds; extra options after `--`,
  e.g. `pixi run golden gcc -- --atol 0`); `docs` in the `docs` environment (`utils/doc.sh` → `build-pixi/doc/html`,
  no Meson configuration needed). Extra Meson options at creation: `pixi run setup gcc -- -Dspx=true`.
- **Environment variables** (`[activation.env]`; pixi expands `$VAR` and `${VAR:-default}` like the shell, checked):
  `CCACHE_BASEDIR=$PIXI_PROJECT_ROOT`, `CCACHE_NOHASHDIR=1`, ccache's default cache directory
  (`~/.cache/ccache`, shared with the system ccache 4.5); `MESON_PACKAGE_CACHE_DIR=${XDG_CACHE_HOME:-$HOME/.cache}/specs/meson-packagecache`
  (honoured from Meson 1.3.0: absent from the 1.2.3 sources, present in 1.3.0; downloads use a temporary file and
  a rename, so concurrent worktrees are safe; checked: the four tarballs land there and `subprojects/packagecache/`
  stays empty).
- **Environments:** `py` (python 3.12, numpy, pandas, `matplotlib-base`, jupyterlab; pyDigitalWaveTools from PyPI)
  and `docs` (doxygen, graphviz) use `no-default-feature = true`: no second toolchain. `matplotlib-base` instead of
  `matplotlib`: same library without the Qt backend (Tk and Jupyter backends work); comment in `pixi.toml` says the
  list must agree with `pyspecs/requirements.txt`.
- **pixi's post-link scripts stay disabled** (pixi default): it skips graphviz/gtk3/gdk-pixbuf scripts; the graphviz
  package already ships its plugin `config8` and Doxygen's PNG graphs render (370 graphs).
- **CI:** `prefix-dev/setup-pixi@v0.11.2` (latest, 2026-10-14), `pixi-version: v0.82.1`, `environments: default`,
  `locked: true`, `cache: true` (keyed on `pixi.lock`); wrap cache `~/.cache/specs/meson-packagecache`, ccache in
  `.ccache` (workflow `CCACHE_DIR`); `setup <tc> -- -Dnative=false` like the apt jobs. Not run on GitHub (no push).

## Measurements (this machine: 4 cores, XFS with reflinks, 2026-10-22)

| | result |
|---|---|
| Fresh `pixi install` (default env, empty package cache) | 221 s wall; 468 MiB downloaded (sum of the lock's linux-64 package sizes: 91 packages; `py` 165 MiB, `docs` 86 MiB more); package cache 2.06 GB extracted; `.pixi/` 2.0 GB apparent; disk used +2.47 GB in total |
| Second worktree's `pixi install` (warm cache) | 1.3 s; +198 MiB disk. Files are **reflinked** (XFS `shared` extents, link count 1), not hard-linked; the extra space is the files pixi rewrites for the prefix. All three environments: default 2.0 GB, py 744 MB, docs 368 MB apparent |
| First worktree build (`pixi run build`, empty ccache) | 173 s (SystemC + SPECS + gtest, 198 compilations, 0 hits) |
| Second worktree build, default `-march=native` | 125 s, **48 % hits** (95/198: all SystemC and gtest objects; every SPECS object misses) |
| Second worktree build, `-Dnative=false` | **2 s, 99.5 % hits** (191/192; the miss is SystemC's `sc_ver.cpp`, which uses `__DATE__`/`__TIME__`) |
| Full `pixi run test` from scratch (gcc / clang) | 178 s / 145 s |

Cause of the native misses (ccache log): ccache 4.14 resolves `-march=native` by running
`g++ -### -E - -march=native` and hashes its output, which contains the worktree's absolute paths (`cc1plus`,
`-isystem <env>/include`): base_dir cannot rewrite those. Paths on the command line are rewritten correctly.

**Proposed Paseo setup step** (not committed; for the maintainer to decide), `paseo.json` at the repo root:
```json
{ "worktree": { "setup": "command -v pixi >/dev/null 2>&1 && pixi install --locked || echo 'pixi not on PATH: skipped'" } }
```
(`worktree.setup` takes a string or a list, per Paseo's config schema; the daemon's PATH must contain pixi.)
Agents in worktrees that only need tests could add `pixi run setup gcc -- -Dnative=false` to share ccache fully.

## Final results (fresh clone of `ws/pixi` at 1d5ba05, `/tmp/PF-final`)

| Build | `meson test` | sanitizer messages | golden default | golden `--atol 0` |
|---|---|---|---|---|
| pixi gcc 15 (`build-pixi/gcc`, release, native) | 12/12 | — | 212/212 | 212/212 |
| pixi clang 21 (`build-pixi/clang`) | 12/12 | — | 212/212 | 212/212 |
| pixi gcc ASan+UBSan (debug) | 12/12 | 0 | 212/212 | 212/212 |
| pixi clang ASan+UBSan (debug) | 12/12 | 0 | 212/212 | 212/212 |
| CI walk-through (other fresh clone: `setup gcc -- -Dnative=false`, build, test, golden) | 12/12 | — | 212/212 | — |
| plain route, system gcc 11.5 + system meson 1.11.2 (`/tmp/PF-plain`, no pixi) | 12/12 | — | — | 212/212; strict 212/212 |

- **Bit-identity (strict `--rtol 0 --atol 0 --print-slack 0`)**: only builds like the baseline's are bit-identical.
  pixi gcc 15 native: 5 cases differ (`regress/b031-mzi-bidir`, `tb/ac_add_drop`, `tb/ac_add_drop-fd`, `tb/ac_crow`,
  `regress/b131-nrings-crow`), last 1–2 digits, all pass at `--rtol 1e-13`; pixi clang 21: 23 cases, pass at
  `--rtol 1e-12`, **identical to system clang 19**; system gcc 11 and pixi gcc 15 with `-Dnative=false`: the same
  28 cases, identical to each other. So the difference follows FMA contraction (`gnu++20` → `-ffp-contract=fast`
  with `-march=native`), not pixi. Not re-recorded; recorded as **B-183 (`needs-human`)** with options.
- **Flags**: `compile_commands.json` of `build-pixi/gcc` vs the plain build, all 192 compilations: identical flags
  (`-O3`, `-march=native`, `-std=gnu++20`, `-D_GLIBCXX_ASSERTIONS=1`, warnings, defines; buildtype release,
  `b_ndebug=false`) except source-relative paths and one extra **`-pthread`** on SystemC's objects and link (its
  CMake `FindThreads`: the environment's sysroot is glibc 2.28, where pthreads is not in libc yet). ccache is the
  first word of every command.
- **ldd/readelf**: `specs`, `unit_tests`, `libspx.so` (gcc) and `specs` (clang) have `DT_RPATH` with the
  environment's `lib/` and resolve `libstdc++.so.6`, `libgcc_s.so.1` there (`libsystemc.so.3.0` from the build
  tree); `specs` runs directly from a plain shell (outside `pixi run`).
- **py**: `import numpy, pandas, matplotlib, pyDigitalWaveTools` OK (numpy 2.5.4, pandas 3.0.6, matplotlib 3.11.2);
  `pyspecs.specsparse.parse_vcd_dataframe` parses the trace of `circuits/add_drop.cir` written by the pixi-built
  `specs` (FD, 31×31). **docs**: `pixi run -e docs docs` → `build-pixi/doc/html/index.html`.
- **Lock**: `pixi install --locked --all` and `pixi lock --check` OK; the lock solves for linux-64, linux-aarch64,
  osx-64, osx-arm64 (only linux-64 installed and run). Both CI YAML files parse.
- Warnings: same counts as the system compilers (gcc: the 8 known `-Wstrict-aliasing` in `src/specs.cpp`;
  clang: 443, mostly `-Wdeprecated-declarations`, as system clang 19).

## Found, not done

- **B-183** (above): what "bit-identical" means across compilers/flags; maintainer decision.
- **ccache and `-march=native`**: no cross-worktree sharing of SPECS objects with native builds (ccache hashes the
  absolute paths in `-###` output). Options: `-Dnative=false` for agent worktrees, a fixed `-march=<cpu>`, or an
  upstream ccache report. Not changed (native is the project default).
- **Doxygen 1.18** warns about 12 obsolete `doc/Doxyfile` tags (93 warnings in total); `doxygen -u` not run.
- Binaries built in the environment break if the worktree is moved or `.pixi/` removed (absolute rpath); the
  README says they depend on the environment.
- `pixi.lock` is ~13 600 lines; every `pixi update` gives a large diff (marked generated, `merge=binary`).
- `macos-latest` is a floating label; pin it when the macOS port starts.
- **macOS (expected to break, from reading the code; nothing tried):**
  - `src/test/parser_state.cpp:44` lists `/proc/self/fd` (no `/proc` on macOS; `/dev/fd` exists).
  - `meson.build`: the installed-SystemC lookup tests `libsystemc.so` (macOS: `.dylib`); the SystemC subproject's
    `b_lundef=false` must become `-undefined dynamic_lookup` for `libsystemc` to link against the user's `sc_main`
    (Meson does that for `b_lundef`, to be checked); `-xnone` C argument and QuickThreads on arm64 (SystemC 3.0.2's
    CMake picks the QT/pthreads backend per architecture) unverified.
  - CI artefact patterns and the README name `libsystemc.so.*`; install rpath/`install_name` (`@rpath`,
    `@loader_path` instead of `$ORIGIN`) untested.
  - The plain conda `clang` on macOS has no `clang_osx-*` activation: no `CONDA_BUILD_SYSROOT`/`-isysroot`, no
    `ld64`/`cctools` from conda (relies on Xcode's SDK and linker), and its libc++ may differ from the one in the
    environment; expect to add the SDK path or `ld64` packages.
  - No gcc in the macOS environment: `pixi run test` (default `gcc`) fails with a clear message; the CI job uses
    `clang`. LeakSanitizer is not supported by ASan on macOS arm64, so `tests/sanitizers/lsan.supp` does not apply
    and the asan tasks need `ASAN_OPTIONS=detect_leaks=0` or similar.
  - `fork`-based tests (`SPECS_TEST_HEADER`, death tests) should work on macOS but are untested; Windows is out of
    scope (decision).
  - `-march=native` on Apple arm64 with clang 21 is accepted (LLVM ≥ 15), untested; `ldd`/`readelf` checks become
    `otool -L`/`otool -l`.

## Bookkeeping

Progress file `backlog/sprint-2026-10c/PF-progress.md`; heartbeat `45fdd6ee` deleted after this report. Scratch:
`/tmp/PF-*` (clones, build dirs, the cold package cache `/tmp/PF-cold`), removable. Files outside the repo changed:
`~/.cache/rattler` (pixi package cache, 2.1 GB), `~/.cache/specs/meson-packagecache` (5.5 MB of wrap tarballs),
`~/.cache/ccache` (objects).
