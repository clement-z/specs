# B-151: Subcircuits cannot see top-level `.param` values or top-level `.subckt` definitions
severity: med | category: parser | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parse_tree.h:682-697, src/parser/parse_tree.cpp:417-428, src/parser/parse_tree.cpp:846
## Problem
`get_variable()` checks the local and global (`pi`, `c`) tables only. `find_subcircuit()` searches only the tree
where the X element lives. A subckt body cannot use a top-level `.param` or instantiate a subckt defined at top
level, which blocks hierarchical designs. The workaround is to pass every value as a subckt kwarg and to nest
definitions.
## Evidence
- `backlog/repro/a4-param-scope.cir`: rc 1, `Variable not found: L`.
- `backlog/repro/a4-subckt-scope.cir`: rc 1, `Subcircuit definition not found: S`.
## Expected
Lexical scoping: look up the defining tree (`ParseTree::parent`, which is already set to `subcircuit.parent` at
parse_tree.cpp:306), then its parents.
## Suggested fix
Walk `parent` in both lookups. Design points:
- (a) Which value of a re-assigned top-level `.param` a body sees. The parent tree is fully parsed when flattening
  happens, so a body would see the *last* value.
- (b) A recursion/depth guard, because a subckt could then instantiate itself.
- (c) `get_variable` must become const-correct (`parent` is `const ParseTree*`).
## Golden impact
None expected (no golden netlist relies on the failure).
## Related
B-155.

- Discovery seeds: S3.03 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-10.
## Verification
Ran both repros on both builds.
