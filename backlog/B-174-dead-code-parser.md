# B-174: Dead code in the parser
severity: low | category: style/maintainability | effort: S | fix-class: easy-unattended | verdict: CONFIRMED
location: src/parser/parse_tree.h:460-463, :519-520, :563, :675-680, :702-722, :392-401, src/parser/parse_element.h:47, :74, src/parser/parse_directive.h:57-68, src/parser/parse_directive.cpp:69-125, :128-131, src/parser/parser.y:56-57, :70, :216-220, :430-431, :476-477, :481-495, :517-518, :522-536, :570-571, :697-724, :760-768, :826-829, :865-877, src/parser/parser.l:192, :280, :324-325, :518-519, :566-579, src/parser/parse_tree.cpp:922-924, :953-971, src/main.cpp:65-141, :187
## Problem
Unused or unreachable code:
- `ParseNet::validate()`, `bidir_capable()`, `ParseElement::n_nets`/`bidirectionalable` (shadowed by the macro
  members), `set_global_variable`, `ParseTree::clone`.
- A stray line-continuation `\` at parse_tree.h:563.
- `ICDirective`, and the NODESET `#if 0` body.
- Unused union members `c_ptr`/`vec_d_ptr`, and the never-lexed `T_NONE`.
- Unreachable "positional arguments cannot follow keyword arguments" checks (the grammar already forbids it).
- The unreachable subckt kwargs-before-port check.
- Empty X-element TODO `if` blocks, commented bus and str rules, `delete_me_npos`, `if (false)` debug blocks.
- `do_circuit()` and `raphael_main()` in main.cpp.
## Evidence
grep shows no callers: `validate()` and `bidir_capable()` have 0 uses; `ParseTree::clone`, `set_global_variable`
and `ICDirective` are never instantiated or called.
## Expected
Removed.
## Suggested fix
Delete them, one commit per file. Do not touch `T_DIRECTIVE_SAVE`/`.nodeset` until B-157/23 are decided.

Skip `main.cpp:65-141` and `:187`: they are covered by B-026.
## Golden impact
None (behaviour-neutral).
## Related
B-173, B-113.

- Review ids: F-A4-38.
## Verification
Code reading and grep for references.
