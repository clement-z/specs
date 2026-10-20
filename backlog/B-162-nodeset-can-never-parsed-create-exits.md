# B-162: `.nodeset` can never be parsed, and create() exits "disabled" anyway
severity: low | category: parser | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parser.y:774, src/parser/parse_directive.cpp:65-125
## Problem
- The grammar expects `'/' net_name '/' T_STR '='`, but `P=` lexes as `T_ASSIGN_ID`, so every `.nodeset /a/P=1` is a
  syntax error.
- If it parsed, `net_name` would create a net as a side effect, and `NODESETDirective::create` exits
  `NODESET is disabled pending bugfixes`.
- The implementation is under `#if 0`.
## Evidence
`backlog/repro/a4-nodeset.cir`: `Parsing error (...:4): syntax error -- offending token: "P"` (also with a space
before `=`).
## Expected
Either a working `.nodeset`, or a clear "not supported" error at lex time.
## Suggested fix
Decide: remove the token, rule and dead code and add a lexer error like B-161, or fix the rule
(`'/' net_name '/' assignment`) and re-enable the engine part (specs.cpp `nodeset_orders`).
## Golden impact
None.
## Related
B-161, B-157.

- Discovery seeds: S5.07, S3.08 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-23.
## Verification
Ran on Release.
