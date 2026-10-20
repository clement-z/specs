# B-042: Remove dead "nan wavelength" init in PhaseShifter (shifts printed signal IDs)
severity: low | category: style/maintainability | effort: XS | fix-class: needs-design | verdict: CONFIRMED
location: src/devices/phaseshifter.cpp:9, src/devices/phaseshifter.cpp:84, src/devices/phaseshifter.cpp:130
## Problem
This is the same dead "nan wavelength" init as B-041. Here the maps are iterated on every `vin` change
(phaseshifter.cpp:62-76, :167-196). Each vin change before light arrives (or when light is on another λ) therefore
builds and queues a zero-field event at λ id 0, which the OOP then suppresses. Physical results do not change, but the
extra `OpticalSignal` constructions advance the global signal-ID counter.
## Evidence
Scratch build with the lines removed:
- tb/ps and tb/phaseshifter fail golden only because the `[NNNN]` IDs in stdout shift (e.g. `IN1:  [0030]` →
  `[0022]`, `OUT: [0099]` → `[0090]`).
- Their stdout with IDs normalised is identical, and the VCD (`ps_tb.vcd`) is byte-identical apart from `$date`.
## Expected
No dead code, and no spurious λ-0 events.
## Suggested fix
Delete the 3 lines and re-record the tb/ps and tb/phaseshifter baselines in the same commit. This needs a decision
because golden stdout changes, even though only IDs are affected.
## Golden impact
tb/ps and tb/phaseshifter stdout: signal IDs shift; values and VCD are unchanged.
## Related
B-041. Cross-area A1/A5: golden stdout prints global OpticalSignal IDs, which `run_golden.py normalise_text`
(:93-108) does not normalise.

- Review ids: F-A2-12.
## Verification
Scratch build plus golden check, and a manual diff of normalised stdout and VCD for both tbs.
