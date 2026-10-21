# PC2 report — SystemC as a Meson subproject, `thirdparty/` → `subprojects/`

Branch `ws/subprojects` (from `sprint/2026-10c`), 2026-10-21/22. Scope: "Decisions of 2026-10-21, third round"
in `backlog/DECISIONS-2026-10-21.md`. Not pushed. Golden: **no re-record**, every build bit-identical
(`--atol 0`).

## Items

| Step | Outcome | Commit |
|---|---|---|
| 1. Layout: `thirdparty/` → `subprojects/`, `subproject_dir` dropped, both submodules removed (gitlinks + `.gitmodules`; the shared `.git/modules` untouched), `.gitignore` Meson pattern | done | a13e765 |
| 2. Wraps `systemc.wrap` (tag 3.0.2) and `args.wrap` (tag 6.4.7): `[wrap-file]`, sha256, `method = cmake`, `[provide]` | done | a13e765 |
| 3. SystemC and args from the wraps by default; an installed copy only on request | done | 5c9b535 |
| 3b. SystemC subproject not sanitized (found by the ASan checks) | done | 65da3ff |
| 4. CI (GitHub, GitLab), `install.sh`, `utils/build-systemc.sh` removed | done | 1cddeee, 27afe84 |
| 5. Docs: README, CLAUDE.md, ONBOARDING, DECISIONS.md 34–38, CHANGELOG `[0.2.0]` | done | 5b62394 (+ 65da3ff, 27afe84 for DECISIONS) |

Tarball check (step 2): the former gitlinks were exactly the tags (`70b0fc8` = SystemC `3.0.2`, `114200a` =
args `6.4.7`). `diff -r` (excluding `.git`) between the tag checkouts and the extracted tarballs: **args is
identical**. **SystemC** differs only by files missing from the tarball, all matched by `.* export-ignore` in
SystemC's own `.gitattributes`: `.gitattributes`, `.github/`, `.gitmodules`, `.mailmap`,
`tests/systemc/bugs/stack_alignment/.notsparcOS5` (a regression-suite marker; the regression suite is not
built). No file present in both differs. GitHub has no release object with assets for SystemC 3.0.2, so the
wrap uses the tag archive URL.

## Decisions taken

1. **Opt-in mechanism: `force_fallback_for=systemc,args` in `default_options`**, read in `meson.build` with
   `get_option('wrap_mode')` / `get_option('force_fallback_for')` (both work on Meson 1.3.0 and 1.11.2):
   - default: the wraps; no lookup of an installed SystemC/args, even with `-Dcmake_prefix_path`;
   - `-Dforce_fallback_for=`: pkg-config, then CMake package, each with the ABI check; else the wrap;
   - `--wrap-mode=nofallback`: the same lookup, else an error (new message, names `-Dcmake_prefix_path`);
   - `--wrap-mode=forcefallback`: the wraps.
   **nofallback wins over the project's default `force_fallback_for`.** In Meson's own `dependency()`,
   `force_fallback_for` takes precedence, but Meson cannot tell a project default from a user value, and the
   prompt asks for `--wrap-mode=nofallback` to select an installed SystemC. Marked **[review]** in DECISIONS.md 35.
   `dependency()`'s automatic fallback is not used: the CMake subproject needs `cmake.subproject_options()`.
   The installed lookups pass `allow_fallback: false`, otherwise `nofallback` + the default
   `force_fallback_for` would make Meson configure the wrap through its `[provide]` entry.
2. **args through its CMake build, not its `meson.build`.** args 6.4.7 ships a `meson.build` providing
   `args_dep`, but it calls `install_headers('args.hxx')` unconditionally: `meson install` would start
   installing `<prefix>/include/args.hxx`. `cmake.subproject('args')` with `ARGS_BUILD_EXAMPLE/UNITTESTS=OFF`
   and `set_install(false)` gives an INTERFACE target: nothing built or installed (checked: the install tree
   holds only `specs`, `libspx.so`, `libsystemc.so*`). `[provide] args = args_dep` is still in the wrap
   (CMake subprojects expose `<target>_dep`).
3. **The SystemC subproject is never sanitized** (`set_override_option('b_sanitize', 'none')`). Sanitizer
   builds used to link an unsanitized, installed SystemC. Built from the wrap, it inherited
   `-fsanitize=address,undefined`, and UBSan's vptr check fires in SystemC's own process dispatch
   (`sc_process.h:633`, member-function pointer called on `sc_process_host`). With `meson test`'s
   `halt_on_error`, gcc ASan gave meson test 1/12 and golden 89/212. With the override, SPECS objects are
   sanitized as before and SystemC's are not (`compile_commands.json`: 0/86 vs 46/46). `lsan.supp` unchanged.
   Commits 5c9b535..64f6125 therefore fail sanitizer tests (non-sanitizer builds pass); keep this in mind if bisecting.
4. **CI ccache:** `CC`/`CXX` stay per matrix entry but are prefixed with `ccache` (`ccache gcc`,
   `ccache clang++`): Meson only adds ccache itself when they are unset. Caches: `subprojects/packagecache`
   (GitHub: key = hash of `subprojects/*.wrap`; GitLab: one fixed key `wraps-ccache-ubuntu24.04-gcc`, since
   `cache:key:files` takes at most two files and Meson checks the hashes anyway) and `.ccache`
   (`CCACHE_MAXSIZE=500M`). Full history for `git describe` (`fetch-depth: 0`, `GIT_DEPTH: "0"`).
5. **Artefacts:** `build/subprojects/systemc-*/libsystemc.so.[0-9]*[0-9]` (the simple `libsystemc.so*`
   also matched the `libsystemc.so.3.0.2.p/` object directory in a fresh clone; fixed in 27afe84).
