# B-068: Delete unconstructible dead classes: `Ring`, `PowerProbe`, `PhaseProbe`
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/devices/ring.h, src/devices/ring.cpp, src/devices/alldevices.h:33, src/devices/probe.h:82-90, src/devices/probe.cpp:50-74
## Problem
`Ring` cannot be used.
- It declares `on_port_in_changed()` (ring.h:59) without defining it, and the constructor's `SC_THREAD` would need it.
- It uses `SC_HAS_PROCESS(Waveguide)` (:85).
- Its default length is 30e-6 cm (:66), and it sets the through phase to π/2 (:45).
- It never applies its `m_*` parameters, and it names children with '.'.
- ring.cpp does not even include ring.h.
- Nothing constructs it: the ring tb builds DC+WG by hand.

`PowerProbe`/`PhaseProbe` have no constructor, and `Probe` has no default constructor, so they cannot be instantiated. Nothing references them.
## Evidence
grep: `Ring` is only included via alldevices.h:33. `PowerProbe`/`PhaseProbe` appear only in probe.h/probe.cpp.
## Expected
No dead code.
## Suggested fix
- Delete src/devices/ring.h and src/devices/ring.cpp, and the `#include "devices/ring.h"` at alldevices.h:33. CMake globs sources, so no list edit is needed; check the Makefile and Meson lists if they enumerate files.
- Delete probe.h:82-90 and probe.cpp:50-74.
## Golden impact
None.
## Related
B-069.

- Discovery seeds: S2.24 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-22.
## Verification
Code reading and grep. This is safe because none of the symbols is reachable or constructible.
