# B-108: Golden baseline is bit-exact only for -march=native (FMA) builds; planned CI uses native=false
severity: med | category: build | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: fixed in 676f915 (sprint 2026-10c, B-183): baseline re-recorded without -march=native, native off by default
location: CMakeLists.txt:37, discover/meson/PLAN.md:26-30,62,64,101,119, tests/golden/run_golden.py (tolerances)
## Problem
`-march=native` is added unconditionally (CMakeLists.txt:37) and the baseline was recorded on an AVX2+FMA CPU, so GCC contracts `a*b+c` into `vfmadd*`. A build without `-march=native` (what the planned GitHub Actions job does with `-Dnative=false`, PLAN.md:101) is NOT bit-identical to the baseline: 12 of 44 golden cases differ in the last printed digit at `--rtol 0 --atol 0`. They only pass with the default tolerance (rtol 1e-9). PLAN.md:119 asks for `check --atol 0` (rtol stays at its 1e-9 default), which works, but anyone who "tightens" CI to `--rtol 0` will get red CI on every non-FMA runner (and on aarch64 or older x86). obsoleted-by-meson: no (the Meson `native` option carries this over).
Second, PLAN.md:26-30 gives the wrong reason for the numerical risk: "ISO `-std=c++20` disables contraction". On GCC the ISO-mode default of `-ffp-contract=off` applies to C only; for C++ the default stays `fast` (shown below). `cpp_std=gnu++20` is still the right choice, but the thing that actually changes numerics is `-march=native` (FMA availability), not `gnu++` vs `c++`.
## Evidence
```
# Release build of specs without -march=native (scratch copy of CMakeLists.txt with that line deleted), same GCC 11.5:
objdump -d specs-generic | grep -c vfmadd      -> 0      (native build /tmp/ws-review-build/specs: 123)
run_golden.py check --specs specs-generic --baseline tests/golden/baseline -j 2                 -> 44 passed
run_golden.py check ... --atol 0                                                                -> 44 passed
run_golden.py check ... --atol 0 --rtol 0                                                       -> 12 FAIL:
  cir/add_drop cir/add_drop_pyspecs cir/subckt_tests opt/tracefile-o opt/tol-resolution-overrides opt/mode-sampled-time
  opt/mode-fd opt/multi-file opt/vci tb/ac_add_drop tb/ac_crow tb/lambda
  e.g. crow_tb.vcd pcross.abs: r0.006092776066760333 != r0.006092776066760329   (1 ulp)
run_golden.py check --specs /tmp/ws-review-build/specs ... --atol 0 --rtol 0                    -> 44 passed (native build is bit-exact)
# GCC 11.5, a*b+c, -O2 -march=native:  -std=c++20 -> vfmadd (1)   -std=gnu++20 -> (1)   -std=c++17 -> (1)   -ffp-contract=off -> (0)
```
Repro: `backlog/repro/a7-golden-native-vs-generic.sh`.
## Expected
CI on a portable (non-native) build is green and stays green; the docs state which comparison mode is valid for which build.
## Suggested fix
1. In the Meson CI job (GitHub and GitLab) run the golden check with the default `--rtol`, never `--rtol 0`; keep `--atol 0`.
2. Add one sentence to `tests/golden/README.md`: "bit-exact (`--rtol 0`) only for `-march=native` FMA builds; non-native builds differ by 1 ulp".
3. Correct the contraction rationale in PLAN.md (and in DECISIONS.md when the Meson workstream writes it): the driver is `-march=native`/FMA, not `-std=c++20` vs `gnu++20`.
4. Do NOT add `-ffp-contract=off` to make the baseline portable without sign-off: it changes results of every case (CLAUDE.md: compiler-flag/numerics changes need `needs-human`), and the baseline would have to be re-recorded.
## Golden impact
none for 1-3. (4 would change the last digit in 12 cases and need a re-record.)
## Related
B-116 (Meson-port checklist), S4.11. discover/04 §9.1 and §10 (unknown "why Makefile -O2 build is bit-identical": answer is the one above, c++17 also contracts).

- Discovery seeds: S4.11 (see `backlog/README.md` for seed numbering).
- Review ids: F-A7-01.
## Verification
Built `specs` (Release, -j2) from a scratch copy without `-march=native` and ran the golden harness at the three tolerance settings; counted `vfmadd` with objdump; compiled a 1-line FMA test at three `-std` values.
