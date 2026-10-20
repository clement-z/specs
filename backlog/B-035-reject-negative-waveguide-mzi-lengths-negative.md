# B-035: Reject negative waveguide/MZI lengths (negative delay aborts SystemC)
severity: low | category: bug | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/parser/parse_element.cpp:94-95, src/parser/parse_element.cpp:114-115, src/parser/parse_element.cpp:507-510, src/devices/waveguide.cpp:18, src/devices/waveguide.h:47-52
## Problem
`WG ... length=-100e-6` is accepted:
1. The parser writes `m_length_cm` directly. `WaveguideBase::setLength()` (waveguide.h:47-52) prints an error for
   negative lengths without exiting, and it is used only by the dead `Ring`.
2. The negative group delay goes through `sc_time(double, SC_NS)`, which casts `v*scale+0.5` to int64 and then to an
   unsigned tick count (SystemC sc_time.cpp:150-161).
3. The first TD event therefore schedules into the past, and SystemC aborts with a core dump.

MZI `LENGTH`/`LENGTH_REF` reach the same waveguides (mzi_active.cpp:37-47).
## Evidence
`backlog/repro/a2-wg-negative-length.cir`:
```
@100 ps, ROOT/VLSRC1 emitted: ...
Fatal: (F4) assertion failed: *t==sc_time_stamp()
In file: .../sysc/communication/sc_event_queue.cpp:95
In process: ROOT/WG1.out_delayed_writer.event_queue_0.fire_event @ 100 ps
exit=134
```
## Expected
A clear error at elaboration, like the existing self-loop check at parse_element.cpp:73-77.
## Suggested fix
In `WGElement::create`, before `return obj;` (:126):
```cpp
if (obj->m_length_cm < 0)
{
    cerr << "Error (" << name << "): waveguide length must be >= 0" << endl;
    exit(1);
}
```
In `MZIElement::create`, before `return obj;` (:526), add the same check on `obj->m_length_cm` and
`obj->m_length_ref_cm` with the message `"MZI length and length_ref must be >= 0"`.
## Golden impact
None. All golden lengths are positive.
## Related
- `ng < 0` (or `ng(λ)` < 0 with a large D) produces the same negative delay. Whether to validate ng is a physics
  question, so it is not part of this fix.
- Cross-area A1 (S1.03): delay quantisation in `sc_time(double)`.

- Review ids: F-A2-05.
## Verification
- Ran the repro on the stock binary and got exit 134.
- Code reading: lengths are only assigned at :95/:115/:508/:510 before the proposed checks.
- Safe because only negative lengths change behaviour, and they currently crash in TD.
