# B-040: GenericWaveguide (disabled) has wrong Taylor phase coefficients
severity: low | category: physics | effort: XS | fix-class: needs-human | verdict: CONFIRMED | status: obsolete (GenericWaveguide removed in 54b15dc, decision 2026-10-23)
location: src/devices/generic_waveguide.h:18-19, src/devices/generic_waveguide.h:37, src/devices/generic_waveguide.h:78-82, src/devices/generic_waveguide.h:117
## Problem
The target is φ(λ) = 2π·L·neff(λ)/λ, expanded in Taylor coefficients (TransmissionMatrix applies 1/n!):
- :78 `dphi_0 = fmod(2πL·neff, 2π)` is missing `/λ0`.
- :81 `dphi_1 = −2πL·ng/λ0²` is correct, even though the comment at :80-81 says it is wrong.
- :82 `dphi_2 = 2πL·2ng/λ0³` omits the dispersion term. It should be `2πL·(2ng/λ0³ − c·D/λ0²)`, since dng/dλ = cD.
- `group_delay_1 = L·D` (:85) is correct.

Other problems in the same file:
- The constructor exits immediately (:18-19), so the class is unreachable.
- `setLoss` has a debug `cout << loss` (:37).
- The `m_lambda0` comment says "dispersion" (:117).
## Evidence
Code reading of the lines above, checked against d/dλ of 2πL·neff(λ)/λ with ng = neff − λ·dneff/dλ.
## Expected
Either correct coefficients or no dead class.
## Suggested fix
(needs-human) Choose one:
- delete GenericWaveguide and its includes in tb/wg_tb.cpp:6 and tb/lambda_tb.cpp:6 (the usage at wg_tb.cpp:106 is
  commented out);
- or fix: `dphi_0 = fmod(two_pi_L*m_neff/lambda0, 2*M_PI)` and
  `dphi_2 = two_pi_L*(2*m_ng/pow(lambda0,3) - c*m_D/pow(lambda0,2))`, then fix the comments and remove the debug
  cout before re-enabling.
## Golden impact
None. The class is never constructed.
## Related
B-039.

- Discovery seeds: S2.13 (see `backlog/README.md` for seed numbering).
- Review ids: F-A2-10.
## Verification
Code reading. The derivation is given above.

**2026-10-23 (PD2b):** obsolete. `GenericWaveguide` and the rest of the `GenericTransmissionDevice` family were removed (54b15dc, maintainer decision 2026-10-23). A generic N-port device is to be written from scratch (scout F-03); the Taylor coefficients above apply if it offers a Taylor form. The old code is at cc1e52e.
