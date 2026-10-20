# B-141: legacy testbenches: unused variables (compiler warnings), dead code, copy-paste message bugs, sticky stream flags
severity: low | category: style/maintainability | effort: S | fix-class: easy-unattended | verdict: CONFIRMED
location: src/tb/freqsweep_tb.cpp:60-62,66,148-213; src/tb/freqsweep_tb.h:32; src/tb/lambda_tb.cpp:6,21-22; src/tb/crow_tb.cpp:21-30,42-50,113,120-121,138-157,178-190; src/tb/merger_tb.cpp:73; src/tb/phase_shifter_tb.cpp:48-60; src/tb/pcm_device_tb.cpp:11; src/tb/directional_coupler_tb.cpp:6,82; src/tb/wg_tb.cpp:6; src/tb/mesh_tb.cpp (double init: reviewer A3)
## Problem
`g++ -Wall -Wextra` on src/tb reports `unused variable` for freqsweep_tb.cpp:60,61,62 (`event_counter`, `success_counter`, `test_number` in a `monitor()` whose body is `wait(); continue;`) and lambda_tb.cpp:21,22. Other items:
- freqsweep_tb.h:32 declares `freqsweep_tb_run_crow()`, which exists only in a 65-line commented-out block (freqsweep_tb.cpp:148-213).
- crow_tb.cpp: header comment copied from freqsweep ("specs -t ac_add_drop", :21-30); eight successive overwrites of `lambda` (:42-50); `if (true || (verbose && i % ...))` (:113) prints every sweep step; `monitor()` has dead code after `continue` (:138-157) and `if (false) sensitive << ...` in the header; lines 178-185 set ring length, loss, coupling, neff, ng that lines 186-190 overwrite (`setRingLength` only assigns `m_ring_length`, crow.h:92-96); `pc` is leaked. `cout << fixed << setprecision(20)` (:120-121) is never reset, so after an FD sweep `do_test` prints `Total runtime: 14435.43800000000010186341 ms`.
- merger_tb.cpp:73 failure text says "Expected 2W" for a check that expects 4 W.
- phase_shifter_tb.cpp:48 and :60 are the same check (`arg == 2`); message at :44 says "0.630210W as output field" for a modulus in V/m.
- pcm_device_tb.cpp:11 comment "to 2W" for a 2 mW value.
- directional_coupler_tb.cpp:82 unused `IN3, IN4`; :6 unused include `devices/generic_2x2_coupler.h`; wg_tb.cpp:6 and lambda_tb.cpp:6 unused `devices/generic_waveguide.h`.
- freqsweep_tb.cpp:153 and :156: probe named `"ptrough"` (typo; appears in the VCD scope name, so changing it changes the golden VCDs of tb/ac_add_drop).
## Evidence
```
$ g++ -std=gnu++20 -Wall -Wextra -DBUILD_TB=1 -fsyntax-only ... src/tb/*.cpp
tb/freqsweep_tb.cpp:60:18: warning: unused variable 'event_counter'  (also :61, :62)
tb/lambda_tb.cpp:21:18 / :22:24 warning: unused variable 'success_counter' / 'test_number'
```
## Expected
No warnings, no dead blocks, accurate messages.
## Suggested fix
Delete the unused variables and dead blocks listed above; fix the two message strings and the duplicate check (remove the second); add `std::cout.unsetf(std::ios::fixed); std::cout << std::setprecision(6);` (default flags) after the sweep in `crow_tb::run_fd`, or use a local `std::ostringstream`. Do NOT rename `ptrough` (changes VCD scope names) and do not touch mesh_tb (A3).

Exclude the unused variables at `freqsweep_tb.cpp:60-62` and `lambda_tb.cpp:21-22`: they are already covered by B-111. Whichever lands first, the other skips those lines.
## Golden impact
none: all golden cases run the event-driven path and none of the edited strings is printed in a passing run. Verify with `run_golden.py check` (the stdout of tb/ac_crow in default mode is unaffected; the sticky flags only act in FD mode).
## Related
B-019 (check helper makes most of the duplicated blocks disappear).

- Review ids: F-A5-21.
## Verification
Compiler output above; read every file. Safe because every removed item is unreferenced or overwritten before use.
