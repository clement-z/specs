# B-059: Detector model questions: beat-term sign vs the waveguide phase convention, 2πft precision, noise terms
severity: med | category: physics | effort: S | fix-class: needs-human | verdict: CONFIRMED
location: src/devices/detector.cpp:66-71,78-79,102-111, src/devices/waveguide.cpp:65
## Problem
1. **Sign.** The beat sum is Σ E_k·e^{+j2πf_k t} (detector.cpp:70). Under an e^{+jωt} time convention a delay τ multiplies a field by e^{−jωτ}, but the waveguide applies e^{+j2π·neff·L/λ} (waveguide.cpp:65). With two or more wavelengths the beat note appears advanced by the phase delay instead of delayed: it is mirrored. Single-λ intensity is unaffected.
2. **Precision.** 2π·f·t with f ≈ 1.9e14 Hz loses about 0.25 rad of phase at t ≈ 1 s and is fine below about 1 ms. Using (f_k − f_ref)·t removes this, and also removes the ulp jitter behind B-050.
3. **Noise.**
   - `noise_gen` is drawn twice per sample, independently, for `readout` and `readout_no_interference`.
   - The `no_interf` value uses the shot noise of the interference photocurrent.
   - `inoise_rms` holds a variance.
   - The bandwidth is the parameter called operating frequency (`FOP`).
## Evidence
Code at the cited lines. The sign analysis is mathematical: with E_k = A_k·e^{+jβ_k L}, the beat term is A1A2*·e^{j(ω1−ω2)(t + τ_p)}.
## Expected
One documented phase/time convention used consistently by every device, and a clearly specified noise model.
## Suggested fix
A human picks the convention: either e^{−jωt} in the detector, or a change of sign in WG/PS/DC. Document it in discover/02 and the docs, and switch to relative frequencies. The noise points are minor and can be clarified at the same time.
## Golden impact
Changes multi-λ PDET outputs only. No golden case has one.
## Related
B-050, B-055, A2 (waveguide/phase-shifter conventions).

- Discovery seeds: S2.09 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-13.
## Verification
Code reading. Seeds S2.09 and discover/02 §4 agree with it.
