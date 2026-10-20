# B-075: `double delay;` may be used uninitialised in GenericTransmissionDevice (-Wmaybe-uninitialized)
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: deferred to ws-cleanup (style)
location: src/devices/generic_transmission_device.cpp:163, :221
## Problem
`delay` is assigned only inside the `while (!init_done)` loop (line 183); `init_done` is `volatile`, so GCC cannot prove the loop runs and warns at line 221 (`sc_time(delay, SC_SEC)`). In practice the second loop is reachable only after `init_done = true`, which is set after `delay = Tij.tau`, so this is a false positive, but it shows up in every Release build.
## Evidence
`/tmp/ws-review-ninja.log`: `generic_transmission_device.cpp:221:43: warning: 'delay' may be used uninitialized [-Wmaybe-uninitialized]`; reproduced with the exact build flags (`-O3 -DNDEBUG -march=native -std=gnu++20`).

Additional evidence (independent report F-A3-29, A3 reviewer):

In my scratch Release build log: `generic_transmission_device.cpp:221:43: warning: 'delay' may be used uninitialized in this function [-Wmaybe-uninitialized]`.
## Expected
No warning.
## Suggested fix
Line 163: `double delay;` -> `double delay = 0.0;`.
## Golden impact
None. The golden suite cannot reach this code: no class derived from `GenericTransmissionDevice` is instantiated by any netlist or testbench (the only uses, `tb/wg_tb.cpp:106` and `tb/directional_coupler_tb.cpp:110`, are commented out; see B-069).
## Related
Devices reviewer: line 157 prints `"<i>, <j> is active"` to stdout at thread start (debug noise), and `volatile bool init_done` is unnecessary. Not filed here.

- If B-069 deletes the GenericTransmissionDevice family, this fix becomes moot (caveat from the duplicate report B-075).

- Discovery seeds: S4.15 (see `backlog/README.md` for seed numbering).
- Merged duplicate reports: F-A3-29 (A3).
- Review ids: F-A7-07, F-A3-29.
## Verification
Compiled the original and the patched file with the build's flags: 1 warning -> 0.

**Independent second check (V2, of F-A7-07): CONFIRMED.** Method: `double delay;` -> `double delay = 0.0;` at generic_transmission_device.cpp:163, rebuilt, compared warnings, golden check; also compared object code with the F-A3-29 variant (`= 0`)

Baseline `generic_transmission_device.cpp:221:43: 'delay' may be used uninitialized [-Wmaybe-uninitialized]` is gone, nothing new. Code reading confirms the claim: loop 1 exits only through `init_done = true`, which is set right after `delay = Tij.tau`; `continue` on NaN wavelength re-enters the loop. Golden impact "none" is correct but cannot be demonstrated by golden: no class derived from GenericTransmissionDevice is instantiated anywhere (the only uses, `tb/wg_tb.cpp:106` and `tb/directional_coupler_tb.cpp:110`, are commented out; see B-069). So the Golden-impact sentence "exercised by testbenches/the generic couplers" should read "not instantiated by any netlist or testbench; golden suite cannot reach it". 44/44 pass at atol=rtol=0. PREFERRED over F-A3-29 (see below).

**Orchestrator:** Kept as the primary report (the second check preferred `= 0.0`); F-A3-29 merged into it.

**Independent second check (V2, of F-A3-29): CONFIRMED.** Method: `double delay = 0;` at generic_transmission_device.cpp:163, rebuilt, compared warnings, golden check, byte-compared the object file with the F-A7-07 variant

Works identically: warning gone, no new warnings, 44/44 golden. The object files produced by `= 0` and `= 0.0` are byte-identical (cmp), so the two fixes are the same change. This is a duplicate of F-A7-07: merge them into one backlog item and keep F-A7-07's `= 0.0` (matches the type); F-A3-29's "drop it if B-069 deletes the class" caveat is worth keeping in the merged item. Not a design choice.
