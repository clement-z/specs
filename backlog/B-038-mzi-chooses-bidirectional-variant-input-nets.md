# B-038: MZI chooses the bidirectional variant from its input nets only
severity: low | category: parser | effort: XS | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parse_element.cpp:483-490
## Problem
`MZIElement::create` copies the phase-shifter rule ("for PS, if any optical net is bidir") but checks only
`nets[0]` and `nets[1]`. With `mzi1 in _ out1& out2 v`, `MZIActiveUni` is instantiated and connected with
`connect_uni` to the bidirectional `out1`, so light travelling back into `out1` is ignored. The behaviour is
inconsistent with WG and COUPLER, which go bidirectional when any of their nets is bidirectional (:84-88, :292-297).
## Evidence
`mzi1 in _ out1& out2 v` with `.op` runs (exit 0) and builds a Uni MZI: the OP shows the `MZI1/DC1_out1` uni wires.
`mzi1 in& _ out1 out2 v` builds the Bi variant (and aborts, B-031).

Additional evidence (independent report F-A4-21, A4 reviewer):

Code at :486-487 (`nets[0]`, `nets[1]` only).
## Expected
The same rule as the other bidirectional elements: bidirectional if any optical net (0..3) is bidirectional.
## Suggested fix
After B-031 lands, add `nets[2]` and `nets[3]` to the check at :486-487 and fix the comment. This is needs-design
because it changes results for netlists that put `&` only on MZI outputs (reflections would then propagate).
## Golden impact
None. No MZI netlist in golden.
## Related
B-031 must land first, otherwise these netlists start aborting.

- The A4 reviewer filed the same issue (B-038, merged) and rated it needs-human; kept needs-design because the bidirectional MZI aborts today (B-031).

- Merged duplicate reports: F-A4-21 (A4).
- Review ids: F-A2-08, F-A4-21.
## Verification
Ran both netlists on the stock binary. Code reading of :483-490.
