# B-025: `-T` and `--nrings_crow` are silent no-ops; `-f` silently wins over `-t`; misleading help
severity: low | category: cli | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/main.cpp:197-201,209-212,219-222,244-247,341-345,357-359,382-429
## Problem
`-T` ("Run manual test function") returns 0 before doing anything, so `-T -f x.cir` runs nothing. `--nrings_crow`
("temporary") is parsed but its assignment is commented out (crow_tb.cpp:33 still has the global). `-f` and
`-t` together run only the netlist. Help text problems: `-R/--resolution` sets `.options timescale`, while
`.options resolution` is the port multiplier; `-R` text is missing a separator; `-o` does not say ".vcd is appended".
## Evidence
`specs -T -f a1-tran-no-analysis.cir` → rc=0, no SIMULATION banner. `--nrings_crow 5` → rc=0, no effect. (`backlog/repro/a1-cli.sh`)
## Expected
Every flag either works or is removed. Conflicting flags are rejected.
## Suggested fix
Remove `-T`. Either wire `--nrings_crow` (uncomment main.cpp:343 under `BUILD_TB`) or remove it. Error on `-f` together with `-t`. Fix the help strings.

Note: the `--nrings_crow` part is split out as B-131 (wire the option back, with a range check). This issue keeps `-T`, `-f` vs `-t` and the help strings.
## Golden impact
`cli/help` stdout.
## Related
B-007, B-026.

- Discovery seeds: S1.28 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-25.
## Verification
Ran them.
