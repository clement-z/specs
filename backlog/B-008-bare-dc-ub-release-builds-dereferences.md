# B-008: Bare `.dc` is UB in Release builds (dereferences `begin()` of an empty map)
severity: med | category: bug | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
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

- Discovery seeds: S1.24 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-08.
## Verification
Ran the repro (E549). With the patch: the message is printed, rc=1, golden unchanged.
