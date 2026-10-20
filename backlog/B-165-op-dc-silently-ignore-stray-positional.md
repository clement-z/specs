# B-165: `.op` and `.dc` silently ignore stray positional and keyword arguments
severity: low | category: parser | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in b50ced3
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

**Independent second check (V8, of F-A4-28): CONFIRMED.** Method: added the args/kwargs rejection (B-148 message pattern, kwargs as in .tran) at the top of OPAnalysis::create and DCAnalysis::create, rebuilt, golden --atol 0, ran the repro, a `.dc ... 7 foo=1` variant, plain .op/.dc, and a differential run of all netlists

Base: `.op 5 foo=1` rc 0, and `.dc /cwsrc1/WL a b c 7 foo=1` rc 0. Patched: both rc 1 (`.op: too many positional arguments (1, expected at most 0)`, and the same for `.dc`). Plain `.op` and `.dc /cwsrc1/WL a b c` are unchanged. Golden: 44 pass, no notes. Differential run over circuits/*.cir and all repro netlists: only a4-op-stray-args.cir changes. Nothing in circuits/, the cheatsheet (`.op`, `.tran T`, `.dc WL(cwsrc1) wl0 wl1 dwl`), pyspecs (CLI args only) or the notebook uses stray args. Rejecting them matches the existing `.tran` behaviour, so the only alternative would be a warning, and nothing argues for one. The sweep spec itself goes through `cw_sweep_order`/`sweep_orders`, not `args`, so it is not affected.
