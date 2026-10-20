# B-039: Waveguide reference wavelength λ0 = 1.55 µm is hard-coded
severity: low | category: physics | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/devices/waveguide.cpp:25, src/devices/waveguide.cpp:28, src/devices/waveguide.cpp:42, src/devices/waveguide.cpp:60, src/devices/waveguide.cpp:84-86, src/devices/waveguide.cpp:123-126, src/devices/waveguide.cpp:140, src/devices/waveguide.cpp:162, src/devices/waveguide.cpp:186-188, src/devices/waveguide.cpp:224-227, src/devices/waveguide.cpp:239, src/devices/waveguide.cpp:263-265
## Problem
The neff/ng/D Taylor expansion is centred on the literal `1.55e-6`, which appears 16 times across the three thread
copies. The expansion itself is correct:
- dneff/dλ = (neff−ng)/λ0;
- ½d²neff/dλ² = −cD/(2λ0);
- dng/dλ = cD;
- phase sign e^{+jβL}.

Neff/ng/D values characterised at another wavelength (e.g. O-band) cannot be entered correctly. The cheatsheet
documents "neff/ng/d at 1550nm", so this is a missing parameter rather than a hidden bug. `GenericWaveguide` already
has `lambda0`.
## Evidence
`grep -n 1.55e-6 src/devices/waveguide.cpp` gives 16 hits.
## Expected
A `lambda0` parameter (member `m_lambda0_m`, kwarg `LAMBDA0`/`WL0`) that defaults to 1.55e-6.
## Suggested fix
- Add `double m_lambda0_m = 1.55e-6;` to `WaveguideBase`.
- Use it in place of every literal (or in the shared helper from B-046).
- Add a `LAMBDA0` kwarg in `WGElement::create`, and pass it through MZI/MZIActive if wanted.
## Golden impact
None with the default value.
## Related
B-046 (dedup first makes this a one-place change).

- Discovery seeds: S2.10, S5.17 (see `backlog/README.md` for seed numbering).
- Review ids: F-A2-09.
## Verification
Code reading. I also checked the Uni/Bi copies against each other with `diff`: they are identical apart from port
names and verbose output.
