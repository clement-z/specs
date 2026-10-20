# B-092: `shorten()` truncates values without an ellipsis: numbers lose their exponent and look like different numbers
severity: low | category: python | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 5e54f9b
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

**Independent second check (V5, of F-A6-17): CONFIRMED.** Method: ran shorten() on 8 inputs before/after; generated the DOT for add_drop.json (from `specs -n --json=`) and ran `dot` on it

After `return s[:length-3] + '...'` (line 11): 1.2345e-09 -> '1.234...', 299792458.0 -> '29979...', 'ROOT/N00001' -> 'ROOT/...'; strings of length <= 8 and lists ('[...]') are unchanged. Both call sites pass the value through html.escape, and the generated graph (layout switched to dot for the check) renders and contains "...". The fix is one local line, no design choice.
