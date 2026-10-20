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
