# B-149: `X1` with no arguments segfaults (args.back() on empty vector)
severity: med | category: parser | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/parser/parse_tree.cpp:841
## Problem
`xelem->args.back()` is called without checking that `args` is non-empty. `x1` alone or `x1 L=1` (kwargs only)
crashes.
## Evidence
`backlog/repro/a4-x-noargs.cir` (`x1`): rc 139 (SIGSEGV) in both builds, after `Flattening ROOT`.
## Expected
`Error: subcircuit instance ROOT/X1 has no subcircuit name`, rc 1.
## Suggested fix
Before :841:
```
        if (xelem->args.empty())
        {
            cerr << "Error: subcircuit instance " << xelem->name << " has no subcircuit name" << endl;
            exit(1);
        }
```
## Golden impact
None (verified).
## Related
B-155.

- Discovery seeds: S3.07 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-08.
## Verification
Patched scratch build: both `x1` and `x1 L=1` give the message and rc 1.
