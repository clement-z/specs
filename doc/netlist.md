# Netlist element arguments

Nets, positional arguments and keywords of the netlist elements listed below.
This file complements `doc/resources/cheatsheet.pdf` and takes precedence where they differ.

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
  - `K` (alias `INITIAL_STATE`): initial state, integer, default 0;
  - `TANH_COEF`: shape of the transmission curve, default 3.

  A keyword overrides the positional value.
- Required: `emelt` and `n` (both non-zero), `tc` and `ta` (positionally or by any alias; `tc=0` is
  valid). There is no default transmission.
- Power transmission in state k: `tc + (ta - tc) * tanh(TANH_COEF * k / n)`.
- Example: `pcmcell1 in out 1e-12 10 0.85 0.95` is `pcmcell1 in out emelt=1e-12 n=10 tc=0.85 ta=0.95`.
