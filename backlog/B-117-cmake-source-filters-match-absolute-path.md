# B-117: CMake source filters match the absolute path: a checkout under any `test/`, `tb/` or `parser/` directory builds nothing
severity: low | category: build | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: CMakeLists.txt:20-23, :26, :49
## Problem
`file(GLOB_RECURSE SOURCES_BIN "${SRC_PATH}/*.cpp")` returns absolute paths, then `list(FILTER ... EXCLUDE REGEX ".*/tb/.*")`, `".*/parser/.*"`, `".*/test/.*"` are applied to those absolute paths. If the repository lives anywhere below a directory called `test`, `tb` or `parser` (e.g. `/builds/test/specs`, `~/test/specs`), every source is filtered out and CMake fails with `No SOURCES given to target: spx`. Also `${SOURCES_TB}` on line 49 is never defined. obsoleted-by-meson: yes (explicit source lists); the CMake fix is only worth doing if the Meson merge slips.
## Evidence
```
$ cp -r CMakeLists.txt cmake src /tmp/ws-review-A7/test/specs/ ; cmake -S /tmp/ws-review-A7/test/specs -B .../b -G Ninja -DCMAKE_PREFIX_PATH=<deps> -DBUILD_TB=OFF
CMake Error at CMakeLists.txt:50 (add_library):
  No SOURCES given to target: spx
```
(`backlog/repro/a7-cmake-path-regex.sh`)
## Expected
Same 31 `specs` objects irrespective of where the checkout lives.
## Suggested fix
Use a source-relative glob so that the filters never see the checkout path (verified by the second check on
`test/specs`, `c++/specs`, `tb/parser/test/specs` and a plain path; the originally proposed anchored regex built from
`${SRC_PATH}` breaks on paths containing regex metacharacters such as `c++`):
```
CMakeLists.txt:20-23
-file(GLOB_RECURSE SOURCES_BIN CONFIGURE_DEPENDS "${SRC_PATH}/*.cpp")
-list(FILTER SOURCES_BIN EXCLUDE REGEX ".*/tb/.*")
-list(FILTER SOURCES_BIN EXCLUDE REGEX ".*/parser/.*")
-list(FILTER SOURCES_BIN EXCLUDE REGEX ".*/test/.*")
+file(GLOB_RECURSE SOURCES_BIN RELATIVE "${SRC_PATH}" CONFIGURE_DEPENDS "${SRC_PATH}/*.cpp")
+list(FILTER SOURCES_BIN EXCLUDE REGEX "^(tb|parser|test)/")
+list(TRANSFORM SOURCES_BIN PREPEND "${SRC_PATH}/")
CMakeLists.txt:49  add_executable(${PROJECT_NAME} ${SOURCES_BIN} ${SOURCES_TB})  ->  drop ${SOURCES_TB} (defined nowhere)
```
`list(TRANSFORM)` needs CMake >= 3.12 (the file requires 3.14). Line 26 (`main.cpp` filter) needs no change. Skip this
issue entirely if the Meson migration has already removed CMake.
## Golden impact
none.
## Related
B-121.

- Review ids: F-A7-14.
## Verification
Reproduced; with the anchored regexes the same scratch tree configures with 31 `specs` objects (equal to the control build).

**Independent second check (V2, of F-A7-14): ADJUST.** Method: reproduced the failure (checkout under `.../test/specs`: `No SOURCES given to target: spx`), applied the suggested anchored-regex fix, configured from `test/`, `tb/`, `parser/`, plain and `c++/` paths, counted objects with `ninja -t targets all`; then tried an alternative fix on the same matrix

The suggested fix works for `test/`, `tb/`, `parser/` (31 `specs` + 30 `spx` objects, identical to the control) but is NOT safe: regexes built from `${SRC_PATH}` break on checkouts whose path contains regex metacharacters. Verified regression: a checkout under `.../c++/specs` configures fine today but with the suggested fix fails with `CMake Error at CMakeLists.txt:21 (list): list sub-command FILTER, mode REGEX failed to compile regex "^/.../c++/specs/src/tb/"` (same for `(`, `[`, `+`). The finding only notes this in a parenthesis, so the fix as written is not unique/safe for unattended application. Replace the suggested fix text with the relative-glob variant, verified on `test/specs`, `c++/specs`, `tb/parser/test/specs` and plain (always 31 specs objects, same 30 spx, same object list as the control):
```
CMakeLists.txt:20-23
-file(GLOB_RECURSE SOURCES_BIN CONFIGURE_DEPENDS "${SRC_PATH}/*.cpp")
-list(FILTER SOURCES_BIN EXCLUDE REGEX ".*/tb/.*")
-list(FILTER SOURCES_BIN EXCLUDE REGEX ".*/parser/.*")
-list(FILTER SOURCES_BIN EXCLUDE REGEX ".*/test/.*")
+file(GLOB_RECURSE SOURCES_BIN RELATIVE "${SRC_PATH}" CONFIGURE_DEPENDS "${SRC_PATH}/*.cpp")
+list(FILTER SOURCES_BIN EXCLUDE REGEX "^(tb|parser|test)/")
+list(TRANSFORM SOURCES_BIN PREPEND "${SRC_PATH}/")
CMakeLists.txt:49  add_executable(${PROJECT_NAME} ${SOURCES_BIN} ${SOURCES_TB})  ->  drop ${SOURCES_TB} (not defined anywhere: grep)
```
(`list(TRANSFORM)` needs CMake >= 3.12, the file requires 3.14; tested with 3.26.5.) Line 26 `list(FILTER SOURCES_LIB EXCLUDE REGEX ".*/main.cpp")` still operates on absolute paths and needs no change (only the pre-existing, theoretical issue of a directory literally named `main.cpp...`). Golden impact: none (configure-only change, same source set). obsoleted-by-meson stays yes.

**Orchestrator:** Suggested fix replaced by the second checker's tested variant (relative glob).
