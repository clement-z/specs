# B-115: `-Wfatal-errors` is applied globally (CMake now, and planned for Meson) and hides every error after the first
severity: low | category: build | effort: XS | fix-class: needs-design | verdict: CONFIRMED
location: CMakeLists.txt:38, discover/meson/PLAN.md:62
## Problem
With `-Wfatal-errors` the compiler stops at the first error per TU, so the real number of problems is never visible: discover/04 §1 records that the second `unit_tests` error (`EXPECT_ANGLE_EQ`/`EXPECT_PRED_FORMAT2` misuse) was masked by the `static_assert(false)` in `src/test/utils.h:81`. For unattended agents and CI logs this multiplies fix/rebuild round trips. obsoleted-by-meson: no, PLAN.md:62 copies it into the Meson global args.
## Evidence
`CMakeLists.txt:38  target_compile_options(common INTERFACE -Wfatal-errors)`; PLAN.md:62 "Global args: `-Wfatal-errors`, ...".
## Expected
Full error list by default; fail-fast only if asked.
## Suggested fix
Do not carry it into `meson.build` (or put it behind a `fatal_errors` option, default false). Remove it from CMakeLists.txt:38 only if CMake is still around.
## Golden impact
none.
## Related
B-116.

- Review ids: F-A7-12.
## Verification
Code reading of the two cited lines plus the discovery note.
