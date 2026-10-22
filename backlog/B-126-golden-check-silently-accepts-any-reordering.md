# B-126: golden `check` silently accepts any reordering of stdout lines for every case
severity: med | category: test-gap | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: fixed in b43e7cd (sprint 2026-10b P1)
location: tests/golden/run_golden.py:755-773 (`compare_case`, condition at :766), :1096-1099 (`--strict-order` help), CLAUDE.md "Testing"
## Problem
If stdout differs, `compare_case` also compares the sorted multiset of lines; if that matches, the difference is reported as a note ("reordered") and the case passes, for all cases, unless `--strict-order` is given. The command mandated in CLAUDE.md does not pass `--strict-order`. The justification (pointer-ordered iteration of `std::set<T*>`) applies to 4 cases only (tb/det, tb/detector under -O0; cir/subckt_tests and opt/vci under clang), but the testbench stdout is a chronological event log: swapping whole time blocks, i.e. making time run backwards or reordering events, still passes.
## Evidence
`sh backlog/repro/a5-golden-holes.sh <repo>` (works on a scratch copy of the baseline), part 1: the first two 5-line event blocks of tb/wg are swapped (100 ms before 0 s):
```
PASS     tb/wg  (stdout: line order differs (reordered))        exit=0
--strict-order: FAIL, "ONLY THE LINE ORDER DIFFERS ..."
```
Cross-check: `check --strict --strict-order --atol 0` already passes 44/44 for the Release build (run today), so a strict default is affordable.
## Expected
Order differences are accepted only for streams explicitly declared `unordered_streams` in cases.json.
## Suggested fix
Make strict the default: in `compare_case` change `if stream in unordered or not strict_order:` to `if stream in unordered:`; add `"unordered_streams": ["stdout"]` to cir/subckt_tests and opt/vci (clang reorders them per README); keep a `--lenient-order` flag for cross-compiler runs. Update README "Known nondeterminism" and CLAUDE.md. Cannot be verified for the Debug/clang builds from here, hence needs-design.
## Golden impact
none today (Release passes strict). Behaviour of `check` becomes stricter on other compilers.
## Related
B-127, B-128, B-129.

- Review ids: F-A5-06.
## Verification
Ran the repro (above).
