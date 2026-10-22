# B-161: `.ic` exits with code 1 and no message (and swallows any `.ic*` keyword)
severity: low | category: parser | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in c12ea44
location: src/parser/parser.l:469, src/parser/parse_directive.cpp:128-131
## Problem
`^\.IC { exit(1); }` stops the program silently. `.icfoo` also matches (prefix). `ICDirective::create`
(`"IC directive is not implemented"`) is unreachable.
## Evidence
`backlog/repro/a4-ic-silent.cir`: rc 1. The last output is the `PARSING CIRCUIT` banner, with no error line.
## Expected
`Parsing error (file:4): the .ic directive is not supported`, rc 1.
## Suggested fix
`parser.l:469`: `^\.IC { yylerror(yyscanner, "the .ic directive is not supported"); exit(1); }`. This keeps rc 1;
`exit(yylerror(...))` would give 3.
## Golden impact
None.
## Related
B-162, B-174 (ICDirective dead code).

- Discovery seeds: S5.06, S3.08 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-22.
## Verification
Patched scratch build: prints the message with file:line.

**Independent second check (V8, of F-A4-22): CONFIRMED.** Method: applied the parser.l:469 change, rebuilt, golden --atol 0, ran a4-ic-silent.cir, `.icfoo 1` and `.ic` inside a subckt (base vs patched)

Base: rc 1, no message for all three. Patched: rc 1 (kept) with `Parsing error (<file>:4): the .ic directive is not supported` and `-- while parsing line: ".ic /a/P=1"`. `.icfoo` is also caught with the same message. Inside a subckt body the location prints `(:1)`; that is the pre-existing subckt error-location problem, not caused by this fix. Golden: 44 pass, no notes.
