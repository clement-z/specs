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

**Independent second check (V2, of F-A7-06): CONFIRMED.** Method: applied `[[maybe_unused]]` at parse_tree.cpp:217 and deleted parse_analysis.cpp:39 and :78, rebuilt (Release/NDEBUG), checked g++ (NDEBUG and non-NDEBUG) and clang++ 19 -fsyntax-only before/after, golden check, plus 5 DC/TRAN repros before/after

Baseline Release log has `parse_tree.cpp:217:18: unused variable 'has_reader'`; gone after the fix (g++ -DNDEBUG and without NDEBUG: 0 warnings; clang: warning gone). clang on the original parse_analysis.cpp reports `39:9: variable 'i' set but not used [-Wunused-but-set-variable]` (GCC 11 silent as claimed); gone after. Golden 44/44 at atol=rtol=0 (cir/add_drop contains a `.dc` sweep); stdout of a1-dc-range-endpoint, a1-dc-bare, a4-dc-endpoint, a4-paper-fig1b, a1-tran-no-analysis byte-identical before/after. Note: clang also prints many unrelated `sc_has_process_used` deprecation warnings from SystemC 3 headers (not part of this finding; relevant for the planned clang CI job).
