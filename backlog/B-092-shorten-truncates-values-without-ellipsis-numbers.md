# B-092: `shorten()` truncates values without an ellipsis: numbers lose their exponent and look like different numbers
severity: low | category: python | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: utils/draw_graph_from_json.py:6-13
## Problem
`s[:length-3]` was meant to leave room for "..." but nothing is appended, so kwargs displayed in the graph are silently wrong.
## Evidence
```
shorten(1.2345e-09)    -> '1.234'
shorten(299792458.0)   -> '29979'
shorten(3.14159265)    -> '3.141'
shorten('ROOT/N00001') -> 'ROOT/'
```
## Expected
Truncation is visible, e.g. `1.234...`.
## Suggested fix
`return s[:length-3] + '...'`.
## Golden impact
none.
## Related
B-090, B-091.

- Discovery seeds: S3.33 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-17.
## Verification
Ran the function on the listed inputs.
