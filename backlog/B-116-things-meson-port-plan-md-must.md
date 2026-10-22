# B-116: Things the Meson port (PLAN.md) must not forget
severity: low | category: build | effort: M | fix-class: needs-design | verdict: PLAUSIBLE
location: discover/meson/PLAN.md, discover/04-build-test-ci.md §9
## Problem
Items that exist in today's build/CI/gitignore, survive the migration, and are absent or underspecified in PLAN.md. Each was checked against the current tree unless stated.
1. SystemC include dirs must be `-isystem`: with `-I` GCC 11 prints 23 `sc_has_process_used is deprecated` warnings for ONE translation unit (`parse_analysis.cpp`), clang 344 in total. discover/04 §9.6 says to use `include_type: 'system'`; PLAN.md §2 does not carry it over. Alternative: define `SC_ALLOW_DEPRECATED_IEEE_API`, or remove the 36 `SC_HAS_PROCESS` uses (devices reviewers).
2. flex and bison must be `required: true` with versions: today a missing bison gives `Unknown CMake command "bison_target"` (verified by forcing `BISON_EXECUTABLE-NOTFOUND`). parser.y uses `%define api.pure full` / `%param` (bison >= 3.0, no `%require` in the file; 3.7.4 was used) and parser.l uses reentrant `yylex_init_extra`/`bison-bridge` (flex 2.6.4 was used); use `find_program('bison', version: '>=3.0')` and `find_program('flex', ...)`, optionally add `%require "3.0"` to parser.y.
3. The golden/CI jobs need `python3` (the harness is run with `python3 -I`); the current Alpine `before_script` installs none. Apply B-108 to the tolerance used with `-Dnative=false`.
4. Any `test()` that runs `specs -t <name>` needs `workdir:` with an existing `traces/` and `circuits/`-relative `.include` paths; the golden harness handles it itself (temp dirs), the tb smoke tests do not (B-007).
5. `.gitignore`: besides `subprojects/packagecache` and wrap checkouts (PLAN.md:126), apply B-114 and note that `build/` is the only build-dir pattern today.
6. Meson auto-generates a `clang-format` target whenever `.clang-format` exists at the source root (it does): `ninja clang-format` would bulk-reformat 104/108 non-compliant files, the exact thing CLAUDE.md forbids; mention it next to the dropped `make format` (PLAN.md:93).
7. `-Wfatal-errors`: B-115. `b_ndebug=false` (PLAN.md:32-35) is fine: no `assert` in `src/` (63 uses outside `src/test`) has a side effect (grep for `++`, `=`, `push/pop/insert/erase/write/read` inside assert: none), and the baseline passes on CMake Debug. But with asserts on, the `-Db_ndebug=true` variant reintroduces B-112.
8. `.gitmodules`: the first section is named `deps/args` but its path is `thirdparty/args` (harmless, but `git submodule` names and `.git/modules` diverge; renaming needs `git submodule sync` on existing clones). SystemC is pinned to the `3.0.1` tag (11ad094, verified with `git ls-remote`); keep it so CI's cache key and `thirdparty/systemc` stay in sync with CLAUDE.md.
## Evidence
See the individual items; item 1: `g++ -fsyntax-only -std=gnu++20 -Wall -Wextra -I<deps>/include ... parse_analysis.cpp` -> 23 warnings, with `-isystem` -> 0; item 2: configure with `-DBISON_EXECUTABLE=BISON_EXECUTABLE-NOTFOUND` -> `CMake Error at src/parser/CMakeLists.txt:16 (bison_target): Unknown CMake command "bison_target".`
## Expected
PLAN.md/meson.build cover them (or document why not).
## Suggested fix
Hand this list to the ws/meson implementer; no repo change needed from the backlog.
## Golden impact
none.
## Related
B-108, B-114, B-007, B-115.

- Discovery seeds: S4.11, S5.10 (see `backlog/README.md` for seed numbering).
- Review ids: F-A7-13.
## Verification
Items 1, 2, 6 (by Meson docs, not run), 7, 8 verified as stated; the Meson behaviour itself (items 1, 6) not run (Meson not exercised in this review).
