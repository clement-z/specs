# B-159: CrossingBi is never instantiated, despite DECLARE_BIDIR_ELEMENT
severity: med | category: bug | effort: S | fix-class: needs-human | verdict: CONFIRMED
location: src/parser/parse_element.cpp:576-590, src/parser/parse_element.h:87
## Problem
`CrossingElement::create` always calls `INSTANTIATE(pt_helper, false)`. Unlike WG/DC, it never checks whether its
nets are bidirectional. `instantiate_and_connect_bi` (:546-574) and `CrossingBi` (devices/crossing.h:75) are dead.
Backward light through a crossing is never modelled. `kind()` still reports `CROSSING (bidir)`.
## Evidence
Code at :590. The repro of B-147 shows the crossing elaborated uni next to a bidirectional WG.
## Expected
The same rule as `DCElement::create` (:292-297): bidir if any net is bidirectional.
## Suggested fix
Copy the DC detection loop. Enabling it changes results for crossings on bidir nets and relies on `CrossingBi`
being correct (A2/A3), so a human should sign it off.
## Golden impact
None (no golden crossing).
## Related
B-147, B-038.

- Decide the crossing physics (B-032, B-033) before enabling `CrossingBi`.

- Discovery seeds: S2.07, S3.13 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-19.
## Verification
Code reading at :590 and the repro run.
