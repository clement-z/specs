# B-132: circuits/subckt_test.cir fails to parse: three trivial syntax errors
severity: low | category: bug | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: circuits/subckt_test.cir:2,5, tests/golden/cases.json:69-77
## Problem
Element names need at least one character after the prefix (parser.l uses `^CWSRC({ALPHA_PLUS_NUM})+`, `^WG(...)+`), so the bare names `cwsrc` (line 5) and `WG` (line 2) are not elements, and `pow=1` is not a CWSRC parameter (`power=`). `specs -f circuits/subckt_test.cir` exits 1 ("Parsing error ... :5: syntax error"). Fixing only line 5 exposes the next errors one by one (syntax error in the subcircuit body at flatten time, then `Unknown keyword: pow`).
## Evidence
`backlog/repro/a5-subckt-test-fixed.cir` (the three edits) runs: exit 0, `ROOT/out = 1 W @ 1550 nm, phase -0.8107 rad` (3 m of lossless neff=1 waveguide, `len=3` default).
## Expected
The shipped example runs.
## Suggested fix
circuits/subckt_test.cir: line 2 `WG` -> `WG1`; line 5 `cwsrc in pow=1 wl=1.55e-6` -> `cwsrc1 in power=1 wl=1.55e-6`. (Optionally uncomment `probe1 out`.) Update the golden note at cases.json:69-77.
## Golden impact
cir/subckt_test: exit 1 -> 0 with new stdout and VCD; re-record that case only.
## Related
B-133, B-134.

- Discovery seeds: S3.11, S4.18, S5.21 (see `backlog/README.md` for seed numbering).
- Review ids: F-A5-12.
## Verification
Ran the fixed netlist. Safe: example file only.
