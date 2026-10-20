# B-060: PCM programming/transmission model needs a decision (energy, steps, timing, tanh normalisation)
severity: med | category: physics | effort: M | fix-class: needs-human | verdict: CONFIRMED
location: src/devices/pcm_device.cpp:56-83,102-131,144, src/devices/pcm_device.h:50-51
## Problem
1. "energy_absorbed" integrates the incident power Σ P_in·Δt (:118-122), not the absorbed power ((1−T)·P_in or an absorption coefficient).
2. At most one state step per influence window, whatever E/Emelt is (:124-131).
3. A window is evaluated only at the next rising edge after `m_influence_time_ns` (1 ns) of darkness (:110). The last pulse train of a simulation therefore never programs, and the pulse that triggers the evaluation is the first one to see the new transmission.
4. T(N) = Tc + (Ta−Tc)·tanh(speed·N/N) reaches only 99.5% of the span with speed = 3 (:144), so Ta is never reached.
5. N = 0 or N < 0 gives NaN or a frozen state. The parser rejects N = 0 (parse_element.cpp:1242), but the C++ constructor default is N = 0 (pcm_device.h:51), and nothing rejects negative N or k > N.
## Evidence
Code at the cited lines. The paper does not describe the PCM model.
## Expected
A documented model chosen by the OCTANE authors.
## Suggested fix
A human decides, for example: absorbed energy, the number of steps = floor(E/Emelt) capped at N, evaluation at the falling edge plus the window, T normalised by tanh(speed), and validation of N > 0 and 0 ≤ k ≤ N.
## Golden impact
tb/pcm would change with any model change.
## Related
B-051.

- Discovery seeds: S2.11, S2.12 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-14.
## Verification
Code reading.
