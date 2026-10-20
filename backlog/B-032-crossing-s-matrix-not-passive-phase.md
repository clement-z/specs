# B-032: Crossing S-matrix is not passive (in-phase crosstalk, |t|²+|x|² > 1, signed xtalk)
severity: high | category: physics | effort: S | fix-class: needs-human | verdict: CONFIRMED
location: src/devices/crossing.cpp:9-18, src/devices/crossing.cpp:62-74, src/devices/crossing.h:28-33, src/parser/parse_element.cpp:593-606
## Problem
`CrossingUni` and `CrossingBi` compute `t = 10^(-att/20)` and `x = 10^(+xtalk/20)` and build S = [[t, x], [x, t]]
with both entries real (phase 0). The device then creates energy in three ways:
- A single input gives |t|²+|x|² > 1 whenever xtalk is set and att is small. For example, att=0 and xtalk=-20 dB give
  1.01.
- With two coherent inputs, the in-phase cross term interferes constructively. With the same parameters, 2 W in gives
  2.42 W out.
- The two dB parameters use opposite sign conventions. `att` is a positive loss, while `xtalk` is a signed ratio. A
  user who writes `xtalk=20` (meaning 20 dB isolation) gets 100× power on the cross port.
## Evidence
`backlog/repro/a2-crossing-gain.cir` (two 1 W CW at 1550 nm into in1/in2, `att=0 xtalk=-20`, `.op`):
```
ROOT/out1 (OPTICAL) = ... (1.100000 V.m⁻¹, 1.210000 W, 0.0000 rad)
ROOT/out2 (OPTICAL) = ... (1.100000 V.m⁻¹, 1.210000 W, 0.0000 rad)
```
Single-input runs (only `cwsrc1 in1`):
- `xtalk=-20` gives out1 = 1 W and out2 = 0.01 W (total 1.01 W).
- `xtalk=20` gives out1 = 1 W and out2 = 100 W.

Code:
- crossing.cpp:9 `crosstalk_field_lin = ... pow(10.0, m_crosstalk_power_dB / 20)`
- crossing.cpp:15-18 `S13 = polar(t, 0.0)`, ..., `S14 = polar(x, 0.0)`
## Expected
The model should be passive and unitary-bounded for any input phases:
- the cross term in quadrature with the through term (`x·e^{jπ/2}`, same convention as the directional coupler);
- |t|²+|x|² ≤ 1;
- one documented dB sign convention.
## Suggested fix
(needs-human) In both Uni and Bi:
- `x = 10^(-|xtalk_dB|/20)` (or reject `xtalk > 0` with `cerr` + `exit(1)`);
- `S14 = S23 = polar(x, M_PI/2)`;
- choose one of `t = sqrt(10^(-att/10) - x²)` (att is the total loss) or `t = 10^(-att/20)·sqrt(1 - x²)` (att is the
  excess loss);
- document the choice in the cheatsheet;
- add a golden case built from `a2-crossing-gain.cir`.

With quadrature and t²+x² = 1, that repro gives 1 W + 1 W.
## Golden impact
None. No golden case uses CROSSING. The OCTANE crossings use the default xtalk=NaN (x = 0), which is unchanged.
## Related
- B-033 (same functions, so fix both together).
- A4 S2.07/S3.13: CROSSING outputs are bound `AS_READER` and `CrossingBi` is never instantiated
  (parse_element.cpp:539-540, :590).

- Discovery seeds: S2.06 (see `backlog/README.md` for seed numbering).
- Review ids: F-A2-02.
## Verification
Ran `a2-crossing-gain.cir` and the two single-input variants on the stock binary. The outputs are quoted above.

**Independent second check (V4, of F-A2-02): CONFIRMED.** Method: Reproduced with my own netlists on the stock binary (1310 nm, keyword args), and read crossing.cpp:6-18 and :59-74.

- Single input:
  - `xtalk=15` (isolation written as positive): 1 W in gives 31.62 W on out2.
  - `att=0.5 xtalk=-10`, 2 W in: 1.7825 + 0.2 W. That case is still lossy, so gain needs 10^(-att/10)+10^(xtalk/10) > 1.
- Two 0.5 W coherent inputs:
  - `att=0 xtalk=-30`: 1.064 W out. Gain appears even at -30 dB crosstalk.
  - `att=0 xtalk=-10`, in phase: 1.732 W out.
  - `att=0 xtalk=-10`, anti-phase: 0.468 W out.
  So the S-matrix is not passive or unitary.
- This is physics and needs-human.
- Golden impact "None" is correct:
  - no golden netlist or tb uses CROSSING;
  - OCTANE (octane_matrix.cpp:90) builds `Crossing(name, loss)` with xtalk = NaN, so x = 0, and both proposed formulas leave t unchanged when x = 0.
- Side observation, not part of this finding: negative positional numbers are a parse error (`crossing1 a b o1 o2 0 -30` gives "offending token -"). Keyword args work.
