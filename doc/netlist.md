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
