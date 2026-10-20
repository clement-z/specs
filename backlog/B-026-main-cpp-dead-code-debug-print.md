# B-026: main.cpp dead code, debug print, unused includes, ParserState leak
severity: low | category: style/maintainability | effort: S | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 39500e1, 3494312 (ws-cleanup C-01, C-08; the ParserState leak by B-170)
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
`opt/tol-resolution-overrides`: the removed debug line `1e-05` at the top of stdout (re-record that case). In addition, `delete parser_state` changes heap layout, so `cir/subckt_tests` and `opt/vci` pass with "line order differs (reordered)" notes (pointer-ordered output, B-016); re-recording rewrites their stdout/VCD order. To avoid that, leave the ~64-byte `ParserState` leak (also listed in B-170) until B-016 is fixed.
## Related
B-025.

- Overlaps: the `ParserState` leak is also in B-170; `do_circuit`/`raphael_main` are also listed in B-174, which defers to this issue.

- Discovery seeds: S1.28, S3.32, S4.21, S5.19 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-26.
## Verification
Golden run with the reltol print removed: only that case differs. The rest is compile-only.

**Independent second check (V3, of F-A1-26): ADJUST.** Method: applied the full deletion list plus the ParserState change, rebuilt, golden check --atol 0, then a record/diff; bisected by re-running without `delete parser_state`

Compiles cleanly. No remaining references outside discover/*.md. Removing the includes (ctime, iomanip, deque) and `using namespace literals` is fine. The golden impact claim is incomplete. opt/tol-resolution-overrides fails only on the removed `1e-05` line, as claimed. In addition, cir/subckt_tests and opt/vci PASS "with notes (line order differs (reordered))". A record shows their OP printout order and VCD files change. Bisected: this comes from `delete parser_state`, which changes the heap layout and so the pointer-ordered std::set iteration (B-016). Without the delete: 0 notes. Golden impact should read: "opt/tol-resolution-overrides (removed debug line); cir/subckt_tests and opt/vci reorder (harness PASS with notes; re-recording rewrites their stdout/VCD order)". The other option is to drop the one-off ~64-byte ParserState leak fix until B-016 lands.

**Orchestrator:** Golden-impact text completed (heap-layout reorder from freeing ParserState).
