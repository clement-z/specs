# B-157: The paper's Fig. 1b netlist does not parse: `.save`, `dc1` coupler prefix, `.dc WL(cwsrc1)`
severity: med | category: parser | effort: M | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parser.l:461, src/parser/parser.y:82, src/parser/parser.y:786-807, src/parser/parser.l:387
## Problem
- `.save` has a token but no grammar rule.
- The paper uses `dc1`/`dc2` for couplers, but the lexer only knows `COUPLER`.
- `.dc` only accepts `/elem/ATTR start stop step`, while the paper and cheatsheet use `WL(cwsrc1)`. In addition:
  - sweep bounds accept only `INT|NUM|{var}`, not `{expr}` or negative values;
  - elements inside a subckt instance are not addressable (`/x1/cwsrc1/WL` is a syntax error).

The published example cannot be run.
## Evidence
`backlog/repro/a4-paper-fig1b.cir` (copied verbatim from `doc/papers/zrounba2023-specs-ecoc.pdf`):
`Parsing error (...:4): syntax error -- offending token: ".save"`. Without `.save`: offending token `"dc1"`.
`.dc WL(cwsrc1) ...` gives offending token `"WL"`. `.dc /x1/cwsrc1/WL ...` gives offending token `"/"`.
`.dc /cwsrc1/WL {{l0}-5e-8} ...` gives offending token `"{"`.
## Expected
The paper's netlist runs, or the docs and paper errata state the supported syntax.
## Suggested fix
Decide which aliases to support:
- `.dc ATTR(elem) start stop step` → reuse `register_sweep_order`.
- A `DC` element prefix as an alias of `COUPLER`.
- `.save P(net) ...` → map to probes, or accept and ignore with a warning.
- Allow `variable.delimited_expr` for sweep bounds.

Alternatively, drop `T_DIRECTIVE_SAVE` and update the paper/cheatsheet docs.
## Golden impact
None for additive grammar.
## Related
B-162 (.nodeset), B-161 (.ic), docs area (cheatsheet mismatches).

- Discovery seeds: S5.04, S5.05, S3.08 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-16.
## Verification
Ran the repro on Release.
## Status
Partly fixed (sprint 2026-10b): `.dc WL(cwsrc1)` in b00b8ae (P5, F-12); `.save P(out) P(drop)` in 9feac83
(P6, F-05; golden case `regress/f05-save-paper-fig1b` runs the paper's DC part). Open: the `dc1`/`dc2` coupler
prefix (the golden netlist renames them `coupler1`/`coupler2`), `{expr}` and negative sweep bounds, and
elements inside a subcircuit instance in `.dc`.
