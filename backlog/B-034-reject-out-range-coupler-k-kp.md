# B-034: Reject out-of-range COUPLER k/kp/t and SPLITTER ratio (silent 0 W / gain)
severity: med | category: bug | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/parser/parse_element.cpp:303-330, src/parser/parse_element.cpp:209-228, src/devices/directional_coupler.cpp:8-9, src/devices/splitter.cpp:7-8
## Problem
Nothing checks the coupling or split values:
- COUPLER `k>1` (or `kp ∉ [0,1]`, `t>1`) makes `m_dc_through_coupling_power ∉ [0,1]`, so
  `10*log10(<0)` = NaN (directional_coupler.cpp:8-9). Through and cross both become NaN, because `0*NaN = NaN` in
  `m_memory_in2*S24`.
- SPLITTER `ratio>1` gives `sqrt(1-ratio)` = NaN on out2, and out1 then carries more power than the input.

The NaN fields are never visible, because the OOP gating treats them as "no change". `abs(NaN-0) > abstol` is false
(optical_output_port.cpp:60-73, :161-165). The outputs therefore silently stay at 0 W, with no error.
## Evidence
`backlog/repro/a2-coupler-nan.cir` (`.op`; 0.5 W enters each device):
```
ROOT/c1o1 ... (0.000000 V.m⁻¹, 0.000000 W)    <- COUPLER k=1.1: both outputs 0 W
ROOT/c1o2 ... (0.000000 V.m⁻¹, 0.000000 W)
ROOT/s1o1 ... (0.866025 V.m⁻¹, 0.750000 W)    <- SPLITTER ratio=1.5: 0.75 W out of 0.5 W in
ROOT/s1o2 ... (0.000000 V.m⁻¹, 0.000000 W)
exit=0
```
## Expected
Elaboration should stop with a clear error. This follows the project's `std::cerr << ...; exit(1);` pattern;
CLAUDE.md forbids new asserts for user input.
## Suggested fix
In `DCElement::create`, just before `return obj;` (parse_element.cpp:330):
```cpp
if (!(obj->m_dc_through_coupling_power >= 0 && obj->m_dc_through_coupling_power <= 1))
{
    cerr << "Error (" << name << "): coupler through power " << obj->m_dc_through_coupling_power
         << " is outside [0, 1] (k and t must be in [-1, 1], kp in [0, 1])" << endl;
    exit(1);
}
```
In `SplitterElement::create`, just before `return obj;` (:228):
```cpp
if (!(obj->m_split_ratio >= 0 && obj->m_split_ratio <= 1))
{
    cerr << "Error (" << name << "): splitter ratio " << obj->m_split_ratio << " is outside [0, 1]" << endl;
    exit(1);
}
```
The `!(a && b)` form also rejects NaN.
## Golden impact
None. Every golden coupler uses k = 0.15 or 0.85, and no golden netlist sets a splitter ratio.
## Related
- Negative `att`/`loss`/`il`/`IL_*` also silently give gain (WG, COUPLER, MERGER, SPLITTER, PSHIFT, MZI, CROSSING).
  Whether to reject them is a design choice, so it is not part of this fix.
- B-032 covers the crossing `xtalk` sign.

- Discovery seeds: S2.14 (see `backlog/README.md` for seed numbering).
- Review ids: F-A2-04.
## Verification
- Ran the repro on the stock binary (output above).
- Code reading: the check runs after all positional and keyword assignments, so it covers `k`, `kf`, `kp`, `t` and
  positional `k`.
- Safe because only previously garbage-producing inputs are affected.

**Independent second check (V4, of F-A2-04): CONFIRMED.** Method: Applied both checks, rebuilt, swept edge values before and after. Golden check: 44/44 PASS. Repro now exits 1 with the message.

- Endpoints are accepted and give the same output as before:
  - coupler: k=0, k=1, k=-1, k=-0.3, kp=0, kp=1, t=0, t=1, t=-1, positional 0.5;
  - splitter: default, ratio=0, ratio=1, ratio=0.3, positional 0.7.
- Out-of-range values are rejected (rc=1). All of them gave silent 0 W (NaN gated) or gain before:
  - coupler: k=1.0000001, kp=1.01, kp=-0.01, t=1.01, positional 1.5;
  - splitter: ratio=1.5, ratio=-0.1, positional 1.2, ratio=1.0000001.
  - The splitter cases gave 1.5 W or 1.1 W out of 1 W in.
- I agree it fits easy-unattended. These inputs are undefined: log10 of a negative number gives NaN, sqrt of a negative number gives NaN. No valid input changes, and the range is unambiguous with closed endpoints.
- No golden case, circuits/ netlist, discover/ example or pyspecs file uses out-of-range values. The circuits use k = 0.15, k={k}, default dc, and a default SPLITTER.
- Two optional cosmetic notes:
  - The message prints at the default 6-digit precision. ratio=1.0000001 reports "splitter ratio 1 is outside [0, 1]", and k=1.0000001 reports "through power -2e-07".
  - The check is on the final value, so a later valid keyword masks an earlier invalid one (`k=1.2 kp=0.3` is accepted). That is harmless.
