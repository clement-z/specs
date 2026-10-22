# B-133: three legacy-syntax example netlists cannot be parsed by the current grammar
severity: low | category: docs | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: fixed in 829894f (sprint 2026-10b P5, F-12): option (b), moved to `circuits/legacy/` with a README; golden cases kept
location: circuits/double_ring.cir, circuits/triple_ring.cir, circuits/paul_simple_ring.cir (all fail at line 4), tests/golden/cases.json:41-49,60-68,85-93
## Problem
They use the old SPECS v1 netlist language: `one 0`, `clk 1 100p`, `bsrc 0 1 2 {0, 252, 15, 0} 1550.0e-9`, 4-node `dc 2 3 4 5`, `wg 10 3 350.85u`, 3-node `pdet 2 35 24`, `*tracenet`. The current grammar has no `one`, `clk`, `bsrc`, no bare `dc`/`wg` names and a different `pdet` signature. `specs -f` exits 1 with "Parsing error (circuits/double_ring.cir:4): syntax error". triple_ring.cir also carries a stale comment block copied from double_ring. They are the only examples of 2- and 3-ring cascades.
## Evidence
```
$ specs -n -f circuits/double_ring.cir   -> Parsing error (circuits/double_ring.cir:4): syntax error   (same for triple_ring, paul_simple_ring)
```
## Expected
Every netlist in circuits/ either runs or is clearly marked legacy.
## Suggested fix
Decision needed: (a) delete the three files, (b) move them to `circuits/legacy/` (outside the `circuits/*.cir` glob used by `discover_uncovered`) with a README line, or (c) port them (resonance wavelengths, couplings and the bitstream source would need to be re-derived, a physics-adjacent task). Recommend (b).
## Golden impact
cir/double_ring, cir/triple_ring, cir/paul_simple_ring are removed from cases.json and the baseline (the harness prints STALE until re-recorded).
## Related
B-132, B-134.

- Discovery seeds: S3.11, S4.18, S5.21 (see `backlog/README.md` for seed numbering).
- Review ids: F-A5-13.
## Verification
Ran the parser on all three; checked parser.l prefixes.
