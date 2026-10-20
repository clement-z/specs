# B-036: --vci printing segfaults when a port's channel is not spx::oa/ea_signal_type
severity: low | category: bug | effort: S | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in fb97125
location: src/devices/waveguide.cpp:44-46, src/devices/directional_coupler.cpp:31-38, src/devices/crossing.cpp:25-31, src/devices/phaseshifter.cpp:17-22, src/devices/merger.cpp:15-17, src/devices/splitter.cpp:15-17, src/devices/pcm_device.cpp:23-25, src/devices/probe.cpp:14, src/devices/cw_source.cpp:14, src/devices/value_list_source.cpp:22, src/devices/electrical_value_list_source.cpp:22, src/devices/bitstream_source.cpp:37
## Problem
Every verbose print does `(dynamic_cast<spx::oa_signal_type *>(p.get_interface()))->name()`: 51 sites in 11 device
files, plus a variant at bitstream_source.cpp:37. The cast is null in two cases:
- the port is bound to any other channel type, e.g. `sc_signal<OpticalSignal>` with the default writer policy, as used
  by CROW;
- an input port is unbound (input ports are `SC_ZERO_OR_MORE_BOUND`, specs.h:44,49).

`->name()` on that null pointer is a segfault. Netlist runs are not affected (all their nets are
`oa_signal_type`/`ea_signal_type`), but C++ composites and testbenches are.
## Evidence
`backlog/repro/a2-vci-crow.sh` runs `specs --vci -t ac_crow`. It prints `wg_bot_0:` ... `phase delay = 0.00129385 ns
(1.5708rad @1.55)` and then dies with exit 139. `specs -t ac_crow` without `--vci` exits 0. All other tbs exit 0
with `--vci`.
## Expected
`--vci` should print channel names (or `<unbound>`) and never crash.
## Suggested fix
Append to src/utils/sysc_utils.h (it is included everywhere via specs.h):
```cpp
// Name of the channel bound to a port, or "<unbound>" (used by --vci printing)
template<typename P>
inline const char *port_channel_name(P &port)
{
    auto *obj = dynamic_cast<sc_object *>(port.get_interface());
    return obj ? obj->name() : "<unbound>";
}
```
Then replace the 51 sites mechanically:
```
sed -i -E 's/\(dynamic_cast<spx::[oe]a_signal_type \*>\(([A-Za-z0-9_]+)\.get_interface\(\)\)\)->name\(\)/port_channel_name(\1)/g' src/devices/*.cpp
```
Finally, hand-edit bitstream_source.cpp:37 to `port_channel_name(p_out)`.

Correction from the second check: `bitstream_source.cpp` includes neither `specs.h` nor `sysc_utils.h`, so the fix must also add `#include "utils/sysc_utils.h"` there; otherwise it does not compile. With it, `--vci -t ac_crow` goes from exit 139 to 0 and all other `--vci` outputs are unchanged.
## Golden impact
None. The printed text is identical whenever the cast succeeded. opt/vci is unchanged.
## Related
optical_output_port.cpp:177 has a commented-out copy of the pattern.

- Discovery seeds: S1.29 (see `backlog/README.md` for seed numbering).
- Review ids: F-A2-06.
## Verification
In a scratch build I applied the helper and the sed (51 replacements):
- `specs --vci -t ac_crow` exits 0 and prints `S_BR_0 --> S_BL_0`.
- The full golden check gives 44/44 PASS.

The fix is behaviour-neutral: it only replaces a null dereference.

**Independent second check (V4, of F-A2-06): ADJUST.** Method: Applied the helper, the sed (51 replacements in 11 files, count matches) and the bitstream hand-edit. The build FAILED. After adding an include it built; then ran golden, `--vci -t ac_crow`, and `--vci` on all 19 tbs before and after.

- The suggested fix does not compile as written: `bitstream_source.cpp:37: error: 'port_channel_name' was not declared in this scope`. bitstream_source.cpp/.h include only systemc.h, optical_output_port.h and optical_signal.h. None of them includes specs.h or sysc_utils.h, so the claim "included everywhere via specs.h" is false for that file.
- Required change to the suggested fix: when hand-editing bitstream_source.cpp:37, also add `#include "utils/sysc_utils.h"` to bitstream_source.cpp, after `#include "devices/bitstream_source.h"`. The other 11 files already get it through specs.h, directly or via their headers.
- With that include, results:
  - `specs --vci -t ac_crow`: exit 139 before, exit 0 after. The first 34 output lines are identical; the rest is new output, starting with `S_BR_0 --> S_BL_0`.
  - All other 18 tbs with `--vci` give identical stdout apart from "Total runtime".
  - Golden check (incl. opt/vci, with --print-slack 0): 44/44 PASS.
- The fix is behaviour-neutral.

**Orchestrator:** Missing include added to the suggested fix.
