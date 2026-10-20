# B-161: `.ic` exits with code 1 and no message (and swallows any `.ic*` keyword)
severity: low | category: parser | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
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
