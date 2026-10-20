# B-169: `.ends NAME` (standard SPICE) is a syntax error
severity: low | category: parser | effort: XS | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parser.l:294-313, src/parser/parser.y:238
## Problem
`.ends s` returns `T_DIRECTIVE_ENDS`, then `s` is parsed as a new top-level token.
## Evidence
`.subckt s a b` / ... / `.ends s` gives `Parsing error (...:4): syntax error -- offending token: "s"`.
## Expected
`.ends` with an optional name (checked against the `.subckt` name).
## Suggested fix
Accept an optional `T_STR` after `T_DIRECTIVE_ENDS` in `subcircuit:` and compare it with the subckt name (warn or
error on mismatch).
## Golden impact
None.
## Related
B-157.

- Discovery seeds: S3.20 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-32.
## Verification
Ran on Release.
