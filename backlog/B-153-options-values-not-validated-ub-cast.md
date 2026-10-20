# B-153: `.options` values are not validated (UB cast, out-of-range timescale, non-positive tolerances)
severity: med | category: parser | effort: S | fix-class: easy-unattended | verdict: CONFIRMED
location: src/parser/parse_directive.cpp:30-37
## Problem
- `RESOLUTION` stores a double into the `uint64` `default_resolution_multiplier`. Negative or huge values are UB, and
  0.5 truncates to 0.
- `TS/TIMESCALE` casts any int to the enum.
- `ABSTOL/RELTOL` accept values <= 0.
- The only checks are the `verifyConfig()` asserts (specs.h:138-146), which do nothing under NDEBUG. The CLI
  `--abstol/--reltol` is validated (main.cpp:321,332); the netlist path is not.
## Evidence
`backlog/repro/a4-options.sh <specs>` on Release:
```
resolution=-1   rc=0  resolution multiplier: 18446744073709551615 | Simulated 10 ps
resolution=0.5  rc=0  resolution multiplier: 0 | Simulated 10 ps
resolution=1e30 rc=0  resolution multiplier: 0 | Simulated 10 ps
timescale=3     rc=1  E705 invalid tracing timescale unit set: not a power of ten: 1000000000000000000 fs
timescale=-20   rc=1  E705 ... 0 fs
timescale=-12.7 rc=0  engine timescale -12 (silently truncated)
abstol=-1       rc=0  abstol: -1
reltol=0        rc=0  reltol: 0
```
Debug: rc 134 (verifyConfig assert) for all but timescale=-12.7.
## Expected
A clean error for values that `verifyConfig` would reject.
## Suggested fix
In `OPTIONSDirective::create`:
- ABSTOL/RELTOL: `double v = as_double(); if (!(v > 0))` → error.
- TS: `int v = as_integer(); if (v < SPECSConfig::ONE_FS || v > SPECSConfig::ONE_SEC)` → error.
- RESOLUTION: `double v = as_double(); if (!(v >= 1 && v < 18446744073709551616.0))` → error, before the cast.

Message: `.options: <kw> must be ... (got <value>)`, then exit(1). The same constraints as `verifyConfig`, so there is
no design choice. Rejecting non-integer timescale/resolution would be an extra decision; leave it out.
## Golden impact
None (verified; golden values are valid, and `-R -14` goes through the same footer).
## Related
B-173 (duplicate TRACEALL branch). Engine area: `verifyConfig` should also stop using assert. CLI area:
`main.cpp:351` accepts `-R -20`, which would then hit this new message.

- Complementary: B-018 turns the `verifyConfig` asserts into errors and bounds `-R`.

- Discovery seeds: S1.27 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-12.
## Verification
Patched scratch build: the 7 invalid values give rc 1 with a message. Golden cir/cir-json/opt/cli are unchanged.
