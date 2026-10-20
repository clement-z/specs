# B-111: Six unused local variables in two legacy testbenches and Generic2x2Coupler
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/tb/freqsweep_tb.cpp:60-62, src/tb/lambda_tb.cpp:21-22, src/devices/generic_2x2_coupler.h:35
## Problem
`-Wunused-variable` (GCC 11, every build): `event_counter`, `success_counter`, `test_number` in `freqsweep_tb::monitor()` (the whole body is `while(true){wait();continue;}`), `success_counter`, `test_number` in `lambda_tb::monitor()`, and `lambda0` in `Generic2x2Coupler::prepareTM()` (the real lambda0 lives in `TM.lambda0`, unrelated). The header warning is emitted by every TU that includes it. Survives the migration.
## Evidence
```
freqsweep_tb.cpp:60: unsigned int event_counter = 0;   :61 unsigned int success_counter = 0;   :62 const unsigned int test_number = 3;
lambda_tb.cpp:21: unsigned int success_counter = 0;    :22 const unsigned int test_number = 0;
generic_2x2_coupler.h:35: double lambda0 = 1550e-9;
```
(`lambda_tb::monitor` still uses `event_counter`; keep it.)
## Expected
No warnings.
## Suggested fix
Delete those six lines (no other references: `grep -n "test_number\|success_counter" src/tb/freqsweep_tb.cpp src/tb/lambda_tb.cpp` shows only the declarations).
## Golden impact
none.
## Related
B-027, B-112, B-075 (other warnings from the same log; may be one "fix: silence -Wall warnings" series).

- Overlap: `generic_2x2_coupler.h:35` is also listed in B-045.

- Review ids: F-A7-04.
## Verification
Applied on a scratch copy; `g++ -fsyntax-only -Wall -Wextra` of freqsweep_tb.cpp, lambda_tb.cpp, directional_coupler_tb.cpp: 3+2+1 warnings before, 0 after. Pure deletion of unused locals.
