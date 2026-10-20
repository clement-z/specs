# B-168: Expression and argument semantics surprises (`^` left-assoc, `[]`, `None`, negatives, duplicate kwargs)
severity: low | category: parser | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parser.y:906-909, src/parser/parser.y:917-941, src/parser/parser.y:826-829, src/parser/parser.y:846-851, src/parser/parser.y:502-509, src/parser/parse_element.cpp:743,756,767
## Problem
- `2^3^2 = 64`: `^` is left-associative.
- `[]` yields `[None]`.
- `T_NONE` is never lexed, yet the VLSRC/EVLSRC errors say "or None".
- Negative positional values need braces (`wg1 a b 1e-3 2.3 4.2 -0.1` is a syntax error).
- `.dc` bounds cannot be expressions.
- A duplicate kwarg silently overwrites (`length=1 length=2`). Case variants (`Length`, `LENGTH`) are both applied,
  in map order.
## Evidence
- `.param x = 2^3^2` then `.options test_variable={x}` gives `TEST_VARIABLE(DOUBLE): 64`.
- `.param e = []` gives `[{"type":"NONE","value":null}]`.
- The negative positional gives `offending token: "-"`.
## Expected
`^` right-associative (2^3^2 = 512). `[]` is an empty list. Error texts say "leave the slot empty". Unary minus in
positional values. Duplicate kwargs rejected.
## Suggested fix
`fact: pow '^' fact`. Handle the empty-list case in `listvar.bound`. Fix the message text. Allow `'-' variable_base`
in `variable.delimited_expr`. Check for duplicates in the `element.with_kwargs` action.
## Golden impact
None expected (no golden netlist uses chained `^`, `[]` or duplicate kwargs).
## Related
B-160.

- Discovery seeds: S3.26 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-31.
## Verification
Ran each case on Release.