6. **`.gitignore`:** `/subprojects/*`, `!/subprojects/*.wrap`, `!/subprojects/packagefiles/`; old
   `thirdparty/` lines and `.deps/` removed; `.ccache/` (CI compiler cache) added.
7. **Docs:** CLAUDE.md no longer has any machine path (deps from the wraps; gcc-sanitizer and `LSAN_OPTIONS`
   notes kept). DECISIONS.md: new entries 34–38, plus "superseded" markers on 1, 16, 17, 18, 30, 33. CHANGELOG:
   `[0.2.0]` Changed, Meson bullet (and `install.sh` wording under New). `discover/0*.md`, `STATUS.md`,
   `meson/PLAN.md`, `meson/VALIDATION.md`, `REBASE-2026-10.md` and `backlog/` are historical and were left
   unchanged; `discover/orchestration/` is untouched.

## Checks (final tree unless noted)

| Check | Result |
|---|---|
| Fresh gcc, no prefix (`meson setup`, ccache auto) | meson test 12/12; golden 212/212 `--atol 0`; `specs` → build-tree `subprojects/systemc-3.0.2/libsystemc.so.3.0` |
| Fresh clang (`CC="ccache clang" CXX="ccache clang++"`, as CI) | 12/12; 212/212 |
| Fresh gcc ASan+UBSan debug, `-Dnative=false` | 12/12, 0 sanitizer messages in the test log; 212/212 (with the `lsan.supp` `LSAN_OPTIONS`) |
| Fresh clang ASan+UBSan debug (`CC=clang CXX="ccache clang++"`) | 12/12, 0 sanitizer messages; 212/212 |
| Meson **1.3.0** (venv), default setup + build | setup without warnings; 12/12; 212/212 (at 5c9b535; the later `b_sanitize` override also configures on 1.3.0) |
| Fresh `git clone` of the branch (no submodules) + `./install.sh` | downloads systemc, args, gtest; 12/12 in 148 s; golden 212/212; `specs --version` = `v0.1-299.g65da3ff` (v0.2.0 not tagged yet) |
| Offline: `meson subprojects download`, then setup/compile/test with `--wrap-mode=nodownload` inside `unshare -rn` (no network) | setup OK, 0 downloads; 12/12 |
| Opt-in: `-Dforce_fallback_for= -Dcmake_prefix_path=<specs-deps-next>` | uses it: introspect `SystemCLanguage cmake 3.0.2`, `args cmake 6.4.7`, no systemc/args subproject dir, `ldd specs` → prefix `lib64/libsystemc.so.3.0`, installed RPATH = prefix `lib64`; 12/12; 212/212 |
| Opt-in: `--wrap-mode=nofallback` with/without the prefix | prefix used / new clear error |
| ABI check: SystemC 3.0.2 built as C++17 (`sc_api_version_3_0_2_cxx201703`) in `/tmp` | rejected with the existing warning; falls back to the wrap with `-Dforce_fallback_for=`, error with `nofallback` |
| `-Dspx=true` (CI gcc leg in the clone: `-Dnative=false -Dspx=true`, compile, test, `ninja doc`) | 12/12; `libspx.so` built; doc `index.html` built; artefact files present |
| `-Dtests=disabled` (GitLab doc job in the clone, then compile + test) | doc OK; gtest skipped; 11/11 |
| `meson install --destdir /tmp/PC2-inst` | `usr/local/bin/specs`, `usr/local/lib64/{libspx.so,libsystemc.so*}`; RPATH `/usr/local/lib64` on both |
| Install into a real prefix (`-Dprefix=/tmp/PC2-inst2`), run without `LD_LIBRARY_PATH` | `specs -f circuits/add_drop.cir` runs to DONE; `ldd` of `specs` and `libspx.so` → `/tmp/PC2-inst2/lib64/libsystemc.so.3.0` |
| CI YAML | both parse (PyYAML); steps walked through locally as above (GitHub gcc/clang/sanitizer legs, GitLab build/test/doc) |
| SystemC subproject build time | 28 s on 4 cores without ccache (rest of the build 92 s); fresh clone `install.sh` total 148 s |

## Found, not done

- `src/test/devices/detector.cpp:389`: `std::memcpy(values.data(), …, 0)` with an empty vector (null `data()`)
  when the forked scenario returns no bytes. UB only on that failure path (UBSan reported it while SystemC
  was sanitized), harmless otherwise.
- `.github/workflows/ci.yml`: the comment above the `sanitizers` job still says "Not gating yet" although
  the job is blocking (pre-existing).
- SystemC's tag archive is a GitHub-generated tarball; GitHub does not formally guarantee archive checksums.
  A `source_fallback_url` (e.g. a mirrored copy) would protect against a checksum change.
- Meson's cmake module prints "Unknown generator expression" warnings while configuring SystemC
  (pre-existing; the `-xnone` workaround already covers the one that matters).
- The args wrap's `method = cmake` means a plain `dependency('args')` fallback elsewhere would use args'
  CMake defaults (examples and unit tests ON); SPECS always passes its own options, so nothing is affected.
- Historical docs still describe submodules and `~/.local/opt/specs-deps`: `discover/0*.md`,
  `discover/STATUS.md`, `discover/meson/{PLAN,VALIDATION}.md`, `discover/REBASE-2026-10.md`.
- Local Meson runs on Python 3.9: Meson 1.12 will need Python ≥ 3.10 (already noted in DECISIONS.md 3).

## Final results

meson test **12/12** and golden **212/212 bit-identical (`--atol 0`, no re-record)** on fresh gcc, clang,
gcc ASan and clang ASan builds without any prefix, in a fresh clone, offline, and with the opt-in installed SystemC.
