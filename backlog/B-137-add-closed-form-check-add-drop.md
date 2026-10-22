# B-137: add a closed-form check of the add-drop ring (CW sweep) against the transfer function
severity: low | category: test-gap | effort: M | fix-class: needs-design | verdict: CONFIRMED
location: new tests/analytic/ (suggested), circuits/add_drop.cir, tests/golden/run_golden.py (VCD parser could be reused)
## Problem
No test compares a simulation with an independent result. The golden baseline would accept any consistent but wrong physics.
## Evidence
`python3 -I backlog/repro/a5-adddrop-analytic.py traces/add_drop.vcd` after `specs -f circuits/add_drop.cir -o traces/add_drop`: with k=0.85 (field), 2 dB/cm, neff 2.3, ng 4.3, half ring `101*lambda0/neff`, phase `2 pi L neff(l)/l` (neff linear in l as in waveguide.cpp), through `T = r - k^2 r h^2/(1 - r^2 h^2)`, drop `-k^2 h/(1 - r^2 h^2)`, `h = t e^{j phi}`: over the 30 swept points `max |P_sim - P_model| = 1.44e-6 W` (through port ~5e-6 W at 1550 nm, drop 0.994478). Same check in frequency-domain mode of `specs -t ac_add_drop -m fd` (501 points, identical couplers 0.85 power through, 1 dB/cm, 0.0775 cm per half ring, neff=ng=1): through 4e-4, cross 8e-4 W (reltol 1e-4, high-Q ring), so use a tolerance of about 100 x reltol (B-124).
## Expected
A test that fails if the physics drifts.
## Suggested fix
Add the script (made self-contained, it only needs stdlib) as `tests/analytic/check_add_drop.py`, run from ctest or from run_golden.py as an extra step; tolerance 1e-5 W for add_drop.cir (reltol 1e-6) and 2e-3 W for the FD testbench. Decide where it is executed (needs-design).
## Golden impact
none.
## Related
B-009 (swept wavelengths), B-124.

- Discovery seeds: S2.29 (see `backlog/README.md` for seed numbering).
- Review ids: F-A5-17.
## Verification
Ran both comparisons (numbers above).
