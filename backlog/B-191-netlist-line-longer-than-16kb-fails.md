# B-191: A netlist line longer than about 16 KB fails to parse
severity: low | category: parser | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: fixed in 8fc95cb (sprint 2026-10c PK): REJECT removed, no line length limit
location: src/parser/parser.l (rules using REJECT)

## Problem
Found in phase PF03b ("Found, not done"), confirmed by the coordinator: a VLSRC with a long inline
`values=[...]` list fails with `input buffer overflow, can't enlarge buffer because scanner uses REJECT` (exit 2)
once the line is longer than flex's buffer: 700 entries (16051 bytes) parse, 800 entries (18351 bytes) fail.
Value files are not affected.

## Expected
Long lines parse, or a clear error naming the limit and suggesting a value file.

## Suggested fix (needs design)
Remove the need for REJECT in the scanner (it prevents flex from growing its buffer), or enlarge `YY_BUF_SIZE`
as a stopgap; add a golden case with a long inline list.

## Golden impact
None for existing cases; one new case.
