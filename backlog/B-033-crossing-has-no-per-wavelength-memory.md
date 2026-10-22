# B-033: Crossing has no per-wavelength memory (light lost or errors with 2 λ)
severity: med | category: physics | effort: S | fix-class: needs-human | verdict: CONFIRMED | status: fixed in 04c7819 (sprint 2026-10b P3)
location: src/devices/crossing.cpp:37-56, src/devices/crossing.cpp:101-128
## Problem
On every event the thread reads the last event of each port (`p_in1->read()`, `p_in2->read()`) and adds
`s1*S13 + s2*S23`. When the two ports carry different wavelengths, `OpticalSignal::operator+=`
(optical_signal.cpp:8-35) keeps one wavelength:
- In FD mode (`.op`, `.dc`) it keeps the larger λ.
- In TD mode it keeps the lhs λ and prints an error.

The other wavelength is silently dropped. The DC, Merger and PhaseShifter keep a per-λ `std::map` for exactly this
reason. The crossing does not.
## Evidence
`backlog/repro/a2-crossing-multi-lambda.cir` (in1 = 1 W @1550 nm, in2 = 1 W @1560 nm, ideal crossing). With `.op`:
```
ROOT/out1 (OPTICAL) = [0008]  @ 1550 nm (0.000000 V.m⁻¹, 0.000000 W, 0.0000 rad)   <- expected 1 W @1550
ROOT/out2 (OPTICAL) = [0077]  @ 1560 nm (1.000000 V.m⁻¹, 1.000000 W, 0.0000 rad)
```
With `.tran 1e-12` instead, I got 2× `Attempted to sum signals of different wavelengths, but current simulation mode
doesn't allow it (operator+=)`. In that run out1 = 1 W @1550 is correct, but out2 = 0 W @1550 is wrong (expected
1 W @1560).
## Expected
Each wavelength should propagate independently: out1 = 1 W @1550 nm and out2 = 1 W @1560 nm.
## Suggested fix
(needs-human because results change)
- Add `std::map<uint32_t, OpticalSignal::field_type>` memories: 2 in Uni, 4 in Bi.
- Split the single thread into one thread per input port, as in DirectionalCoupler (directional_coupler.cpp:44-67).
  Alternatively, test `p_inX->event()`.
- On each event, update only that port's memory at its λ id, and emit both outputs for that λ id.
## Golden impact
None. No golden case uses CROSSING.
## Related
B-032 (same code; fix together). Discover 02 §4 "Crossing".

- The crossing reaches the cross-wavelength branches of `OpticalSignal::operator+=`/`-=` (`optical_signal.cpp:17-32,46-61`): in TIME_DOMAIN they print to `cerr` on every event and drop the right-hand side; in FREQUENCY_DOMAIN they keep the longer λ; the mode is read from the **global** `simulation_mode`, which says TIME_DOMAIN during the OP phase of a `.tran`. That behaviour is part of this issue (seed S1.17, initially rejected as unreachable; the second check V3 showed it is reachable). The GenericTransmissionDevice also uses `operator-` but is unreachable from netlists (B-069).

- Discovery seeds: S2.08 (see `backlog/README.md` for seed numbering).
- Review ids: F-A2-03.
## Verification
Ran the repro in both `.op` and `.tran` with the stock binary. The outputs are quoted above.
