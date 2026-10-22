# B-006: `-o ""` segfaults (null `default_trace_file` dereferenced)
severity: med | category: bug | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in eee3be8
location: src/specs.cpp:366-369
## Problem
With an empty `trace_filename`, no VCD is created, but line 369 calls `default_trace_file->set_time_unit(...)`
anyway. The rest of the code already treats a null trace file as "no tracing" (lines 261, 374).
## Evidence
`specs -o "" -f backlog/repro/a1-tran-no-analysis.cir` → `Segmentation fault`, rc=139 (`backlog/repro/a1-cli.sh`).
## Expected
The simulation runs without a VCD.
## Suggested fix
specs.cpp:369:
```diff
-        default_trace_file->set_time_unit(std::pow(10, 15 + engine_timescale), SC_FS);
+        if (default_trace_file)
+            default_trace_file->set_time_unit(std::pow(10, 15 + engine_timescale), SC_FS);
```
## Golden impact
None (validated).
## Related
B-007.

- Discovery seeds: S1.11, S4.13 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-06.
## Verification
Ran the repro (rc=139). With the patch: rc=0, a netlist with an explicit PROBE also runs fine, and golden passes.
Safe because both later users of the pointer already null-check it.

**Independent second check (V3, of F-A1-06): CONFIRMED.** Method: applied the exact diff on the scratch copy, rebuilt, golden check --atol 0, ran the repro and probe netlists with -o ""

Base: rc=139 (SIGSEGV). Patched: rc=0, "Simulated 3300 fs". Netlists with PROBE+MLPROBE+PDET (.tran) and PROBE (.dc) also run, rc=0. Golden: 44/44, 0 notes. Every other user of default_trace_file in the CLI path is after the null check at specs.cpp:261/374. Testbenches always set a trace_filename.
