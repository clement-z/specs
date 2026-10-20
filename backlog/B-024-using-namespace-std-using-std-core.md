# B-024: `using namespace std` / `using std::…` in core headers
severity: low | category: style/maintainability | effort: M | fix-class: needs-design | verdict: CONFIRMED
location: src/optical_signal.h:15, src/specs.h:13-19, src/optical_output_port.h:14-19, src/utils/pqueue.h:6-9, src/utils/general_utils.h:7-10, src/utils/sysc_utils.h:8-9, src/devices/spx_module.h:9-10, src/devices/subcircuit_instance.h:9-11
## Problem
The using-directive in optical_signal.h leaks into every TU. CLAUDE.md forbids new ones.
## Evidence
Code reading.
## Expected
Qualified names in headers.
## Suggested fix
Remove them header by header and qualify. This is not easy-unattended: without the directive, unqualified
`abs`/`pow`/`isnan` calls in many TUs may silently bind to other overloads (e.g. C `abs(int)`). Each step needs a
golden run.
## Golden impact
None if done correctly.
## Related
none.

- Discovery seeds: S1.31, S5.18 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-24.
## Verification
Code reading.
