# B-150: `.include` inside a `.subckt` body is executed at definition time and breaks the body capture
severity: med | category: parser | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in f597031
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

**Independent second check (V7, of F-A4-09): CONFIRMED.** Method: changed parser.l:243 to `<INITIAL>^\.include`, rebuilt; ran the repro, a top-level-include reference, and a .param include with 2 instances against its flat reference; golden check 44/44 (atol 0)

Base: the repro gives rc 1 (syntax error). Every body-level `.include` fails today: after BEGIN(INITIAL) bison expects `.ends`, and a later `.ends` in INITIAL is fatal, so only failing inputs change. Patched: rc 0, with drop = 0.832 mW and out = 0.158 mW, identical to `.include` at top level plus a direct `x1 ... add_drop_filter`. `v7-inc-param.cir` (`.include` of a `.param` file inside the body, 2 instances) gives rc 0 with 2 "Including file" lines, and out = 1 mW at -2.4727 rad, identical to the flat reference. SUBCKT_DECLARATION is an inclusive state and never matched `.include` before either, so nothing else changes. pyspecs/utils/doc emit no `.include`. Sequencing with B-156 (fclose) as noted is fine.
