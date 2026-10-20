# B-164: Net drivers are not validated (>2 optical writers; electrical nets never counted)
severity: low | category: parser | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parse_tree.h:460-463, src/parser/parser.y:670-681
## Problem
`ParseNet::validate()` (writers <= 2, ports <= 2) is never called. Three writers on an optical net, or two writers on
an electrical net (`net.eanalog` never increments reader/writer counts), pass the parser and fail at run time with
SystemC E115. Electrical nets show `readers 0, writers 0` in prints and JSON.
## Evidence
`backlog/repro/a4-three-writers.cir`: pre-flatten print shows `ROOT/a: OANALOG<1>u (3|1)`, then rc 1 with
`Error: (E115) sc_signal<T> cannot have more than one driver`. Two `evlsrc` on one net: rc 1, E115 at delta 7.
## Expected
A netlist error naming the net and its drivers.
## Suggested fix
Count reader/writer ports for electrical nets in the grammar. Call `validate()` (or an equivalent with a message)
after flatten. Decide the policy for more than 2 optical ports.
## Golden impact
None expected (golden nets have <= 2 writers). Check that the electrical counts do not change JSON output for
golden circuits (they have no electrical nets).
## Related
B-144, B-154.

- Review ids: F-A4-26.
## Verification
Ran both cases on Release.
