# Phase C2 — SystemC as a Meson subproject, `thirdparty/` → `subprojects/` (v0.2.0, before the tag)

Read `discover/orchestration/2026-10c/COMMON.md` first and follow it, with two differences: the "Build and test"
commands there use a prebuilt prefix (`-Dcmake_prefix_path=...specs-deps-next`); in this phase, test the new default
(no prefix) and use the prefix only for the opt-in check below. There are no backlog items: the scope is the
maintainer's decision "Decisions of 2026-10-21, third round" in `backlog/DECISIONS-2026-10-21.md`.

Branch `ws/subprojects` (from `sprint/2026-10c`). Phase id for files and heartbeat: `PC2`. Progress file
`backlog/sprint-2026-10c/PC2-progress.md`, done marker `backlog/sprint-2026-10c/PC2-REPORT.md`. **Never `git push`.**
Model: you are Opus at high effort; subagents per COMMON.md (explicit model and effort).

## Today
- `meson.build` sets `subproject_dir: 'thirdparty'`. SystemC is looked up via pkg-config, then its CMake package
  (`-Dcmake_prefix_path`), each with a C++-standard ABI link check, and only then built from the `thirdparty/systemc`
  git submodule with `cmake.subproject()` (options: C++20, shared, no examples, no copyright banner, `-xnone` for C,
  `b_lundef=false`, installed next to `specs`). taywee/args: CMake package, else `-isystem thirdparty/args`
  (submodule). googletest: `thirdparty/gtest.wrap` (WrapDB).
- CI (`.github/workflows/ci.yml`, `.gitlab-ci.yml`) and `install.sh` build SystemC separately with
  `utils/build-systemc.sh` into a prefix (cached in CI) and pass `-Dcmake_prefix_path`.

## To do
1. **Layout.** Rename `thirdparty/` to `subprojects/` (Meson's default; drop `subproject_dir`). Remove the two git
   submodules (`.gitmodules` entries and gitlinks; delete `.gitmodules` if empty). Do not touch `.git/modules` of the
   shared repository.
2. **Wraps.** `subprojects/systemc.wrap` (`[wrap-file]`, the GitHub release tarball of tag 3.0.2 of
   accellera-official/systemc, `source_hash` sha256, `method = cmake` if your Meson minimum allows it; the project
   requires Meson >= 1.3.0) and `subprojects/args.wrap` (tag 6.4.7 of Taywee/args; it ships its own `meson.build`:
   check what it provides and add a `[provide]` section). **Verify that each tarball's tree is identical to the
   submodule checkout it replaces** (`diff -r`, excluding `.git`): SystemC and args must not change. Put downloads in
   a fresh `/tmp/PC2-*` directory; they are third-party data.
3. **SystemC by default from the subproject.** A plain `meson setup build` builds SystemC from the wrap with the
   existing `cmake.subproject()` options (keep them and their comments). An installed SystemC is used only on explicit
   request through Meson's standard mechanism: e.g. `force_fallback_for=systemc` in the project's `default_options`,
   so `-Dforce_fallback_for=` (or `--wrap-mode=nofallback`) selects an installed SystemC, still via pkg-config / CMake
   package with the ABI check (and `wrap_mode=nofallback` + no usable installed SystemC is a clear error). Verify on
   Meson 1.3 semantics (docs) and locally (`~/.local/bin/meson` 1.11) that this works; if Meson does not allow it,
   use the closest standard mechanism, or a project option as a last resort, and record the choice in the report and
   in `discover/meson/DECISIONS.md`. args: also from its wrap by default (header-only, same rule).
   Keep the rpath handling so an installed `specs` (and `libspx.so`) finds `libsystemc.so` in both modes.
4. **CI and scripts.** GitHub and GitLab CI: no `build-systemc.sh`, no SystemC prefix or prefix cache; keep `CC`/`CXX`
   per matrix entry (SystemC compiles C and QuickThreads assembly with `CC`); keep enough git history for
   `git describe` (the version comes from tags); no `submodules: recursive` needed. Cache the wrap downloads
   (`subprojects/packagecache`, keyed on the `.wrap` files) and keep CI time reasonable (ccache if simple; report the
   SystemC subproject build time locally). Fix the artifact step that ships `libsystemc.so*` from the old prefix.
   `install.sh`: plain setup/compile/test. Remove `utils/build-systemc.sh` unless something still needs it.
   `.gitignore`: the usual Meson pattern (ignore `subprojects/*` except `*.wrap` and `packagefiles/`), old
   `thirdparty/` lines removed.
5. **Docs.** `README.md` (build section: no submodules; offline `meson subprojects download`; how to use an installed
   SystemC), `CLAUDE.md` (no machine-specific paths at all: deps come from the wraps; keep the gcc-sanitizer and
   `LSAN_OPTIONS` notes), `meson.options` descriptions, `discover/ONBOARDING.md` and `doc/*.md` where they describe
   the build, `discover/meson/DECISIONS.md` (new entry superseding the old lookup order), and `CHANGELOG.md`
   **`[0.2.0]` section** (not `[unreleased]`: this lands before the tag), under Changed, in the Meson bullet. Do not
   rewrite historical reports and plans in `backlog/` or `discover/` (sprint reports, progress files, old plans).
   Leave `discover/orchestration/` to the coordinator.

## Checks (all must pass; report the numbers)
- Fresh gcc, clang, gcc ASan and clang ASan builds **without any prefix**, as in COMMON.md otherwise:
  `meson compile`, `meson test` 12/12 with no sanitizer message, and the golden check at `--atol 0`: **212/212,
  bit-identical, no re-record at all** (for sanitizer builds set
  `LSAN_OPTIONS=suppressions=$PWD/tests/sanitizers/lsan.supp:print_suppressions=0` when calling `run_golden.py`
  directly). Any output change means something is wrong: stop and report it.
- A fresh `git clone` of your branch into `/tmp` (no submodule init): setup downloads the wraps, builds, tests pass.
  Offline: `meson subprojects download`, then `--wrap-mode=nodownload` setup works.
- Opt-in installed SystemC: `-Dforce_fallback_for=` (or your mechanism) with
  `-Dcmake_prefix_path=$HOME/.local/opt/specs-deps-next` uses it (say how you checked) and passes golden; a SystemC
  built with another C++ standard is still rejected by the ABI check with the existing warning.
- `-Dspx=true` (libspx.so), `-Dtests=disabled`, and `meson install --destdir /tmp/PC2-inst` followed by running the
  installed `specs` on `circuits/add_drop.cir` (it must find `libsystemc.so` without `LD_LIBRARY_PATH`).
- Validate both CI YAML files (syntax) and walk through their steps locally as far as possible; the coordinator
  pushes after the gate, on the maintainer's request.

## Commits
Small steps, e.g. `build: rename thirdparty/ to subprojects/, wraps for systemc and args`, `build: systemc from the
subproject by default`, `ci: build systemc as a subproject`, `doc: ...`. Each with cause, change and checks in the
body; the tree must build at every commit. Then `PC2-REPORT.md` (decisions taken, "Found, not done", final results).
