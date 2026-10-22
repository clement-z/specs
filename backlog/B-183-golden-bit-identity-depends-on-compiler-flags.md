# B-183: Bit-identical golden outputs only hold for gcc with `-march=native` (FMA); other compilers and flags differ in the last bits
severity: low | category: determinism | effort: S | fix-class: needs-human | verdict: CONFIRMED | status: fixed (option (c), see Resolution)
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

## Resolution (2026-10-22)
Maintainer decision: **(c)**, with `native` off by default (`meson.options`) and a pixi command for native builds
(`pixi run build-native`, toolchains `gcc-native`, `clang-native`). The whole baseline was re-recorded from the pixi
gcc 15 release build with the project defaults. Audit: before recording, that build passed the previous baseline at
the default tolerances and at `--rtol 1e-12 --atol 0 --print-slack 0`; old vs new baseline differ strictly in exactly
the 28 cases above (floats, last 1-2 digits; a VCD only records changes, so a last-bit flip can add or drop a line),
plus the ignored stdout of 8 error/regress cases (`ignore_streams`) that had gone stale since F-05.

Strict check (`--rtol 0 --atol 0 --print-slack 0`) against the new baseline, fresh clone:

| build | strict failures |
|---|---|
| system gcc 11.5 and pixi gcc 15.3, release (default) | 0 |
| system clang 19.1 and pixi clang 21.1, release | 6, one `.dc` sweep point (`cir/add_drop`, `opt/tol-resolution-overrides`, `opt/mode-fd`, `opt/multi-file`, `regress/b004-mode-td-dc`, `regress/f05-format-vcd-csv-dc`), <= 1e-14 relative |
| pixi ASan+UBSan debug, gcc and clang | the same 6 |
| pixi gcc 15, `-Dnative=true` | 28 (the list above) |

All pass at the default tolerances and with `--atol 0`. The remaining 6 differ without FMA, so they come from how
compilers and optimisation levels evaluate a few operations (e.g. math-function calls); not investigated further.
