# B-053: VL/EVL value files: the first line is always discarded; a header-less file loses its first sample
severity: med | category: bug | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/devices/value_list_source.h:63-97, src/devices/electrical_value_list_source.h:60-94
## Problem
`setValues(filename)` does an unconditional `getline(file, line)` (VL :65, EVL :62) before the loop.
- In a header-less file, the first data row is silently dropped.
- The intended header detection (`lineNumber == 1 && field == "time"`, VL :93, EVL :90) then looks at the *second* line, so it is dead for a file with a "time" header.
- Other loose ends:
  - Lines with CRLF endings are not handled.
  - `stod` accepts trailing junk.
  - `std::out_of_range` from `stod` is not caught.
## Evidence
`backlog/repro/a3-vlsrc-file.cir` (run in the repro dir):
- vlsrc1 (`a3-vl-noheader.csv`, rows 0.1/0.2/0.3 ns) emits only at 200 ps and 300 ps; the 100 ps / 1 mW row is lost.
- vlsrc2 (same rows plus a `time,power,wavelength` header) emits all three rows, but "Skipping header line" is never printed.
## Expected
All data rows are read. An optional header is detected explicitly.
## Suggested fix
Remove the unconditional `getline` and detect the header properly: either the first non-comment line has a non-numeric first field, or keep the `"time"` rule. This is a design choice, because files whose header does not start with `time` currently work only by accident. Do it once in a shared parser (see B-074).
## Golden impact
None: no golden case reads a values file (paul_3ring_puf fails earlier because its CSV is missing).
## Related
B-064 (error message), B-074 (VL/EVL duplication), B-063 (negative power).

- Discovery seeds: S2.18 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-07.
## Verification
I ran the repro with the release binary.
