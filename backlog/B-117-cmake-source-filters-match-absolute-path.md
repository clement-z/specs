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
```
CMakeLists.txt:21-23  list(FILTER SOURCES_BIN EXCLUDE REGEX "^${SRC_PATH}/tb/")   (and /parser/, /test/)
```
(`SRC_PATH` containing regex metacharacters such as `+` would need quoting.) Delete the stray `${SOURCES_TB}` on line 49.
## Golden impact
none.
## Related
B-121.

- Review ids: F-A7-14.
## Verification
Reproduced; with the anchored regexes the same scratch tree configures with 31 `specs` objects (equal to the control build).
