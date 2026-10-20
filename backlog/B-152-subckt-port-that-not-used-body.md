# B-152: A subckt port that is not used in the body crashes flatten (uncaught map::at)
severity: med | category: parser | effort: XS | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parse_tree.cpp:902, src/parser/parse_tree.cpp:912
## Problem
If a declared port does not appear on any element in the body, `sub_pt.nets.at(internal_net_name)` throws. If the
external net already exists, `sub_pt.nets[...]` creates a NONE-typed net and `combine_with` fails with a misleading
"Could not reconcile nets". Net names are case-sensitive (B-166), so a `.subckt s A` whose body uses `a` hits this.
## Evidence
`backlog/repro/a4-x-unused-port.cir`: rc 1,
`Error: (E549) uncaught exception: map::at`. Adding `probe2 out` at top level instead gives
`Could not reconcile nets: ROOT/X1/b and ROOT/out`.
## Expected
Either accept the dangling port (SPICE allows it) or reject it with
`subcircuit S: port 'b' is not connected inside the body`.
## Suggested fix
Before :902, check `sub_pt.nets.count(internal_net_name)`. Then either create the net with the external net's
type/size (accept) or print a clear error (reject). Choosing between the two is the design decision.
## Golden impact
None.
## Related
B-146, B-166.

- Review ids: F-A4-11.
## Verification
Ran both variants on both builds.
