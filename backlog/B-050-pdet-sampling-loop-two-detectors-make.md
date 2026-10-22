# B-050: The PDET sampling loop: two detectors make an open-ended `.tran` never end, and every PDET wakes and dumps every Ts
severity: high | category: perf | effort: M | fix-class: needs-design | verdict: CONFIRMED | status: fixed in be96f26 (sprint 2026-10b P3)
location: src/devices/detector.cpp:48-57,64-79
## Problem
`on_time_tick()` loops on `if (sc_pending_activity()) wait(Ts, m_event_manual_trigger); else wait();` with Ts = 1 ps by default.
- **Hang.** With two or more PDETs, each detector's 1 ps timeout is "pending activity" for the other, so they keep each other alive forever. A `.tran` without a duration (accepted syntax, used by circuits/subckt_tests.cir) never terminates, and the VCD grows without bound.
- **Cost.** Even a single detector wakes every Ts while anything else is pending.
- **VCD bloat.** It recomputes Σ E_k·e^{j2πf_k t}, and for a single wavelength the result jitters in the last ulp (0.0009999999999999998 / 0.001 / ...). The traced `readout` therefore changes at almost every sample, producing a VCD entry per ps per detector for a constant input.
## Evidence
- `backlog/repro/a3-pdet-two-hang.cir` (VLSRC pulse at 1 ns, 2 x PDET, `.tran`): `timeout 20` kills it (exit 124) after writing a 292 MB VCD and reaching about 10 µs of simulated time.
  - With one PDET removed: "Simulated 1 ns", exit 0.
- `a3-pdet-readout.cir` with `.tran 10e-9` and a constant 1 mW input: 4696 `readout` VCD entries in 10 ns.
- `--mon` repro: 2 PDETs and `.tran 1e-6` take 1.5 s for one source event.
## Expected
Open-ended `.tran` ends when the circuit is quiet. Detector cost scales with input events, not with simulated time divided by Ts.
## Suggested fix
Design choice. One option: sample on `m_event_manual_trigger` only, and add periodic Ts sampling only while two or more non-zero wavelengths are stored (that is the only case where a beat makes the output time-dependent). Compute the single-λ case as Σ|E_k|² so it is exact. Do not use `sc_pending_activity()` to decide liveness.
## Golden impact
None of the current cases has an enabled detector. New behaviour changes the PDET readout VCD (fewer entries, ulp-level values) for netlists with PDETs.
## Related
B-049, B-056 (random dark field also forces a write every Ts), B-059.

- Review ids: F-A3-04.
## Verification
I ran both repros with `/tmp/ws-review-build/specs` and observed the timeout, the 292 MB VCD and the entry counts above.

**Independent second check (V1, of F-A3-04): CONFIRMED.** Method: ran a3-pdet-two-hang.cir and a one-PDET variant with /tmp/ws-review-build/specs

- **Two PDETs:** `timeout 20` kills the run (exit 124). On this loaded machine the VCD had reached 55 MB at sim time #19788120, and it was still growing.
- **One PDET (pdet2 deleted):** "Simulated 1 ns", exit 0, 37 kB VCD.
- **Code:** detector.cpp:48-57 uses `sc_pending_activity()` as the liveness test, as described.
- The ulp jitter is confirmed by the B-049 VCD.
- **Severity and class:** high (non-terminating run, unbounded output) and needs-design are justified. The absolute VCD size and sim time depend on machine load.
