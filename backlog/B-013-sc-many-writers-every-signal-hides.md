# B-013: `SC_MANY_WRITERS` on every signal hides multiple drivers (silent last-writer-wins)
severity: med | category: bug | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/specs.h:28-31,92-93, src/specs.cpp:116,282,348
## Problem
Optical nets use `SC_MANY_WRITERS`, so SystemC detects two drivers only when they write in the same delta (E115).
In different deltas the writes overwrite each other silently. The parser lets two uni outputs share a net.
## Evidence
`backlog/repro/a1-two-drivers.cir` (merger1 and merger2 both drive `m`, one path delayed): rc=0, no message,
`ROOT/m:1 = 0.353553 (0.125 W)` (merger2's value). merger1's 0.5 W is lost. Without the delay: `Error: (E115)
sc_signal<T> cannot have more than one driver`.
## Expected
An elaboration error for two drivers on one net.
## Suggested fix
Reject at parse time (parser net writer counting, A4). As a safety net, consider `SC_ONE_WRITER` for
`oa_signal_type`: writes from non-process context (SPECSConfig) are allowed by that policy. The testbenches and
the `sc_get_all_object_by_type<sc_signal<OpticalSignal, SC_MANY_WRITERS>>` lookups would need checking.
## Golden impact
None expected if only the parser check is added.
## Related
A4 (net typing: two uni writers become a bidirectional net).

- Discovery seeds: S1.21 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-13.
## Verification
Ran the repro both with and without the delay.
