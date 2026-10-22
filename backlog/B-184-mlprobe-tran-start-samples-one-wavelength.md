# B-184: MLPROBE at the start of a `.tran` samples only the wavelength last written to its net
severity: med | category: bug | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: open
location: src/devices/probe.cpp (`MLambdaProbe::on_port_in_changed()`, the B-048 sampling when the probe is enabled)

## Problem
Found in phase PD2 (report `backlog/sprint-2026-10c/PD2-REPORT.md`, "Found, not done"), verified in PD2b
(2026-10-23). In a `.tran`, probes are disabled during the operating point and, when the transient starts, sample
the current value of their net (B-048). An optical net holds a single `OpticalSignal`: the last value written to
it, of whatever wavelength. An MLPROBE on a multi-wavelength net therefore records, at t = 0, only the wavelength
written last during the operating point; the other listed wavelengths stay at 0 until the net is written again at
that wavelength. With CW sources that never happens, so they read 0 for the whole transient, although the
operating point computed them (the merger keeps one memory per wavelength).

## Evidence
`backlog/repro/b184-mlprobe-tran-start-multilambda.cir`: two CW sources (1.55 and 1.56 um, 1 W) into a merger,
MLPROBE on its output listing both wavelengths, `.tran 1e-9`. Expected 0.5 W at both wavelengths from t = 0.
Observed with 2140dd4 (pixi clang) and 54b15dc (pixi gcc), identical:
- VCD: `power@1550_000000` = 0.5, `power@1560_000000` = 0 for the whole run;
- `--format csv`: one row, `0,ROOT/MLPROBE1,1.55e-06,0.4999999999999999,0.7071067811865475,0`, none at 1.56e-06;
- the OP listing shows `ROOT/m (OPTICAL) = @ 1550 nm (0.707107 √W, ...)` only (a net holds one value; expected).

## Expected
At t = 0, an MLPROBE reports the operating-point value of every listed wavelength present on its net, as the
merger (and any per-wavelength consumer) sees them.

## Suggested fix (needs design)
The probe needs the per-wavelength values the net carried during the operating point, which the net does not
keep. Options: (a) while disabled, the MLPROBE keeps listening and stores the last value per listed wavelength
without tracing, then traces all of them when enabled; (b) the output port that drives the net exposes its last
emitted value per wavelength (`m_emitted_fields`) and the probe reads those at enable. (a) is local to the probe.
Either changes `.tran` traces of multi-wavelength nets (new values at t = 0), so the golden cases with an MLPROBE
on such a net (if any) are re-recorded; add the repro as a golden case. PROBE (last event of the net, one
wavelength by design) is not concerned.

## Related
B-048 (probes sample the OP value at `.tran` start), F-05 (tracing), B-001 (per-wavelength values in FD writes).
