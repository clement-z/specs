# B-112: Two conditional unused-variable warnings (`has_reader` only with NDEBUG, `i` only with newer compilers)
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/parser/parse_tree.cpp:217, src/parser/parse_analysis.cpp:39,78
## Problem
1. parse_tree.cpp:217 `bool has_reader = ...` is read only inside `assert(has_writer ^ has_reader)` (line 220). Release/NDEBUG builds (the CMake Release baseline, `/tmp/ws-review-ninja.log`) warn `unused variable 'has_reader'`; Debug builds do not, so Meson's default `b_ndebug=false` hides it but `-Db_ndebug=true` (PLAN.md:121) brings it back.
2. parse_analysis.cpp:39 `int i = 0;` with `++i;` (line 78) is never read. GCC 11 is silent; clang 19 reports `-Wunused-but-set-variable`, and GCC >= 13 does too. Needed for the clang/new-GCC CI job.
## Evidence
`parse_tree.cpp:217: bool has_reader = pt->nets[net_name].m_connect_reader_count;` / `:220: assert(has_writer ^ has_reader);`
`clang++ -fsyntax-only -Wall -Wextra src/parser/parse_analysis.cpp` -> `parse_analysis.cpp:39:9: warning: variable 'i' set but not used`.
## Expected
No warnings in any build type or compiler.
## Suggested fix
parse_tree.cpp:217: `[[maybe_unused]] bool has_reader = ...;`. parse_analysis.cpp: delete line 39 (`int i = 0;`) and line 78 (`++i;`).
## Golden impact
none.
## Related
B-111, B-027.

- Review ids: F-A7-06.
## Verification
Patched scratch copies: parse_tree.cpp with `-DNDEBUG`: 1 warning -> 0; parse_analysis.cpp with clang: 1 -> 0. Attribute and deleted dead counter do not change code generation.
