# B-150: `.include` inside a `.subckt` body is executed at definition time and breaks the body capture
severity: med | category: parser | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/parser/parser.l:243-260
## Problem
The `.include` rule is active in the exclusive `SUBCKT_DEFINITION` state. It opens the file immediately and
`BEGIN(INITIAL)` leaves body-capture mode. The rest of the body is then parsed as top level, so a syntax error is
certain. Including a device library inside a subckt is a normal use.
## Evidence
`backlog/repro/a4-include-in-subckt.cir` (run from the repo root): rc 1,
`Parsing error (.../a4-include-in-subckt.cir:2): syntax error -- offending token: "\n"`. Line 2 is in the
*included* file.
## Expected
The `.include` line is stored verbatim in the body and executed when the body is re-parsed at flatten time. The
INITIAL-state rule already handles that.
## Suggested fix
`parser.l:243`: change `<INITIAL,SUBCKT_DEFINITION>^\.include` to `<INITIAL>^\.include`. Apply B-156 (fclose) in
the same series: each instance re-opens the file, so >1000 instances would exhaust file descriptors.
## Golden impact
None (verified). The input always failed before.
## Related
B-156, B-143 (path resolution), B-155 (location).

- Discovery seeds: S3.04 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-09.
## Verification
Patched scratch build: the repro runs (rc 0, `Including file:` printed at flatten time). An include of a `.param`
file inside a body also works. Golden is unchanged.
