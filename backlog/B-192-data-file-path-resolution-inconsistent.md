# B-192: Data file paths resolve differently for VLSRC and SPAR
severity: low | category: parser | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: fixed in dd822d7 (sprint 2026-10c PK)
location: src/devices/value_list_source.h, src/devices/electrical_value_list_source.h (`setValues`), SPAR path resolution in src/parser/parse_element.cpp

## Problem
Found in phase PF03b ("Found, not done"). VLSRC/EVLSRC value files are opened relative to the working directory
(the golden netlists write repository-relative paths, e.g. `tests/golden/netlists/b052-vlsrc-values.csv`), while
SPAR data files are looked for next to the netlist first, then in the working directory (as `.include`).

## Expected
One rule for every file a netlist names.

## Suggested fix (needs design)
Resolve value files like `.include` and SPAR (next to the including netlist first, then the working directory).
Netlists that rely on the working directory keep working unless a file of the same name exists next to the
netlist; say so in the CHANGELOG.

## Golden impact
None expected (the existing paths resolve the same way), to be checked.
