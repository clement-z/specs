# B-008: Bare `.dc` is UB in Release builds (dereferences `begin()` of an empty map)
severity: med | category: bug | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in ff391b2
location: src/specs.cpp:149, src/parser/parse_analysis.cpp:34
## Problem
`.dc` without a sweep order leaves `cw_sweep_orders` empty. The only guard is an `assert`, which Release
(`-DNDEBUG`, the build CLAUDE.md mandates) removes. `*cw_sweep_orders.begin()` then reads the map header, and
in practice throws from `vector::reserve`. The second half of the seed ("only the first sweep runs") is wrong:
parser.y:405-409 rejects a second sweep order.
## Evidence
`backlog/repro/a1-dc-bare.cir` → `Error: (E549) uncaught exception: vector::reserve`, rc=1 (Release binary).
## Expected
A clear error.
## Suggested fix
specs.cpp, before line 149:
```cpp
    if (cw_sweep_orders.empty())
    {
        cerr << "Error: .dc analysis requires a sweep order (e.g. .dc /cwsrc1/WL start stop step)" << endl;
        exit(1);
    }
```
Also replace `assert(sweep_orders.size() > 0);` at parse_analysis.cpp:34 with the same message and `exit(1)`,
so the error happens before simulation.
## Golden impact
None (validated).
## Related
B-018 (asserts compiled out).

- B-148 converts the other user-input asserts of the parser.

- The parser asserts at `parse_analysis.cpp:34` were also reported by the A4 reviewer (B-148, seed S3.06); B-148 skips that site.

- Discovery seeds: S1.24 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-08.
## Verification
Ran the repro (E549). With the patch: the message is printed, rc=1, golden unchanged.

**Independent second check (V3, of F-A1-08): CONFIRMED.** Method: ran backlog/repro/a1-dc-bare.cir (base: E549 vector::reserve, rc=1); applied both suggested changes, rebuilt, golden check --atol 0

Patched: the parse_analysis.cpp check fires first, during BUILDING CIRCUIT. It prints "Error: .dc analysis requires a sweep order (e.g. .dc /cwsrc1/WL start stop step)" with rc=1. The specs.cpp guard is then only reachable for API users. Golden: 44/44, 0 notes. The behaviour change is limited to the invalid input: `-n` (dry run) on a bare .dc now exits 1 (Release used to exit 0). parser.y:400-411 accepts a bare `.dc` and rejects a second sweep, as stated. Overlap: parser seed S3.06 (A4, "user-input validation with assert", which cites parse_analysis.cpp:34 and the bare .dc) covers the same assert. No A4 finding is in findings.json yet, so dedupe if A4 files one. B-112 edits parse_analysis.cpp:39,78 nearby (trivial conflict).
