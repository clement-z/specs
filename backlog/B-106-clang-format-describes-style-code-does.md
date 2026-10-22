# B-106: `.clang-format` describes a style the code does not use (K&R control braces, 88 columns, C++11)
severity: low | category: style/maintainability | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: .clang-format:19-35 (`BraceWrapping.AfterControlStatement: false`), `ColumnLimit: 88`, `Standard: Cpp11`; Makefile:289-292 (`make format`)
## Problem
The code base uses Allman braces for control statements (850 lines consisting only of `{` vs 136 K&R-style control lines in `src/**/*.cpp,h`) and goes past 88 columns on 376 lines (138 past 100). `clang-format --style=file` 19.1 reports a diff for 104 of 108 source files, matching CLAUDE.md. `make format` runs `clang-format -i` on every file, so the first use rewrites the tree. `Standard: Cpp11` parses as C++11 although the project is C++20.
## Evidence
```
for f in $(git ls-files 'src/*.cpp' 'src/*.h' | grep -v parser/parser\.); do clang-format --style=file $f | cmp -s - $f || n=$((n+1)); done  -> 104 of 108
```
## Expected
`.clang-format` matches the code (or the code is migrated in one reviewed commit).
## Suggested fix
Either update `.clang-format` (`BreakBeforeBraces: Allman` with `AfterFunction` etc., `ColumnLimit: 120`, `Standard: c++20`) and verify the diff on a few files, or remove the `format` target until a tree-wide reformat commit is agreed. CLAUDE.md forbids bulk formatting in the meantime.
## Golden impact
none (formatting only, but a tree-wide reformat should be a separate commit).
## Related
CLAUDE.md "Code style".

- Review ids: F-A6-31.
## Verification
clang-format 19.1.7 run on all 108 files; grep counts as above.
