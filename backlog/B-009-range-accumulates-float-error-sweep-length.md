# B-009: `range()` accumulates float error: sweep length and endpoint depend on rounding
severity: med | category: bug | effort: S | fix-class: needs-human | verdict: CONFIRMED
location: src/utils/general_utils.h:20-35 (loop at 32), src/parser/parse_analysis.cpp:74
## Problem
`for (val = min; val < max; val += step)` accumulates rounding error. Depending on that rounding, the stop value
is sometimes included and sometimes not, and sweep values drift from `start + i·step`. The parser announces a
different point count from the one that actually runs. A drifted λ also gets its own registry id (B-010).
## Evidence
`backlog/repro/a1-dc-range-endpoint.cir` (`.dc /cwsrc1/P 0 1 0.1`):
```
Recorded sweep order for P(CWSRC1) (10points)
Starting sweep on P(ROOT/CWSRC1) (11 points)        <- last point P=0.9999999999999999 (≈ stop)
```
With step 0.25 the stop is excluded (4 points). In golden add_drop.cir (1535-1565 nm, 1 nm step), 28 of 30 λ
differ from `start+i·step`. The VCD shows e.g. `1.546000000000001e-06`.

Additional evidence (independent report F-A5-02, A5 reviewer):

```
$ sh backlog/repro/a5-dc-sweep-endpoint.sh build/specs
== a.cir   Starting sweep on WL(ROOT/CWSRC1) (30 points) ... last: emitted @ 1564 nm   (window 1535..1565, step 1e-9)
== b.cir   Starting sweep on WL(ROOT/CWSRC1) (11 points) ... last: emitted @ 2000 nm   (1.0..2.0 um step 0.1 um)
python: accumulated 30 steps from 1.535e-6 -> 1.564000000000002e-6, next 1.5650000000000019e-6 >= 1.565e-6  => dropped
        accumulated 10 steps from 1.0e-6   -> 1.999999999999999e-06 < 2.0e-6                               => kept
```

Additional evidence (independent report F-A4-17, A4 reviewer):

- `backlog/repro/a4-dc-endpoint.cir` (`.dc /cwsrc1/WL 1.5e-6 1.6e-6 1e-8`): prints
  `Recorded sweep order for WL(CWSRC1) (9points)` but emits **11** points, 1500…1600 nm (endpoint included).
- add_drop (`1.535e-6 1.565e-6 1e-9`): `(30points)`, 30 emitted, 1535…1564 nm (endpoint excluded).
## Expected
A deterministic number of points, computed as `start + i·step`, with documented stop semantics (SPICE includes the stop).
## Suggested fix
`n = floor((max-min)/step + 1e-9)` (+1 if the stop is inclusive), and `ret.push_back(min + i*step)`. Use the same
`n` in the parser message (parse_analysis.h:59-66). The open decision is inclusive vs exclusive stop.
## Golden impact
DC cases: `cir/add_drop`, `cir/add_drop_sub`, `opt/mode-fd`, `opt/multi-file`, `opt/tol-resolution-overrides`
(λ values shift in the last digits; point count changes if the stop becomes inclusive).
## Related
B-010. A5 has a similar repro (a5-dc-sweep-endpoint.sh).

- Discovery seeds: S1.25, S3.19 (see `backlog/README.md` for seed numbering).
- Merged duplicate reports: F-A5-02 (A5), F-A4-17 (A4).
- Review ids: F-A1-09, F-A5-02, F-A4-17.
## Verification
Ran the repro, and emulated the loop in Python for the golden sweep.
