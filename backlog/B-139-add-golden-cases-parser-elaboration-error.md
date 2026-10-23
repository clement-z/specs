# B-139: add golden cases for parser/elaboration error paths
severity: low | category: test-gap | effort: M | fix-class: needs-design | verdict: CONFIRMED | status: partly addressed: ~50 err/* cases added in sprints 2026-10b/c (coupler k, .ic, X args, ...); unknown element/param, undefined subckt, duplicate name, missing .ends, no-analysis, empty file, power=1/0 still not covered
location: tests/golden/cases.json (cli/* group), new tests/golden/netlists/ (suggested)
## Problem
The only parser failures covered are the five broken shipped examples and `cli/missing-netlist`. Error handling for unknown elements/parameters, undefined variables/subcircuits, duplicate names, unterminated blocks, bad analyses is untested, and four dubious inputs are silently accepted.
## Evidence
`sh backlog/repro/a5-parser-error-paths.sh build/specs` (creates the netlists, prints exit status and last line). Observed on the Release build:
- exit 1 with message: unknown element (`offending token: "foo1"`), unknown parameter (`Unknown keyword: bogus`), undefined `.assign` variable (`Variable not found: nosuch`), missing `.ends` (`Reached end of file before end of subcircuit definition`), undefined subcircuit (`Subcircuit definition not found: NOSUCH`), duplicate element name (`a module with name 'ROOT/WG1' already exists`), two analyses (`Attempted to register a new analysis (TRAN)`), `power=1.2.3`, `.ic` (silent exit 1), `wg1 in length=...` (missing port), `.dc /nosuch/WL ...` (`Element not found: ROOT/NOSUCH`), negative `.dc` step, negative `.tran`, unterminated string.
- exit 0 (accepted): netlist with no analysis (prints DONE, simulates nothing), empty file, `power=1/0` (emits `inf V/m, -nan W`), coupler `k=2` (power gain).
## Expected
Error paths are regression-protected; the four silent cases are either documented or rejected (decision for the parser owner).
## Suggested fix
Put the 16 error netlists in `tests/golden/netlists/` (extend `setup.copy` for those cases), one case each with `"expect": "fail"`; add the 4 silent ones too so any decision shows up as a diff.
## Golden impact
new cases only.
## Related
reviewer A2 (`a2-coupler-nan.cir`), parser reviewer.

- Review ids: F-A5-19.
## Verification
Ran the script.
