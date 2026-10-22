# B-183: Bit-identical golden outputs only hold for gcc with `-march=native` (FMA); other compilers and flags differ in the last bits
severity: low | category: determinism | effort: S | fix-class: needs-human | verdict: CONFIRMED | status: open
location: tests/golden/baseline (recorded with gcc, `-march=native`); meson.build (`-march=native`, `cpp_std=gnu++20`)

## Problem
Found in phase PF (pixi environment, 2026-10-22). Every build below passes the golden check at the default tolerances
and at `--atol 0` (212/212, nothing re-recorded), but with `--rtol 0 --atol 0 --print-slack 0` only the first one is
bit-identical to the baseline (Skylake host, release builds of the same commit):

| build | strict failures | smallest `--rtol` (atol 0, slack 0) that passes |
|---|---|---|
| system gcc 11.5, `-march=native` (plain route) | 0 | 0 |
| pixi gcc 15.3, `-march=native` | 5: `regress/b031-mzi-bidir`, `tb/ac_add_drop`, `tb/ac_add_drop-fd`, `tb/ac_crow`, `regress/b131-nrings-crow` | 1e-13 |
| system gcc 11.5 and pixi gcc 15.3, `-Dnative=false` (as in CI) | 28 (the same for both; the two are bit-identical to each other) | not measured |
| system clang 19.1 and pixi clang 21.1, `-march=native` | 23 (the same for both; bit-identical to each other) | 1e-12 |

The differences are in the last one or two digits of traced doubles (`0.906001739533381` vs `0.9060017395333809`). Cause
(most likely): `gnu++20` lets gcc contract `a*b+c` into FMA (`-ffp-contract=fast`) where the target has FMA, so the
results depend on the target (`-march`) and on the compiler's contraction choices; clang contracts less by default.
So the baseline is bit-exact for one compiler family plus one ISA level, not a property of the source.

## Expected
A documented definition of what "bit-identical" means for the golden check across compilers, e.g. one of:
(a) keep it as is: bit-identity only between builds of the same compiler and flags (the default tolerances are the
cross-compiler check); (b) compile with `-ffp-contract=off` so results stop depending on `-march` and on the compiler
(changes the outputs of every native build in the last bits: a re-record); (c) record the baseline with
`-Dnative=false`.

## Suggested fix
Maintainer decision ((b) changes simulation results in the last bits and needs sign-off). Until then the gates that
require "bit-identical" should compare builds of the same compiler and flags (before/after), as the F-15 phases do.

## Golden impact
None until decided; (b) or (c) re-record the whole baseline.

## Related
F-15 (D2/D3 compare outputs `--rtol 0 --atol 0 --print-slack 0` between builds of the same toolchain), `src/utils/rng.cpp`
(already avoids an FMA-dependent last bit), phase PF report.
