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

Correction from the second check: the fixed netlist produces **no VCD**; `cir/subckt_test` changes to exit 0 with new stdout (63 lines) and empty stderr. When re-recording (`record --only 'cir/subckt_test$'`), also remove `"expect": "fail"` and the stale note from `tests/golden/cases.json` for that case.
## Related
B-133, B-134.

- Discovery seeds: S3.11, S4.18, S5.21 (see `backlog/README.md` for seed numbering).
- Review ids: F-A5-12.
## Verification
Ran the fixed netlist. Safe: example file only.

**Independent second check (V6, of F-A5-12): ADJUST.** Method: ran the original netlist (exit 1, "Parsing error ... :5"), isolated each of the three errors, applied the fix in a scratch circuits/ and ran the golden harness from the scratch tree against the committed baseline, then `record --only` into a copy of the baseline

Reproduced: `cwsrc` -> parse error; with only that fixed, `WG 1 2` -> "Parsing error (:1): syntax error ... offending token WG" at flatten time; with both fixed, `Unknown keyword: pow`. `att` (dB/cm) and `power` are valid keywords (parse_element.cpp), so the intended content is unambiguous (`pow` = `power`); fixed netlist exits 0 with `ROOT/out = 1 W @ 1550 nm, phase -0.8107 rad`. Golden: only `cir/subckt_test` changes (exit 1 -> 0); `cir/subckt_tests` etc. unchanged. Text corrections: (1) the fixed case produces NO VCD (recorded `files: {}`), so say "new stdout (63 lines), empty stderr, exit 0" instead of "new stdout and VCD"; (2) when updating cases.json:69-77 also remove `"expect": "fail"` (it is only informational in run_golden.py) and replace the stale note; re-record with `record --only 'cir/subckt_test$'` (merge mode verified; afterwards `check` is 44/44 pass). Note: *.cir is git-ignored, so the edit to circuits/subckt_test.cir is a tracked file already (no `git add -f` needed, it exists in HEAD).

**Orchestrator:** Golden-impact text corrected (no VCD; update cases.json expect/note).
