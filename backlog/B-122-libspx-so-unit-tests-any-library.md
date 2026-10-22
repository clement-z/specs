# B-122: libspx (and so unit tests and any library user) cannot load a netlist: parser and scanner are not in it
severity: low | category: build | effort: M | fix-class: needs-design | verdict: CONFIRMED
location: CMakeLists.txt:22,25-26, src/parser/CMakeLists.txt:31, src/main.cpp:84-89,146-166, Makefile:26
## Problem
obsoleted-by-meson: partly. PLAN.md:69-70 says "mirror the CMake contents", so the limitation is carried over on purpose. The `spx` library is `src/*.cpp` minus `main.cpp`, `parser/`, `tb/`, `test/`; the lexer/parser and the call to `yyparse()` live in `specs` (parser objects are linked into the executable only, `yylex_init_extra`/`yyparse` are called from `main.cpp`). A gtest or embedding user can only build circuits programmatically; `pyspecs` works through the CLI. Every source is also compiled twice (once for `specs`, once with `-fPIC` for `spx`).
## Evidence
`CMakeLists.txt:22 list(FILTER SOURCES_BIN EXCLUDE REGEX ".*/parser/.*")`; `src/parser/CMakeLists.txt:31 target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_parser)` (only `specs`); `main.cpp:84-89` calls `yylex_init_extra`/`yyparse`.
## Expected
A decision: either document that libspx is "devices + engine only", or move the parse entry points out of `main.cpp` into a `parse_netlist()` in the parser library and build `spx` with it (then `specs` = main + spx, which also removes the double compilation, subject to the `-fPIC` numerics check in discover/04 §9.3).
## Suggested fix
Design decision for the maintainer; defer until after the Meson merge (the layout then changes in one place).
## Golden impact
none expected; a shared-object build of the engine must be re-checked with the full golden suite.
## Related
Core reviewer: dead `do_circuit()`/`raphael_main()` and the leaked `new ParserState()` at main.cpp:146 are in the same code.

- Discovery seeds: S3.23 (see `backlog/README.md` for seed numbering).
- Review ids: F-A7-19.
## Verification
Code reading of the cited lines.
