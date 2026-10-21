# Netlist element arguments

Nets, positional arguments and keywords of the netlist elements listed below.
This file complements `doc/resources/cheatsheet.pdf` and takes precedence where they differ.

## COUPLER

Directional coupler (2x2, lossless unless `loss` is set).

    COUPLERname in1 in2 out1 out2 [k [loss]] [keyword=value ...]

Nets, in this order: two inputs (`in1`, `in2`), then two outputs (`out1`, `out2`). `in1`->`out1` and
`in2`->`out2` are the through paths (phase 0); `in1`->`out2` and `in2`->`out1` are the cross paths (phase
+pi/2). Positional arguments, in order: `k`, `loss`.

| Keyword (case-insensitive) | Sets | Valid range | Resulting powers |
| --- | --- | --- | --- |
| `k`, `kf`, `kfield` (also the 1st positional argument) | **field** cross-coupling coefficient | [-1, 1] | P_cross = k^2, P_through = 1 - k^2 |
| `kp`, `kpow`, `kpower` | **power** cross-coupling ratio | [0, 1] | P_cross = kp, P_through = 1 - kp |
| `t` | **through** field coefficient | [-1, 1] | P_through = t^2, P_cross = 1 - t^2 |
| `loss` (also the 2nd positional argument) | insertion loss in **dB** (power), default 0 | any | both outputs are scaled by 10^(-loss/10) |

`k`, `kp` and `t` all set the same quantity (the through power). Give only one of them: keywords override
positional arguments, and if several keywords are given only one takes effect, not necessarily the last one
written. When none is given, the coupler splits the power evenly (P_through = P_cross = 0.5). A value outside
the valid range is rejected with an error.

Example, a coupler that sends 2.25 % of the power to the cross port (the first two lines are equivalent, and
so is the third up to the rounding of t = sqrt(1 - 0.0225)):

    coupler1 in ring_bl out ring_br k=0.15
    coupler1 in ring_bl out ring_br kp=0.0225
    coupler1 in ring_bl out ring_br t=0.98869

Note on the paper: the "kappa = 0.15 power cross-coupling ratio" of the CROW benchmark (built in C++, not from
a netlist) is a **power** ratio, i.e. `kp=0.15`, or `k=sqrt(0.15)` (about 0.387). The netlist of Fig. 1b uses
`k=0.15`, which is **field** coupling (P_cross = 0.15^2 = 0.0225). The two are different parameters, not an
erratum.

## CROSSING

Waveguide crossing (2x2, passive) with excess loss and crosstalk.

    CROSSINGname in1 in2 out1 out2 [att [xtalk]] [keyword=value ...]

Nets, in this order: two inputs (`in1`, `in2`), then two outputs (`out1`, `out2`). `in1`->`out1` and
`in2`->`out2` are the through paths (phase 0); `in1`->`out2` and `in2`->`out1` are the crosstalk paths (phase
+pi/2, in quadrature, as in the coupler). Positional arguments, in order: `att`, `xtalk`. At most 2; more is
an error.

| Keyword (case-insensitive) | Sets | Default |
| --- | --- | --- |
| `att`, `attenuation` (also the 1st positional argument) | **excess loss** in dB, on top of the power that goes to the cross port | 0 |
| `xtalk`, `crosstalk` (also the 2nd positional argument) | crosstalk in dB: the power ratio out2/in1 with nothing on `in2` | none (no crosstalk) |

Keywords override positional arguments. With x = 10^(-|xtalk|/20) (x = 0 when `xtalk` is not given), the
through field coefficient is t = 10^(-att/20)·sqrt(1 - x^2) and the cross field coefficient is j·x. With one
input, P_cross = x^2 and P_through = 10^(-att/10)·(1 - x^2). The crossing is passive: with `att` = 0 it is
lossless (P_through + P_cross = P_in), and two coherent inputs never give more output power than input power,
whatever their phases. Each wavelength propagates independently.

The sign of `xtalk` is ignored: a positive value is read as an isolation, so `xtalk=-20` and `xtalk=20` both
send 1 % of the power to the cross port. A negative positional number is a parse error, so write a negative
crosstalk as a keyword (`xtalk=-20`) or use the positive form positionally. `att` is not range-checked: a
negative value is a gain, do not use it.

