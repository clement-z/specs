# B-176: Element creation order depends on pointer values (std::set<const ParseElement*>)
severity: low | category: determinism | effort: XS | fix-class: needs-design | verdict: PLAUSIBLE
location: src/parser/parse_tree.h:749, src/parser/parse_tree.cpp:724-742, :777-795
## Problem
`elements_backlog` is a `set<const ParseElement*>`, so elements are created in heap-address order. Because of
B-144/02, the signal slot assignment and therefore the results depend on creation order. A change in allocation
pattern can change elaboration order and results. Addresses usually follow allocation order (no difference
observed), but different element classes have different sizes and heap bins.
## Evidence
Code at :749 and :726. Related evidence: unrelated heap changes in my scratch patches (fclose, string erase)
reordered the pointer-ordered OP printout of cir/subckt_tests and opt/vci (golden note "line order differs").
## Expected
Creation order defined by netlist order.
## Suggested fix
Order the backlog by element index (e.g. `set<size_t>` of indices into `pt->elements`, or a deque plus an "in
backlog" flag).
## Golden impact
None expected if the current order equals allocation order (verify with the harness, `--strict-order`).
## Related
B-144, B-145, B-158.

- Same pointer-order theme in the engine: B-016.

- Review ids: F-A4-40.
## Verification
Code reading. Observed pointer-order sensitivity in OP printing (another area's code).
