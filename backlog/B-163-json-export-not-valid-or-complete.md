# B-163: JSON export is not valid or complete JSON (escaping, inf/nan, bool, precision, subcircuits "TODO")
severity: low | category: bug | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parse_tree.h:119-166, src/parser/parse_tree.cpp:249-301, src/parser/parse_tree.cpp:554-559, src/parser/parse_tree.cpp:569, src/parser/parse_tree.cpp:643-657
## Problem
- Strings are written without escaping.
- Doubles use `scientific` with 6 digits (precision loss), and non-finite values give `inf`/`nan`.
- `"bidirectional"` is written as `0`/`1`, not as a JSON boolean.
- `"subcircuits":"TODO"` is a string where a list is expected.
- Element `"type"` always says `(bidir)` for bidir-capable classes, even when they are instantiated unidirectionally.
## Evidence
`backlog/repro/a4-json.cir` with `-n --json out.json`, then `python3 json.load` gives
`Expecting value ... 'big":{"type":"DOUBLE","value":inf}'`. After replacing inf: `Invalid \escape ...
"value":"C:\x.csv"`. Also `wl=1.5512345678e-6` is exported as `1.551235e-06`, and `"bidirectional":0`.
## Expected
RFC 8259 JSON with full double precision (`max_digits10`), escaped strings, booleans, and a defined encoding for
non-finite values (null or string) and for subcircuits.
## Suggested fix
A small JSON-escaping helper plus `setprecision(17)`. Emit `true/false`. Implement `ParseSubcircuit::to_json`
(name, ports, kwargs) or emit `[]`. The schema and the non-finite encoding are design choices.
## Golden impact
cir-json/* (precision changes values beyond the 1e-9 rtol; `subcircuits` field type; bool type) must be re-recorded.
## Related
B-154.

- Discovery seeds: S3.12 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-24.
## Verification
Ran the repro and validated with Python's json module.
