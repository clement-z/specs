# B-170: Parser memory leaks (scan-string copies, .param pairs, ParserState, sub-tree subcircuits)
severity: low | category: memory/UB | effort: S | fix-class: easy-unattended | verdict: CONFIRMED
location: src/parser/parser.l:133-142, src/parser/parser.y:186-194, src/main.cpp:146, src/parser/parse_tree.h:726-739, src/parser/parse_tree.cpp:342-352
## Problem
- `yy_add_content_from_string` `new[]`s a copy that `yy_scan_string` copies again, and never frees it.
- `local_assignment` actions never `delete $2`.
- `main.cpp:146` `new ParserState()` is never deleted.
- Sub-tree `ParseSubcircuit`s are never freed: `~ParseTree` deletes only when `!parent`, and the elements are
  transferred, but subcircuits are not.
- Directives and analyses `clear()`ed in subckt bodies are not deleted.
## Evidence
`valgrind --leak-check=full` on Debug `specs -n -f circuits/subckt_tests.cir`: `definitely lost: 1,450 bytes in 14
blocks`. Origins: parser.l:133 (3 blocks), parser.y:811 via the `local_assignment` path (9 blocks),
main.cpp:146 (1), parse_tree.cpp:406 from the sub-parse (1).
## Expected
0 definitely-lost bytes.
## Suggested fix
- parser.l:133-142: pass `str.c_str()` directly to `yy_scan_string` (drop `cstr`).
- parser.y:188/192: add `delete $2;` after `set_local_variable`.
- main.cpp: keep the `ParserState*` and delete it after `yylex_destroy`.
- `~ParseTree`: always delete `subcircuits`.
- parse_tree.cpp:345/351: delete the pointers before `clear()`.
## Golden impact
None (behaviour-neutral).
## Related
B-171.

- The `ParserState` leak (main.cpp:146) is also listed in B-026; whichever lands first, the other skips it.

- Discovery seeds: S3.21 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-33.
## Verification
valgrind run on the Debug binary. The fixes only free objects that are not referenced afterwards: the subckt
definitions of a sub-tree are only used during its own `flatten()`.