If any net of the crossing is bidirectional (`&`, or two writers), the crossing is bidirectional: light
entering an output comes out of the inputs with the same coefficients (reciprocal), e.g. `out1`->`in1` through
and `out1`->`in2` cross. Otherwise it is unidirectional (inputs are read, outputs are written).

Example, with 1 W into `a`: x^2 = 0.1, so 0.1 W leaves on `d` at +pi/2 rad and 10^(-0.1)·0.9 = 0.7149 W
leaves on `c` at 0 rad (the two lines are equivalent):

    crossing1 a b c d att=1 xtalk=-10
    crossing1 a b c d 1 10

## CWSRC

Continuous-wave optical source: `CWSRC<name> <out> [wl [power [phi]]] [keyword=value ...]`

- Nets: `out`, optical output.
- Positional arguments, in order: `wl` (wavelength, m), `power` (W), `phi` (phase, rad). At most 3;
  more is an error.
- Keywords (case-insensitive): `WL` (alias `WAVELENGTH`), `POWER`, `PHI`. A keyword overrides the
  positional value; `PHI` is applied after `POWER`.
- Required: none. `power` defaults to 0 W; always give `wl`.
- Example: `cwsrc1 in 1.55e-6 1e-3 0.5` is `cwsrc1 in wl=1.55e-6 power=1e-3 phi=0.5`.

## VLSRC

Optical value-list source: `VLSRC<name> <out> [file] [VALUES=...]`

- Nets: `out`, optical output.
- Positional arguments: `file` (string, path relative to the working directory). At most 1; more is
  an error.
- Keywords (case-insensitive): `VALUES`, either a file name (string) or a list
  `[[t0, p0, wl0], [t1, p1, wl1], ...]` (s, W, m). `FILE` and `SCALE` are reserved: they give a
  "not implemented" error.
- File format: one `t,power,wavelength` line per value (s, W, m). The first line of the file is always
  skipped (B-053), so start the file with a header line. Empty lines and lines starting with `;` are
  skipped.
- Required: none. When enabled, the source emits 0 W unless a value is given at t = 0; without
  values it emits nothing else.
- Example: `vlsrc1 in "pulses.csv"` is `vlsrc1 in values="pulses.csv"`.

## EVLSRC

Electrical value-list source: `EVLSRC<name> <out> [file] [VALUES=...]`

- Nets: `out`, electrical output.
- Positional arguments: `file` (string, path relative to the working directory). At most 1; more is
  an error.
- Keywords (case-insensitive): `VALUES`, either a file name (string) or a list `[[t0, V0], [t1, V1], ...]`
  (s, V). `FILE` and `SCALE` are reserved: they give a "not implemented" error.
- File format: one `t,voltage` line per value (s, V). The first line of the file is always skipped
  (B-053), so start the file with a header line. Empty lines and lines starting with `;` are skipped.
- Required: none. When enabled, the source emits 0 V unless a value is given at t = 0; without
  values it emits nothing else.
- Example: `evlsrc1 e "drive.csv"` is `evlsrc1 e values="drive.csv"`.

## PCMCELL

Phase-change memory cell: `PCMCELL<name> <in> <out> [emelt [n [tc [ta]]]] [keyword=value ...]`

- Nets: `in`, optical input; `out`, optical output (unidirectional).
- Positional arguments, in order: `emelt` (programming energy per state step, J), `n` (number of
  states, integer), `tc` (power transmission in the crystalline state, W/W), `ta` (power
  transmission in the amorphous state, W/W). At most 4; more is an error.
- Keywords (case-insensitive):
  - `EMELT` (aliases `MELT_ENERGY`, `E_MELT`);
  - `N` (aliases `NSTATES`, `NLEVELS`, `N_STATES`, `N_LEVELS`);
  - `TC` (aliases `T_C`, `SP`), `TA` (aliases `T_A`, `EP`);
  - `K` (alias `INITIAL_STATE`): initial state, integer in [0, n], default 0;
  - `TANH_COEF`: shape of the transmission curve, default 3.

  A keyword overrides the positional value.
- Required: `emelt` and `n` (both non-zero), `tc` and `ta` (positionally or by any alias; `tc=0` is
  valid). There is no default transmission.
- Checks: `n > 0` and `0 <= k <= n`, otherwise an error.
- Power transmission in state k: `tc + (ta - tc) * tanh(TANH_COEF * k / n)`.
- Example: `pcmcell1 in out 1e-12 10 0.85 0.95` is `pcmcell1 in out emelt=1e-12 n=10 tc=0.85 ta=0.95`.

