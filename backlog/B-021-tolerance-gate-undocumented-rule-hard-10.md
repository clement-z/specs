# B-021: Tolerance gate: undocumented AND rule, hard 10·abstol zero-snap, reltol divides by 0
severity: low | category: physics | effort: S | fix-class: needs-human | verdict: CONFIRMED
location: src/optical_output_port.cpp:60-73,160-170
## Problem
The gate emits only if `|Δ| > abstol` **and** `|Δ|/|last| > reltol`, which is `max(abstol, reltol·|last|)`. SPICE
uses `abstol + reltol·max(|new|,|old|)`. Any emitted value below `10·abstol` is forced to 0 (`TODO: check
this !!`). The reltol check divides by `|last|`, which is 0 on first emission, and relies on `inf > x` and
`NaN > x` being false. None of this is documented. With a user abstol, the zero-snap becomes visible.
## Evidence
`.options abstol=5e-2` with an MZI λ sweep (dcstale.cir, scratch): 73 of 200 points with |E| between 0.05 and 0.5 √W
are reported as exactly 0. golden `add_drop_pyspecs.cir` uses `abstol=1e-6`, which zeroes every power below 1e-10 W.
## Expected
Documented semantics. The snap threshold is either justified or removed.
## Suggested fix
Decide the rule (keep AND vs SPICE sum; keep, parametrise or remove the snap). Behaviour-neutral clarification for
line 72: `if (last_size == 0) return vector_distance > 0;`.
## Golden impact
Depends on the rule (DC/OP values near the thresholds).
## Related
B-003.

- Discovery seeds: S1.23, S5.13, S5.14 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-21.
## Verification
Ran the scratch MZI sweep and compared with the analytic cos² response.
