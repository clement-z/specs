# B-166: Net names are case-sensitive while everything else is case-insensitive
severity: low | category: parser | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parse_tree.h:597-630, src/parser/parse_tree.h:651-666
## Problem
Only the `ROOT/` prefix is uppercased. `In` and `in` are two nets, with no warning (a classic SPICE pitfall). It also
makes B-152 easy to hit through subckt port names.
## Evidence
`backlog/repro/a4-net-case.cir`: rc 0, `ROOT/in = 0 W` (probe), `ROOT/In = 1 mW` (source).
## Expected
Case-insensitive nets, or at least a warning for nets that have only readers or only writers.
## Suggested fix
Decide: uppercase net names (changes VCD/JSON signal names, which affects pyspecs and the golden files), or add a
dangling-net warning.
## Golden impact
Uppercasing renames every net in all cir*/opt cases. A warning adds stdout lines where nets dangle (e.g. `ROOT/drop`
in add_drop).
## Related
B-152.

- Discovery seeds: S3.25 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-29.
## Verification
Ran on Release.
