# B-165: `.op` and `.dc` silently ignore stray positional and keyword arguments
severity: low | category: parser | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/parser/parse_analysis.cpp:23-27, src/parser/parse_analysis.cpp:29-80
## Problem
`.op 5 foo=1` and `.dc /cwsrc1/WL a b c 7 foo=1` are accepted and the extras are ignored. `.tran` rejects kwargs
(:102-108).
## Evidence
`backlog/repro/a4-op-stray-args.cir`: rc 0, `ANALYSIS[OP] {5 (INTEGER), } {foo (INTEGER) = 1, }`. The `.dc`
variant is also rc 0.
## Expected
`Unknown keyword` / `too many positional arguments`, as for `.tran`.
## Suggested fix
At the start of `OPAnalysis::create` and `DCAnalysis::create`, reject non-empty `args`/`kwargs` with the B-148
error pattern.
## Golden impact
None (golden `.dc`/`.op`/`.tran` lines have no extras).
## Related
B-148.

- Review ids: F-A4-28.
## Verification
Ran on Release. Golden netlists checked by reading.
