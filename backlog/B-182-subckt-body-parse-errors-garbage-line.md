# B-182: Parse errors inside a `.subckt` body print an empty file name and a garbage line number
severity: low | category: parser | effort: S | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 6561510 (sprint 2026-10c PG): defining file and line in the body, nested bodies included
location: src/parser/parse_tree.cpp (~339, `ParseTree(name, subcircuit, kwargs)`)

## Problem
A subcircuit body is re-parsed per instance from a string (`yy_scan_string`); that scanner never gets
`yyset_lineno` or a file name, so a parse error inside the body prints `Parsing error (:<garbage>)`: the line number is
an uninitialised value (`:1634561391` in a release build, `:-1094795586` = 0xBEBEBEBE under ASan). Every parse error in
a subcircuit body is affected, including the integer-overflow error of B-180 and a nested `.ends` name mismatch.

## Expected
`Parsing error (<defining file>:<line in that file>)`, as for top-level lines.

## Suggested fix
Initialise the line number of the per-instance scanner (`yyset_lineno`) and pass the defining file's name and the
body's starting line, so errors point at the `.subckt` body in its file.

## Golden impact
Only the stderr of parse errors inside subcircuit bodies (no existing case has one); add one.

## Related
B-155 (errors inside included files name the including file), B-180, F-12.
