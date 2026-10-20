# B-026: main.cpp dead code, debug print, unused includes, ParserState leak
severity: low | category: style/maintainability | effort: S | fix-class: easy-unattended | verdict: CONFIRMED
location: src/main.cpp:1-5,21,25,65-141,146,187,189,319
## Problem
`do_circuit` (65-141), `raphael_main` (187) and the never-defined `tests_module_registry_access` declaration (189)
are unused. `cout << reltol_val` (319) is a debug print to stdout, emitted even for invalid values. Unused
includes: `<ctime>`, `<iomanip>`, `<deque>`. The redundant forward declaration `class OpticalOutputPort;` (21) and
`using namespace literals;` (25) were only needed by `do_circuit`. `new ParserState()` (146) is never freed.
## Evidence
`grep -rn "do_circuit\|raphael_main\|tests_module_registry_access" src` finds only the definitions.
`specs --reltol 1e-5 …` prints `1e-05` as the first stdout line.
## Expected
none of the above.
## Suggested fix
Delete lines 65-141, 187, 189, 319, includes 1-2 and 5, line 21 and line 25. In `build_circuit`:
`ParserState *parser_state = new ParserState(); yylex_init_extra(parser_state, &scanner); … yylex_destroy(scanner); delete parser_state;`
(ParserState is used only through the scanner).
## Golden impact
`opt/tol-resolution-overrides`: the removed debug line `1e-05` at the top of stdout (validated: it is the only golden difference).
## Related
B-025.

- Discovery seeds: S1.28, S3.32, S4.21, S5.19 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-26.
## Verification
Golden run with the reltol print removed: only that case differs. The rest is compile-only.
