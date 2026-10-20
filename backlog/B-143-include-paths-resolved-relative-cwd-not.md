# B-143: `.include` paths are resolved relative to the CWD, not to the including file
severity: low | category: parser | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parser.l:248
## Problem
`fopen(yytext)` uses the CWD. Netlists only work from one directory (`circuits/subckt_tests.cir` includes
`circuits/add_drop_sub.cir`, so it must be run from the repo root). Quotes are not stripped either.
## Evidence
Code at :248. The golden harness copies `circuits/` into the cwd for this reason (tests/golden/README.md).

Additional evidence (independent report F-A5-24, A5 reviewer):

```
$ cd circuits && specs -n -f subckt_tests.cir   -> Error: included file not found "circuits/add_drop_sub.cir"  (exit 1)
$ specs -n -f circuits/subckt_tests.cir         -> exit 0 (from the repo root)
```
## Expected
Resolution relative to the including file (with a CWD fallback for compatibility).
## Suggested fix
Track the current file's directory in `ParserState`. Try `dir/path` first, then `path`. Strip surrounding quotes.
## Golden impact
None with the fallback. Without it, cir/subckt_tests and others break.
## Related
B-156, B-155.

- Discovery seeds: S3.10 (see `backlog/README.md` for seed numbering).
- Merged duplicate reports: F-A5-24 (A5).
- Review ids: F-A4-25, F-A5-24.
## Verification
Code reading. The golden README documents the cwd constraint.
