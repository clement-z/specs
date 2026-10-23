# B-155: Error messages have wrong or no location (subckt bodies, included files, elaboration errors)
severity: med | category: parser | effort: M | fix-class: needs-design | verdict: CONFIRMED | status: partly addressed: net-type mismatch no longer prints a null pointer (5912b6a, B-146); subckt-body locations are B-182; include file name, kwarg and elaboration locations stay open
location: src/parser/parse_tree.cpp:322-330, src/parser/parser.l:247-256, src/parser/parse_element.cpp:121 (and every "Unknown keyword"), src/parser/parse_tree.h:695, src/parser/parse_tree.h:848,907,913, src/parser/parse_tree.cpp:714-720
## Problem
- Errors in a subckt body print `(:N)`: an empty filename, a line relative to the body, and no instance name. The
  sub-parse's `ParserState` is never filled.
- Errors in an included file print the *parent* file name with the child's line number. The include rule never
  updates `current_filename`.
- Elaboration and semantic errors have no location, and many have no element name: `Unknown keyword: lenght`,
  `Variable not found: L`, `Wrong number of nets for subcircuit instance`.
- `Wrong signal type for 0 connected to ROOT/n` prints a null pointer (`sig`).
- A net named like an element (`WG1` and element `wg1`) reports `a module with name 'ROOT/WG1' already exists`.
## Evidence
- `backlog/repro/a4-errloc-subckt.cir`: `Parsing error (:2): syntax error -- offending token: ","`.
- `backlog/repro/a4-errloc-include.sh <specs>`: `Parsing error (parent.cir:3)` for an error on line 3 of `child.cir`.
- `backlog/repro/a4-errloc-kwarg.cir`: `Unknown keyword: lenght` (no file, line or element).
- `backlog/repro/a4-x-net-type-mismatch.cir`: `Wrong signal type for 0 connected to ROOT/n`.
## Expected
`file:line` (and the instance path for subckt bodies) on every user-facing error.
## Suggested fix
- Store `file:line` on each `ParseElement`/`ParseSubcircuit` at registration (the scanner knows both).
- Set the sub-parse `ParserState::current_filename` to `<file of .subckt> (subckt NAME, instance X1)` and add the
  body's starting line.
- Push/pop the filename together with the include buffer.
- Prefix `create()`/`get_variable` errors with the element name and location.
- At parse_tree.h:848, print the expected signal type, not the pointer.
## Golden impact
Only error-message text in expect=fail cases (cir/subckt_test, cir/double_ring, ...) if their messages are
touched. Inspect with `compare`.
## Related
B-172 (exit codes), B-175.

- Discovery seeds: S3.09 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-14.
## Verification
Ran all repros on Release.
