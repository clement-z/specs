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

## Fix (sprint 2026-10c PK, 8fc95cb)
REJECT is removed from the scanner (twin start conditions for the line rule, fixed-length trailing contexts):
flex grows its buffer, so lines of any length scan; outputs are otherwise unchanged (golden bit-identical,
independent differential review of about 92,000 inputs). Remaining limits, not fixed: bison's parser stack
(10,000 deep) fails a list of about 5,000 inline VLSRC values (`memory exhausted`, exit 1; 4,000 parse); and
scanning one line costs time quadratic in its length (8 MB: 24 s). A value file has neither limit.
