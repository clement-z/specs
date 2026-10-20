# B-134: circuits/paul_3ring_puf.cir needs a CSV that was never committed
severity: low | category: docs | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: circuits/paul_3ring_puf.cir:60-64,69, src/devices/value_list_source.h:56-62,65
## Problem
The netlist ends with `.assign filename="paul_puf_bitstream_158b_3wl.csv"` (four earlier `.assign filename=` lines are dead overrides) and `VLSRC1 in values={filename}`. The file is not in the repository and never was (git history has no .csv). `specs -f` exits 1 with "Error: Cannot open the file." The netlist is otherwise valid: with a small synthetic CSV (header + 4 lines `time,power,wavelength`) in the cwd the whole 3-ring PUF netlist runs to "Simulated 8 ns" with exit 0 in 0.07 s.
## Evidence
```
$ specs -f circuits/paul_3ring_puf.cir   -> ... Creating ROOT/VLSRC1 ... Error: Cannot open the file.   (exit 1)
(with paul_puf_bitstream_158b_3wl.csv: 4 data rows)  -> exit 0, "@3 ps, ROOT/VLSRC1 emitted: ... 1.000000 W"
```
## Expected
Example runs from a clean checkout.
## Suggested fix
Choose one: ship a small `circuits/paul_puf_bitstream_158b_3wl.csv` (needs a decision on the content; note the golden harness copies `circuits/` so it will be found), or replace the file-based source by the inline `values=[[...]]` form that is already in the commented lines 72-73, and remove the four dead `filename` assignments. Also note `value_list_source.h:65` discards the first CSV line unconditionally (header or not) before the "time" header test: owner A3.
## Golden impact
cir/paul_3ring_puf: exit 1 -> 0, new stdout/VCD; re-record that case.
## Related
B-132, B-133, B-143.

- Discovery seeds: S3.11, S4.18, S5.21 (see `backlog/README.md` for seed numbering).
- Review ids: F-A5-14.
## Verification
Ran with and without a synthetic CSV (above).
