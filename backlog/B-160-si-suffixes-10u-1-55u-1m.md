# B-160: SI suffixes (`10u`, `1.55u`, `1m`, `10p`) are a syntax error
severity: med | category: parser | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parser.l:8, src/parser/parser.l:340-351
## Problem
Numbers are C-style only. SPICE users write `10u`, and the legacy netlists use suffixes (`350.85u`).
## Evidence
`cwsrc1 a wl=1.55u power=1m` gives `Parsing error (...:2): syntax error -- offending token: "u"`.
## Expected
`f p n u m k meg g t` scaling (case-insensitive, `meg` vs `m`).
## Suggested fix
Extend the `DOUBLE` pattern with an optional suffix and scale in the action. Watch for conflicts with names
(`1e`, nets like `10u`), and note that net names may start with digits via `T_INT`.
## Golden impact
None (additive).
## Related
B-167.

- Discovery seeds: S3.17 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-20.
## Verification
Ran on Release.
