# Phase F — pixi development environment (after v0.2.0)

Read `discover/orchestration/2026-10c/COMMON.md` first and follow it (heartbeat, progress file, commits, report,
never push). Its "Build and test" section predates phase C2: SystemC and args now come from `subprojects/*.wrap`
and no `-Dcmake_prefix_path` is needed. Scope: "Decisions of 2026-10-22 (development environment)" in
`backlog/DECISIONS-2026-10-21.md`; the background and the trade-offs the maintainer accepted are summarised below.

Branch `ws/pixi` (from `sprint/2026-10c`). Phase id `PF`: progress file `backlog/sprint-2026-10c/PF-progress.md`,
done marker `backlog/sprint-2026-10c/PF-REPORT.md`. You are Opus at high effort. Use Sonnet subagents (effort
medium, stated explicitly) for small, well-specified tasks (README/CLAUDE.md wording, CI YAML edits, checks you
describe precisely); review every diff they produce yourself.

pixi 0.82.1 is installed in `~/.pixi/bin/pixi` but is **not on the PATH** of non-interactive shells: call it by
that path or prepend `~/.pixi/bin` to `PATH` in your commands. Do not install or upgrade pixi itself, and do not
change files in the maintainer's home outside `~/.cache` (pixi/rattler caches, ccache) and your own `/tmp/PF-*`.

## Goal
One optional, pinned development environment that works the same on any Linux machine and (later) on macOS, for
humans and for agents working in many git worktrees. The plain Meson route (system compilers, `meson setup build`)
stays documented and supported; CI keeps its apt jobs.

## To do
1. **`pixi.toml`** (workspace manifest at the repo root; `pixi.lock` committed; `.pixi/` in `.gitignore`; lock file
   marked generated in `.gitattributes`).
   - Channels: conda-forge only. Platforms: linux-64, linux-aarch64, osx-64, osx-arm64 (the lock must solve for
     all four; only linux-64 can be run here).
   - Default environment: a C/C++ toolchain (gcc/g++ on Linux, clang on macOS) **and** clang/clang++ on Linux,
     with the sanitizer runtimes both need; meson (>= 1.3; current is fine), ninja, cmake, pkg-config, flex,
     bison (>= 3), python, ccache, gdb (Linux) / lldb (macOS), clang-format.
   - **Compiler packages:** check what conda-forge's `cxx-compiler` / `gxx_linux-64` / `clang_linux-64`
     activation exports (CC, CXX, CFLAGS, CXXFLAGS, LDFLAGS, ...). Meson picks those up at setup: they must not
     change SPECS's optimisation, `-march`, or disable Meson's automatic ccache. Prefer packages without
     activation flags (e.g. plain `gcc`/`gxx`/`clang`/`clangxx`) or neutralise them, and say which in the report.
   - **Runtime libraries:** binaries built with conda-forge compilers need the environment's `libstdc++` (newer
     than the system's). Make `build/specs`, `unit_tests` and `libspx.so` built in the environment find it (rpath
     to the environment's `lib/`, e.g. via `LDFLAGS` in `[activation.env]`, or the closest clean mechanism) and
     check with `ldd`/`readelf -d`. Note in the README that such binaries depend on the environment.
   - `[activation.env]`: share ccache hits across worktrees (`CCACHE_BASEDIR`, `CCACHE_NOHASHDIR=1`; check
     how pixi expands variables in activation values) and one wrap download cache for all worktrees (verify that
     Meson honours `MESON_PACKAGE_CACHE_DIR`, from which version, and point it to a directory under `~/.cache`).
   - Features/environments: `py` (pyspecs: numpy, pandas, matplotlib, jupyter/jupyterlab, and pyDigitalWaveTools
     from PyPI; keep `pyspecs/requirements.txt` for pip users and say in a comment that the two lists must agree),
     `docs` (doxygen, graphviz). Keep solve groups or separate environments sensible (no duplicated toolchains).
   - Tasks: setup, build, test (all `meson test`), golden (the `run_golden.py check` line of `CLAUDE.md`),
     sanitizer setup/test (gcc and clang ASan+UBSan debug builds, with the LSan suppressions), docs. **Build
     directories must not mix toolchains**: a build directory remembers the compiler it was configured with, so
     pixi tasks must not silently reuse a `build/` configured outside the environment (dedicated directory names,
     or a check that fails clearly). Choose, explain in the README.
