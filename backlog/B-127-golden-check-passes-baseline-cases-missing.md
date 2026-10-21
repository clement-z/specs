# B-127: golden `check` passes when baseline cases are missing, when gtest is skipped, or when `--only` matches nothing
severity: med | category: test-gap | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: fixed in bfd6a15 (sprint 2026-10b P1)
location: tests/golden/run_golden.py:964-975 (NEW/MISSING), :1002-1009 (gtest SKIPPED), :1013-1022 (`summarise`), :484-488 (`select`), CLAUDE.md "Testing"
## Problem
Without `--strict` (not in the CLAUDE.md command) these conditions exit 0: a case present in cases.json but absent from the baseline directory ("NEW"), the gtest case when `--unit-tests` is not given ("SKIPPED"), and `--only <regex>` matching no case ("0 passed"). Deleting `tests/golden/baseline/cases/<group>/<name>` therefore removes that case's protection without any failure. Because `unit_tests` cannot be built in-tree (B-123), the gtest baseline is never exercised by agents following CLAUDE.md. Neither the golden harness nor any `specs -t` runs in `.gitlab-ci.yml`.
## Evidence
`sh backlog/repro/a5-golden-holes.sh <repo>` parts 2 and 3:
```
NEW      tb/wg (not in baseline)        Summary: 2 passed, 0 failed, 1 new   exit=0
--only no_such_case                      Summary: 0 passed ... exit=0
```
## Expected
Missing baseline entries and empty selections fail; skipping gtest is explicit.
## Suggested fix
Make NEW fail by default (add `--allow-new` for the record workflow), exit 2 when `select()` returns no case, and, once B-123 is merged, change the CLAUDE.md command to `check ... --unit-tests build/src/test/unit_tests --strict`.
## Golden impact
none.
## Related
B-126.

- Review ids: F-A5-07.
## Verification
Ran the repro; read `summarise`.
