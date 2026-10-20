# B-035: Reject negative waveguide/MZI lengths (negative delay aborts SystemC)
severity: low | category: bug | effort: XS | fix-class: needs-human | verdict: CONFIRMED
location: src/parser/parse_element.cpp:94-95, src/parser/parse_element.cpp:114-115, src/parser/parse_element.cpp:507-510, src/devices/waveguide.cpp:18, src/devices/waveguide.h:47-52
## Problem
`WG ... length=-100e-6` is accepted:
1. The parser writes `m_length_cm` directly. `WaveguideBase::setLength()` (waveguide.h:47-52) prints an error for
   negative lengths without exiting, and it is used only by the dead `Ring`.
2. The negative group delay goes through `sc_time(double, SC_NS)`, which casts `v*scale+0.5` to int64 and then to an
   unsigned tick count (SystemC sc_time.cpp:150-161).
3. The first TD event therefore schedules into the past, and SystemC aborts with a core dump.

MZI `LENGTH`/`LENGTH_REF` reach the same waveguides (mzi_active.cpp:37-47).

Correction from the second check: negative lengths do not only matter as a crash. In `.op`/`.dc` they finish normally with **gain** when att>0 (1.023 W out of 1 W) and a reversed phase, and in `.tran` they do not crash below about 5.8 µm. `WaveguideBase::setLength()` already prints an error for negative lengths, so rejecting them is the likely answer, but a human should confirm that negative length is never meant as a phase trim.
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
None for the golden cases (44/44 pass with the fix). It changes the behaviour of netlists with negative lengths that run today (they become an error). Length 0 stays accepted.
## Related
- `ng < 0` (or `ng(λ)` < 0 with a large D) produces the same negative delay. Whether to validate ng is a physics
  question, so it is not part of this fix.
- Cross-area A1 (S1.03): delay quantisation in `sc_time(double)`.

- Review ids: F-A2-05.
## Verification
- Ran the repro on the stock binary and got exit 134.
- Code reading: lengths are only assigned at :95/:115/:508/:510 before the proposed checks.
- Safe because only negative lengths change behaviour, and they currently crash in TD.

**Independent second check (V4, of F-A2-05): ADJUST.** Method: Applied both checks, rebuilt, and ran negative and zero lengths in op, dc and tran before and after. Golden check: 44/44 PASS. The repro changes from exit 134 to exit 1.

- The bug is real for the cited case: length=-100e-6 in `.tran` aborts with F4 and exit 134.
- But the finding's safety claim ("only negative lengths change behaviour, and they currently crash in TD") is wrong. Today negative lengths often run to completion with finite, non-physical results:
  - `.op`: `wg1 in out length=-100e-6 att=10` gives out = 1.023 W from 1 W in (gain), phase 2.189 rad, exit 0.
  - `.dc` wavelength sweep: same, exit 0.
  - MZI `.op` with length=-10e-6 gives 0.738/0.262 W, exit 0.
  - TD does not crash when |delay| < 0.5 tick. At 100 fs resolution and ng=2.6 that means |L| < about 5.8 um: WG length=-1e-6 and MZI length_ref=-10.2e-6 in `.tran` both exit 0.
- With the fix, all of these become exit 1, so the fix changes non-crash outputs.
- Required changes:
  - (a) Fix-class: needs-human (small design decision). Is a negative length ever valid, e.g. as an FD phase/delay de-embedding trim? The existing WaveguideBase::setLength() already prints "Error: waveguide length < 0", so the likely answer is "reject", and then the suggested code is correct as written. Alternative if it stays easy-unattended: the maintainer explicitly accepts that inputs which already "work" in op/dc start failing.
  - (b) Problem/Golden-impact text should say: "negative lengths currently crash in TD when |delay| >= half a time tick. Otherwise (op, dc, fd, short lengths in TD) they silently give gain (att>0) and a negated phase. Golden impact: none (all golden lengths >= 0)."
- length=0 is accepted and unchanged (op and tran).

**Orchestrator:** Reclassified easy-unattended -> needs-human: negative lengths run today without crashing in .op/.dc and short .tran.
