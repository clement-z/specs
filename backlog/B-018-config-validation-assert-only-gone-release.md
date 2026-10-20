# B-018: Config validation is assert-only (gone in Release), and `-R` has no lower bound
severity: low | category: cli | effort: S | fix-class: easy-unattended | verdict: CONFIRMED
location: src/specs.h:138-146, src/specs.cpp:43-44,363, src/main.cpp:351
## Problem
`verifyConfig()` is only `assert`s, so Release accepts `.options abstol=-1 reltol=0` silently. `-R` only rejects
values > 0, so `-R -16` reaches `applyEngineResolution` and fails later with a cryptic error.
`verifyConfig` also runs after `applyEngineResolution`.
## Evidence
`-R -16` → `Error: (E705) invalid tracing timescale unit set: not a power of ten: 0 fs`.
`.options abstol=-1 reltol=0` → printed in the config dump and accepted, rc=0.
## Expected
A clear error for out-of-range values before anything is applied.
## Suggested fix
(1) In specs.h, turn each assert into `if (!(cond)) { cerr << "Error: invalid configuration: <what>" << endl; exit(1); }`
(abstol>0, reltol>0, multiplier>=1, ONE_FS<=timescale<=ONE_SEC, mode and analysis in range).
(2) In runAnalysis, call `verifyConfig()` first, before `applyEngineResolution()`, and remove the call at
specs.cpp:363.
(3) main.cpp:351: `|| timescale_val > 0 || timescale_val < -15`.
## Golden impact
None (no golden case uses invalid values; the `-R` bound was validated).
## Related
B-008.

- Discovery seeds: S1.27 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-18.
## Verification
Ran both repros. A patched `-R` check gives "Invalid resolution value", rc=1, golden unchanged. Safe because only
invalid inputs change behaviour, and they already fail or misbehave today.