2. **Docs.** README: a short "Development environment (pixi, optional)" section (install pixi, `pixi run test`,
   `pixi shell`, the `py` and `docs` environments, binaries depend on the environment, plain route unchanged).
   `CLAUDE.md`: how agents build and test through pixi (commands prefixed with `pixi run`, or the tasks), keeping
   the plain route; no machine-specific paths (pixi's own install location is a per-user detail: say "pixi on the
   PATH"). `CHANGELOG.md` `[unreleased]` → New. `discover/meson/DECISIONS.md`: a short entry.
3. **CI.** `.github/workflows/ci.yml`: a pixi job (`prefix-dev/setup-pixi`, cached on `pixi.lock`, `pixi run`
   setup/build/test/golden) on ubuntu-24.04 (blocking) and macos (latest arm64 runner, `continue-on-error: true`,
   comment: macOS port not done yet). Keep the existing jobs unchanged. Validate the YAML. GitLab CI: no change
   unless trivial; say so.
4. **Worktrees and agents.** Measure on this machine: fresh `pixi install` (time, download, disk of `.pixi/` and
   of the shared cache), a second worktree's `pixi install` (time, extra disk: hard links?), and a second
   worktree's build with ccache (hit rate, wall time) versus the first. Paseo can run a setup step when it creates a
   worktree (`paseo.json`, see `paseo workspace setup --help` and paseo's docs): **do not commit a `paseo.json`**;
   put a proposed snippet (e.g. `pixi install`) in the report for the maintainer to decide.
5. **macOS (not runnable here).** Do not attempt the port. List in the report, under "Found, not done", what you
   expect to break on macOS from reading the code and build files (e.g. `/proc/self/fd` in a unit test,
   `libsystemc.so` names in `meson.build`, rpath/install_name, SystemC's build flags, `fork`-based tests).

## Checks (all must pass; report the numbers)
- In a fresh clone of your branch in `/tmp/PF-*`: `pixi run` your setup/build/test/golden tasks with the default
  (gcc) toolchain and with clang: `meson test` 12/12, golden **212/212 at `--atol 0`, bit-identical, no
  re-record**. A difference is a compiler-dependent result: do not re-record; stop, record what differs
  (cases, sizes of the differences) and report it (`needs-human`).
- Sanitizer builds inside pixi (gcc and clang, ASan+UBSan, debug): `meson test` 12/12 with no sanitizer message;
  golden 212/212 with `LSAN_OPTIONS` as in `CLAUDE.md`.
- Flags: `compile_commands.json` of a pixi build has SPECS's usual flags (buildtype, `-march=native` unless
  `-Dnative=false`) and no flags injected by the environment, except the rpath you add; ccache is used.
- `ldd`/`readelf -d` of `specs`, `unit_tests`, `libspx.so` (`-Dspx=true`): the environment's `libstdc++`; the
  binaries run directly (not only through `pixi run`) while the environment exists.
- The plain route outside pixi is unaffected: a fresh system-gcc build of your branch passes `meson test` and golden.
- `pixi run -e py`: `python -c 'import numpy, pandas, matplotlib, pyDigitalWaveTools'` and the pyspecs parser on a
  trace produced by the built `specs` (e.g. `circuits/add_drop.cir`). `pixi run -e docs` (or the docs task):
  Doxygen HTML builds.
- `pixi.lock` solves for all four platforms (`pixi lock` / `pixi install --locked` consistency).
- Both CI YAML files parse; walk through the new job's steps locally with the same `pixi run` commands.

## Commits
Small steps (`build: pixi environment (...)`, `doc: ...`, `ci: pixi job ...`), each with cause, change and checks
in the body; the tree builds at every commit. Then `PF-REPORT.md`: decisions taken (compiler packages, rpath,
build directory names, env variables), measurements (step 4), "Found, not done" (incl. macOS), final results.
