# B-075: GenericTransmissionDevice: silence the -Wmaybe-uninitialized false positive on `delay`
severity: low | category: build | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/devices/generic_transmission_device.cpp:163
## Problem
GCC warns `'delay' may be used uninitialized` at generic_transmission_device.cpp:221 on every build. The warning is a false positive: `delay` is always assigned (:183) before the loop ends, but the `volatile bool init_done` defeats the flow analysis.
## Evidence
In my scratch Release build log: `generic_transmission_device.cpp:221:43: warning: 'delay' may be used uninitialized in this function [-Wmaybe-uninitialized]`.

Additional evidence (independent report F-A7-07, A7 reviewer):

`/tmp/ws-review-ninja.log`: `generic_transmission_device.cpp:221:43: warning: 'delay' may be used uninitialized [-Wmaybe-uninitialized]`; reproduced with the exact build flags (`-O3 -DNDEBUG -march=native -std=gnu++20`).
## Expected
A warning-free build.
## Suggested fix
```
src/devices/generic_transmission_device.cpp:163
-    double delay;
+    double delay = 0;
```
Drop it if B-069 deletes the class.
## Golden impact
None.
## Related
B-069.

- Discovery seeds: S4.15 (see `backlog/README.md` for seed numbering).
- Merged duplicate reports: F-A7-07 (A7).
- Review ids: F-A3-29, F-A7-07.
## Verification
Build log and code reading. This is safe because the initial value is always overwritten before use.
