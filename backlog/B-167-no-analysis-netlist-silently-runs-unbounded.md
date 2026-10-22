# B-167: No analysis in the netlist silently runs an unbounded transient
severity: low | category: cli | effort: XS | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parse_tree.cpp:808-809, src/specs.cpp:19-35
## Problem
Without `.op/.dc/.tran`, the run uses the default TIME_DOMAIN with infinite duration and prints `Simulated 0 s` for
CW circuits, with no warning.
## Evidence
`backlog/repro/a4-no-analysis.cir`: rc 0, `analysis type: TIME DOMAIN`, `Simulated 0 s`.
## Expected
A warning (or an error) that no analysis was given.
## Suggested fix
In `build_circuit`: `if (analyses.empty()) cout << "Warning: no analysis specified, defaulting to .tran" << endl;`.
The wording and the warning-vs-error choice are the design decision.
## Golden impact
With a warning: one extra stdout line in cir/add_drop_sub (the only golden netlist without analysis).
## Related
none.

- Discovery seeds: S3.29 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-30.
## Verification
Ran on Release.
