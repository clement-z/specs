# B-143: examples that `.include` other files only run from the repository root
severity: low | category: docs | effort: XS | fix-class: needs-design | verdict: CONFIRMED
location: circuits/subckt_tests.cir:10, circuits/paul_3ring_puf.cir:66
## Problem
`.include circuits/add_drop_sub.cir` resolves relative to the process cwd, not to the including file. `cd circuits && specs -f subckt_tests.cir` fails.
## Evidence
```
$ cd circuits && specs -n -f subckt_tests.cir   -> Error: included file not found "circuits/add_drop_sub.cir"  (exit 1)
$ specs -n -f circuits/subckt_tests.cir         -> exit 0 (from the repo root)
```
## Expected
Either includes resolve relative to the including file, or the examples state the requirement.
## Suggested fix
Lexer change (resolve relative to the including file, parser owner) or a one-line comment `* run from the repository root: .include is relative to the cwd` in both examples (the golden harness copies `circuits/` into the cwd to satisfy this, README of the harness).
## Golden impact
none for the comment; none for the lexer change (harness already runs from the root layout).
## Related
Parser reviewer; CLAUDE.md "`.include` paths resolve relative to the cwd".

- Review ids: F-A5-24.
## Verification
Ran both invocations.
