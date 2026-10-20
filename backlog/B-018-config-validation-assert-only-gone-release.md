# B-018: Config validation is assert-only (gone in Release), and `-R` has no lower bound
severity: low | category: cli | effort: S | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 587c4de
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

Correction from the second check: in step (2) **keep** the existing `verifyConfig()` call in `prepareSimulation()` (specs.cpp:363): all 13 testbench files and `src/test/devices/waveguide.cpp:152` call `prepareSimulation()` directly. Add a second call at the top of `runAnalysis()`, before `applyEngineResolution()`.
## Golden impact
None for the golden cases (44/44, 0 notes). Behaviour change, stated explicitly: in Release these inputs go from running (rc 0) to an error: `.options abstol=0`, `reltol=0`, `resolution=0`, `resolution=0.5` (cast to 0) and `timescale=1`. They violate the existing assert contract (which already aborts them in Debug) and the CLI `--abstol`/`--reltol` validators. Complementary to B-153, which rejects the same values at parse time.
## Related
B-008.

- Complementary: B-153 validates `.options` values at parse time.

- Discovery seeds: S1.27 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-18.
## Verification
Ran both repros. A patched `-R` check gives "Invalid resolution value", rc=1, golden unchanged. Safe because only
invalid inputs change behaviour, and they already fail or misbehave today.

**Independent second check (V3, of F-A1-18): ADJUST.** Method: applied (1)+(2)+(3) exactly, rebuilt, golden check --atol 0; ran -R -16 and 9 `.options` variants on base and patched; grepped callers of prepareSimulation/verifyConfig; also tested a variant that keeps the old call

The problem is reproduced. Base accepts `.options abstol=-1 reltol=0` (rc=0), and `-R -16` / `.options timescale=-16` fail with E705. Patched: clear errors, rc=1. Golden: 44/44, 0 notes. Change step (2): do NOT remove verifyConfig() from prepareSimulation (specs.cpp:363). All 13 testbench files (src/tb/*_tb.cpp) and src/test/devices/waveguide.cpp:152 call prepareSimulation() directly, never runAnalysis(), so removing it drops all validation for them. Instead, add a call at the top of runAnalysis() and keep the existing one. Validated: 44/44, 0 notes. Golden impact text should add that these inputs go from rc=0 today (Release) to an error: `.options abstol=0`, `reltol=0`, `resolution=0`, `resolution=0.5` (cast to 0), `timescale=1`. They violate the existing assert contract and the CLI --abstol/--reltol validators, so this is acceptable, but say so explicitly. Coordinate with A4 seed S1.27 (parse_directive casts / verifyConfig).

**Orchestrator:** Kept the prepareSimulation() validation call; golden-impact text now lists the inputs that become errors.