## PDET

Photodetector: `PDET<name> <in> <out> [ts [r]] [keyword=value ...]`

- Nets: `in`, optical input; `out`, electrical output (the photocurrent, in A).
- Positional arguments, in order: `ts` (sampling time, s), `r` (responsivity, A/W). At most 2; more is
  an error.
- Keywords (case-insensitive); a keyword overrides the positional value:

  | Keyword | Sets | Default |
  | --- | --- | --- |
  | `ts`, `sampling_time` | sampling time, s | 1e-12 |
  | `r`, `responsivity`, `gain` | responsivity, A/W | 1 |
  | `nb`, `noise_bypass` | boolean: `true` means no noise | true |
  | `fop`, `frequency` | operating frequency used by the noise model, Hz | 1e9 |
  | `darkfield` | boolean: dark-field injection (see below) | true |

  The dark current is 0 and cannot be set from a netlist.
- Readout: R·|E_total|², plus noise when `nb=false`. With one wavelength (or several where at most one has
  a non-zero field) this is R·Σ|E_k|², exact. With two or more wavelengths with non-zero fields, they beat:
  the readout is R·|Σ E_k·e^(j2πf_k·t)|² and depends on time.
- Dark field: when the total field is exactly 0, a field of 1e-20·N(0,1) √W is used instead, so a dark
  detector reads about 1e-40 A instead of 0. It is on by default (`darkfield=true`, the historical
  behaviour); with `darkfield=false` a dark detector reads exactly R·0 + the dark current, that is 0
  (plus the noise when `nb=false`).
- Random numbers (noise, dark field) are reproducible: each detector draws from its own stream, derived
  from the master seed (`specs --seed N` or `.options seed=N`, default seed 0, see `doc/numerics.md`) and
  its hierarchical instance name (e.g. `ROOT/PDET1`). The same netlist and seed give the same values in
  every run, and adding, removing or reordering other devices does not change the random values a
  detector draws. A different seed gives other values. With `nb=false` a new noise value is drawn each
  time the readout is computed.
- The readout is written to `out` (and traced as `<name>.readout` in the VCD; `<name>.readout_no_interference`
  is R·Σ|E_k|²):
  - on every input event;
  - when the detector is enabled: in `.tran`, detectors are disabled during the operating point and enabled at
    the start of the transient, so a detector lit by a CW source shows its readout from t = 0;
  - while two or more wavelengths beat, also every `ts`. These periodic samples are written only when the
    readout changed by more than max(abstol, reltol·|last written value|), with the `.options abstol` and
    `reltol` values.
- A constant input gives one record, not one per `ts`. The detector does not keep the simulation alive: an
  open-ended `.tran` (no duration) ends when the circuit is quiet. A permanent beat (e.g. two CW wavelengths
  on one PDET) never goes quiet, so give a duration.
- `.op` and `.dc`: detectors are disabled and do not write `out`. Testbenches and C++ users get an enabled
  `Detector`.
- History: before F-01 (B-055, B-056) the random numbers were seeded from `std::random_device`, so the
  noise and the dark-field value differed between runs. Before B-049/B-050, `out` was never written (a
  PSHIFT or MZI driven by a PDET saw 0) and two detectors made an open-ended `.tran` run forever.
- Example: `pdet1 in vout` then `pshift1 a b vout sensitivity=1000`. With 1 mW on `in` (R = 1 A/W), `vout`
  is 1 mA and the phase shifter adds 1000 rad/A · 1 mA = 1 rad.

## PROBE, MLPROBE and traced nets in `.tran`

`PROBE<name> <net> [power [abs [phase [wl]]]]` traces the last event of its net (power, field modulus, phase,
wavelength); `MLPROBE<name> <net> wl=[wl1, wl2, ...]` traces power, modulus and phase per listed wavelength
(the wavelength must match exactly). With `.options traceall=1` (the default) every optical net also gets an
automatic probe, traced as `PROBE{<net>}`.

In `.tran`, the probes are disabled during the operating point. When the transient starts, after the operating
point (`.nodeset` included) and after any initial conditions, every probe samples the current value of its net
(B-048, sprint 2026-10b): a net set by a CW source shows its operating-point value from t = 0, as printed in the
operating-point listing on stdout. Before, such a net read 0 in the VCD for the whole transient. A net that was
never written (undriven, or fed only by sources that start later) stays at 0 until its first event.
