# B-174: Dead code in the parser
severity: low | category: style/maintainability | effort: S | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 8b8a88a (ws-cleanup C-04)
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

Corrections from the second check:
- The `ICDirective::create` range is `parse_directive.cpp:128-132` (deleting :128-131 leaves a stray `}`).
- Optional: the commented `return T_NONE`/`T_BOOL` block at `parser.l:492-505` refers to the removed token and can go too.
- Deleting the X-element TODO comments loses a design note; removing only the empty `if` and keeping the comment is also acceptable.
## Golden impact
None (behaviour-neutral).
## Related
B-173, B-113.

- Review ids: F-A4-38.
## Verification
Code reading and grep for references.

**Independent second check (V8, of F-A4-38): ADJUST.** Method: deleted every listed range (main.cpp included), rebuilt, golden --atol 0, differential -n run of all netlists, edge-syntax tests (positional after kwarg for element/directive/analysis/X, subckt port after kwarg, `None` in lists), bison state comparison; grep of every deleted symbol

All the code is truly dead. These symbols have 0 references outside their definitions: `validate()`, `bidir_capable()` (base and both macros), base `n_nets`/`bidirectionalable` (only the macro members are used), `set_global_variable`, `ParseTree::clone`, `ICDirective`, `do_circuit`, `raphael_main`, `c_ptr`/`vec_d_ptr`, `delete_me_npos`. T_NONE is only "returned" inside a /* */ comment in parser.l, so `None` is a syntax error before and after the change. The "positional after keyword" checks and the subckt port-after-kwarg check cannot fire, because the grammar rejects that input first and register_subcircuit always creates a fresh object. I tested both: identical "syntax error" output before and after. The automaton goes from 199 to 198 states (only the T_NONE shift state disappears). Build OK. Golden: 44 pass, 2 reordered notes (heap layout, as in B-156). Differential run: no rc/stderr change. Corrections: (1) `parse_directive.cpp:128-131` must be **:128-132** (the closing brace of ICDirective::create is on :132; deleting :128-131 leaves a stray `}`). (2) decisions.json already says to skip main.cpp:65-141/:187 (B-026). (3) Optional: the commented `return T_NONE`/T_BOOL block in parser.l:492-505 refers to the removed token and could go too. Deleting the X-element TODO comments loses a design note; keeping the comment and removing only the empty `if` is also acceptable.

**Orchestrator:** ICDirective line range corrected (128-132).
