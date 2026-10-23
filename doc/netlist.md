# Netlist syntax

This file has two parts: the syntax of a netlist file (this section) and the nets, positional arguments and
keywords of each element ([Netlist element arguments](#netlist-element-arguments)).

Example, an all-pass ring resonator and a wavelength sweep (`specs -f ring.cir -o traces/ring`; the directory
`traces/` must exist). The sweep has 31 points, from 1535 nm to 1565 nm:

    * All-pass ring resonator, wavelength sweep around 1.55 um
    .param lambda0 = 1.55u
    .param neff    = 2.3
    .param ng      = 4.3
    .param k       = 0.3873              ; field coupling, k^2 = 0.15
    .param ring_len = 101*{lambda0}/{neff}

    cwsrc1 in wl={lambda0} power=1m
    coupler1 in ring_a out ring_b k={k}
    wg_ring ring_b ring_a length={ring_len} neff={neff} ng={ng}
    + att=2                              ; dB/cm

    .options abstol=1e-12 reltol=1e-6
    .dc WL(cwsrc1) 1.535u 1.565u 1n

More examples: `circuits/*.cir` (run them from the repository root) and `tests/golden/netlists/`.

## Lines

- One element, directive or analysis per line. It starts in column 1: a leading space or tab is a syntax error.
- `*` in column 1 starts a comment line. `;` starts a comment that runs to the end of the line. Blank lines are
  ignored.
- `+` in column 1 continues the previous line (the previous line may end with a `;` comment).
- Element prefixes, directive names, keywords, SI suffixes and `.dc` attributes are case-insensitive.
  Element names are upper-cased: `wg1` becomes `ROOT/WG1` (`ROOT/X1/WG1` inside instance `X1`), and two
  elements with the same name are an error. Net names and parameter names are case-sensitive.
- A syntax error prints `Parsing error (file:line): ...`, the offending token and the line on stderr, and
  exits with status 1 (3 for an unexpected character such as `$`).

## Elements and nets

    <PREFIX><name> net1 net2 ... [positional arguments] [key=value ...]

Nets come first, in the order of the table, then the positional arguments, then the keywords.

| Prefix | Nets, in order | Arguments |
| --- | --- | --- |
| `WG` | in, out | cheatsheet: `length` (m), `att` (dB/cm), `neff`, `ng`, `d` |
| `MERGER` | in1, in2, out | cheatsheet |
| `SPLITTER` | in, out1, out2 | cheatsheet |
| `COUPLER` | in1, in2, out1, out2 | [COUPLER](#coupler) |
| `CROSSING` | in1, in2, out1, out2 | [CROSSING](#crossing) |
| `PSHIFT` | in, out, electrical in | cheatsheet |
| `MZI` | in1, in2, out1, out2, electrical in | not documented yet |
| `CWSRC` | out | [CWSRC](#cwsrc) |
| `VLSRC` | out | [VLSRC](#vlsrc) |
| `EVLSRC` | electrical out | [EVLSRC](#evlsrc) |
| `PCMCELL` | in, out | [PCMCELL](#pcmcell) |
| `PDET` | in, electrical out | [PDET](#pdet) |
| `PROBE`, `MLPROBE` | net | [PROBE and MLPROBE](#probe-mlprobe-and-traced-nets-in-tran) |
| `PWR` | in (power meter) | no arguments |
| `X` | one net per subcircuit port, then the subcircuit name | [Subcircuits](#subcircuits) |

"cheatsheet" is `doc/resources/cheatsheet.pdf`. A prefix needs a name after it (`wg1`, `wg_ring`; `wg` alone is
a syntax error).

- Net names start with a letter or `_`, then letters, digits and `_`. An integer is a net name too: `1` is net
  `N00001`. `_` alone is an anonymous net, a new one at each use. Optical and electrical nets are different
  types; using one name for both is an error.
- An optical net normally has one writer (an output) and readers (inputs). A net with two writers is
  bidirectional: light travels both ways. `net&` marks a net bidirectional explicitly. A third writer is an
  error.

## Numbers

Integers (`100`), decimals (`0.5`, `.5`) and exponents (`1.55e-6`). `5.` is not a number.

Integer literals (digits only) are 64-bit signed integers, 0 to 9223372036854775807: a larger one is a parse
error that gives its file and line (B-180). A parameter stored in a narrower type checks its own range (PCMCELL
`n` and `k`: at most 2147483647). A number with an exponent or a scale suffix (`3e9`, `3g`) is a double.

SI scale suffixes go directly after the number, in any case, as in SPICE:

| Suffix | `f` | `p` | `n` | `u` | `m` | `k` | `meg` | `g` | `t` |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Factor | 1e-15 | 1e-12 | 1e-9 | 1e-6 | 1e-3 | 1e3 | 1e6 | 1e9 | 1e12 |

`M` and `m` are both milli, `MEG` is mega. `1.55u` is exactly the same double as `1.55e-6`. A number with a
suffix is a floating-point value (`1k` is 1000.0). Unit letters after the suffix are not accepted: `1.55um` and
`10mW` are syntax errors (offending token `m`, `W`); write `1.55u` and `10m`.

Units are SI: m, W, s, Hz, rad, V, A. Waveguide lengths are in m, losses in dB, waveguide attenuation `att` in
dB/cm. Optical fields are in sqrt(W).

## Values and expressions

| Where | Accepted |
| --- | --- |
| `key=value`, `.param` | a free expression: numbers, `{name}`, `+ - * / ^`, parentheses, unary minus |
| positional argument (elements, `.tran`) | one number, `{name}`, `"string"`, `[list]`, or a braced expression `{...}` |
| `.dc` start, stop, step | a number or `{name}`; no expression |

    wg1 a b length=2*500n neff={neff}      ; free expression in a keyword
    cwsrc1 in {2*775n} 1m                  ; braced expression in a positional argument
    cwsrc2 in2 {{lambda0}*2} 1m            ; a parameter inside braces is {name} too
    vlsrc1 in3 "pulses.csv"                ; string
    mlprobe1 b wl=[1.55u, 1.56u]           ; list

- A parameter is `{name}`, a bare `name` is not a value. A keyword value does not take a braced expression:
  `length={ {x}*2 }` is a syntax error, write `length={x}*2`. A positional argument does not take a free
  expression: `wg1 a b 2*3` and `cwsrc1 in 1.55u -3` are syntax errors, write `{2*3}` and `{-3}`.
- `^` binds tighter than `*` and `/`, which bind tighter than `+` and `-`. `^` groups left to right (`2^3^2` is
  64). `/` is a floating-point division (`1/2` is 0.5). `2*-3` is a syntax error. There are no functions.
- Constants: `{pi}` and `{c}` (299792458 m/s).
- Strings are in double quotes and cannot contain a double quote. Lists are `[a, b, c]` and can be nested
  (`[[t0, p0], [t1, p1]]`).

## Parameters

    .param name=value [name2=value2 ...]

`.assign` is the same directive. The value is evaluated where the parameter is used, so a parameter must be
defined on an earlier line (`Variable not found: L` otherwise). A later `.param` of the same name replaces the
value for the lines after it. The value is an expression and can use earlier parameters.

`specs --param name=value` (repeatable) sets a top-level parameter from the command line. It overrides every
`.param`/`.assign` of that name in the netlist, wherever it is. The value is parsed like a `.param` value
(suffixes, expressions, strings): `--param L=2*3u`. A name the netlist never assigns is still defined and gives
`Warning: --param <name>: the netlist has no .param <name>`. A malformed argument gives
`Invalid --param value: '<arg>' (expected name=value)` and exit status 1. Subcircuit parameters are not affected.

## Subcircuits

    .subckt NAME port1 port2 ... [param=default ...]
    ...body...
    .ends [NAME]

    X<name> net1 net2 ... NAME [param=value ...]

The nets of the instance are connected to the ports in order; their number must match. The name on `.ends` is
optional; if given, it must match the `.subckt` name (case-insensitive), else
`Parsing error (...): .ends xyz does not match the name of the subcircuit (.subckt ABC)` and exit status 1.
Definitions can be nested. An analysis in a body is ignored.

The body is read again for each instance, with its own parameters: the defaults of the `.subckt` line,
overridden by the instance (`x2 b c abc len=30u`). **Top-level parameters are not visible in the body**
(`Variable not found`), nor are `--param` values; `{pi}` and `{c}` are. A body can define derived parameters
with `.param`.

    .subckt abc p q len=10u
    .param l2 = 2*{len}
    wg1 p q length={l2}
    .ends abc
    x1 a b abc
    x2 b c abc len=30u

## `.include`

    .include path

`path` is the rest of the line up to a space, tab or `;`. Quotes are not supported (they become part of the name).
An absolute path is used as is. A relative path is looked up first relative to the directory of the file that
contains the `.include` line (for an `.include` in a `.subckt` body, the file that defines the subcircuit), then
relative to the working directory. If both exist and are different files with different contents, the
file-relative one is used and stderr shows `Warning: .include P: using <dir>/P (relative to the including file);
P relative to the working directory is a different file`. If neither exists: `Error: included file not found
"P"`, exit status 1. Includes can be nested. An `.include` in a body is read for each instance. Stdout shows
`Including file: <path>` with the path opened.

## Analyses

One analysis per netlist. A second one stops the run with `Attempted to register a new analysis (TRAN)` /
`But another one was already specified (OP)` and exit status 1. Without an analysis line the run is a `.tran`
without duration. `-n` skips the analysis.

- `.op`: operating point (frequency domain). The listing of every optical net is on stdout.
- `.dc`: sweep of an attribute of a `CWSRC`, then an operating point at each value:

      .dc /cwsrc1/WL start stop step
      .dc WL(cwsrc1) start stop step

  | Attribute | Sets |
  | --- | --- |
  | `WL`, `WAVELENGTH`, `LAMBDA` | wavelength, m |
  | `P`, `POW`, `POWER` | power, W |
  | `F`, `FREQ`, `FREQUENCY` | optical frequency, Hz |

  The points are start, start+step, start+2*step, ... up to and including stop when (stop-start)/step is an
  integer: `.dc WL(cwsrc1) 1.535u 1.565u 1n` has 31 points (30 before sprint 2026-10b, the stop was excluded).
  The values are accumulated, so the last digits can differ from start + i*step (the last point above is
  1.5650000000000022e-06, backlog B-009). Needs start < stop, step > 0 and at least 2 points
  (`Invalid range parameters`, `Sweep order must contain at least 2 point.`). One sweep per `.dc`.
- `.tran [duration]`: transient (time domain), after an operating point. The duration is in s (number, `{name}`
  or `{expression}`; `.tran 2*5n` is a syntax error). Without it the run goes on until no event is left.

Convergence: `.op`, every `.dc` point and the operating point before `.tran` stop after `maxiter` iterations
(delta cycles; default 1000000, `.options maxiter=N`, `--maxiter N`, 0 for no limit) with an error, a residual
report and exit status 1. See "Convergence of `.op` and `.dc`" in [numerics.md](numerics.md).

## Directives

- `.options key=value ...` (keys case-insensitive): `abstol`, `reltol`, `snapfactor`, `timescale` (alias `ts`),
  `resolution`, `seed`, `maxiter`, and the expert option `maxevents`: see [numerics.md](numerics.md). `traceall` (0 or 1): trace every optical net
  (automatic probes `PROBE{<net>}`). Without it, a netlist with probes (PROBE, MLPROBE, also inside a subcircuit)
  or `.save` targets traces only them, and a netlist with neither traces every optical net (F-05, see "PROBE, MLPROBE and traced nets"
  below). An unknown key stops with `Unknown keyword: foo (value: 1 (INTEGER))`.
- `.nodeset`: disabled, stops with `NODESET is disabled pending bugfixes` (exit status 1).
- `.ic`: rejected, `the .ic directive is not supported`.
- `.save target ...` (F-05): trace the listed optical nets. A target is a net name (power, field modulus, phase
  and wavelength, like a PROBE without arguments) or `QTY(net)` for one quantity, with the PROBE keywords
  (case-insensitive): `P`/`POW`/`POWER`, `E`/`MOD`/`MODULUS`/`MAG`/`MAGNITUDE`, `PHI`/`PHASE`, `WL`/`WAVELENGTH`.
  Example (the paper's Fig. 1b): `.save P(out) P(drop)`. Each saved net gets one probe named like the automatic
  probes, `PROBE{<net>}` (a bidirectional net has two signals, `PROBE{<net>:0}` and `PROBE{<net>:1}`), so its VCD
  variables are the same as with `traceall`; several targets on one net add up (`.save P(out) PHI(out)`). A
  netlist with `.save` traces only its targets and probes (see `traceall`). With an explicit `.options
  traceall=1`, the other nets keep their automatic probes and the saved nets trace what `.save` lists. Errors:
  `Error: .save nope: no net named 'nope' in the netlist`, `... 'v' is not an optical net (EANALOG)`, and a parse
  error for an unknown quantity (`I(out)`). Only top-level nets: like every directive, a `.save` inside a
  subcircuit is ignored with a warning (`found a directive in subcircuit, ignoring.`), and the internal nets of a
  subcircuit instance cannot be named.
- `.title text` and `.end`: ignored (KiCad netlists). `.end` does not stop the parsing.

## Command line

| Option | Meaning |
| --- | --- |
| `-f file`, `--file=file` | netlist; repeat the option for several files, read in order as one netlist |
| `-n`, `--dryrun` | parse and elaborate, do not simulate |
| `--json file` | write the flattened circuit as JSON |
| `-o name`, `--output=name` | trace file base name (`<name>.vcd`, `.csv`, `.npz`, `<name>.manifest.json`); its directory must exist; default `traces/delete_me` |
| `--format vcd\|csv\|npz` | output format(s) of the traces, repeatable (`--format vcd --format csv`); default `vcd` only; see "Tracing and output files" (F-05) |
| `--no-trace` | write no trace file and no run manifest file, as `-o ""` (F-05); wins over `-o`. Netlists only: the `-t` testbenches choose their own trace files |
| `--param name=value` | set a top-level parameter (see Parameters) |
| `--maxiter N` | `.options maxiter`, overrides the netlist |
| `--stats[=ports,devices]`, `--stats-file path`, `--print-stats` | run statistics (event counters), `<name>.stats.json`; see "Run statistics" in [numerics.md](numerics.md) |
| `--progress[=s]` | a progress line on stderr every s seconds of wall-clock time (default 1) |
| `--max-events N` | expert option `.options maxevents=N`, overrides the netlist |
| `--abstol x`, `--reltol x`, `-R n` | tolerances and timescale, see [numerics.md](numerics.md) |
| `--seed N` | master seed, see [reproducibility.md](reproducibility.md) |

`specs -h` lists all options. `-t name` runs a legacy C++ testbench instead of a netlist (`-p` lists them).

## Tracing and output files

What is traced (F-05): the probes (PROBE, MLPROBE) and the `.save` targets of the netlist; when it has none,
every optical net through an automatic probe `PROBE{<net>}` (`.options traceall=0|1` forces either). The
photodetectors (`<name>.readout`) and power meters are always traced in the VCD.

Where, with `-o name` (default `traces/delete_me`) and `--format` (default `vcd`; repeat it for several):

- `vcd`: `<name>.vcd`, every traced variable (probe power, abs, phase, wavelength; PDET readouts), one record per
  time step where a value changed, reals printed by SystemC with 16 significant digits.
- `csv`: `<name>.csv`, a table of the probe samples (no PDET or power meter: a warning says so when there is no
  VCD), one row per **time step, probe and wavelength**, holding the last sample of that wavelength in that time
  step: the delta cycles of one time step (the iterations of a `.op`/`.dc` fixed point, zero-delay propagation)
  are not physical time, and the VCD also only shows the end of a time step. A `.dc` sweep gives one row per
  point and probe (the converged value). Rows in time order; within a time step, in the order of the first
  sample of each (probe, wavelength). Columns:

  | column | unit | content |
  | --- | --- | --- |
  | `time_s` | s | simulation time (the sweep point index times the time resolution in a `.dc`) |
  | `signal` | | probe name (`ROOT/PROBE1`, `PROBE{ROOT/out}`, `ROOT/X1/MLPROBE1`) |
  | `wavelength_m` | m | wavelength of the sample |
  | `power_W` | W | \|E\|² |
  | `field_re`, `field_im` | √W | complex field E (modulus and phase: `abs(E)`, `angle(E)`) |

  Numbers are written in the shortest form that reads back to the same double (more precise than the VCD). A
  probe has no row before its first sample (the VCD shows 0 there), and an MLPROBE only has rows at its listed
  wavelengths. `.save` quantities only select VCD variables: the table always has every column.
- `npz`: `<name>.npz`, the same rows as a NumPy archive (written without any dependency, not compressed): one
  array per column, `time_s`, `wavelength_m`, `power_W`, `field_re`, `field_im` (float64) and `signal` (int32, an
  index into `signal_names`, a unicode array of every probe). With NumPy and pandas:

  ```python
  z = np.load("traces/out.npz")
  df = pd.DataFrame({k: z[k] for k in ("time_s", "wavelength_m", "power_W", "field_re", "field_im")})
  df["signal"] = z["signal_names"][z["signal"]]
  ```

  The `.npz` is kept in memory until the end of the analysis (48 bytes per row); above 4 GiB, use `csv`.

The run manifest `<name>.manifest.json` (F-01) is written when any of these files is; its `trace_file` is the
VCD, or the first table (`.csv`, then `.npz`) without a VCD. `--no-trace` (or `-o ""`) writes nothing.
`--format` and `--no-trace` apply to netlists; the `-t` testbenches write their own VCD.

## Legacy netlists

`circuits/legacy/` holds three pre-release netlists that do not parse with this syntax; see its `README.md`.

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
(the wavelength must match exactly). A netlist without any probe or `.save` traces every optical net through
automatic probes, traced as `PROBE{<net>}`; a netlist with probes or `.save` targets traces only those, unless
`.options traceall=1` asks for the automatic probes too (F-05; before, `traceall` defaulted to 1 and every net was always traced). Nets whose
name starts with `_` get no automatic probe.

**PROBE is for single-wavelength nets.** A net holds one value, the last one written to it, of any wavelength. On
a net that carries several wavelengths, a PROBE (and an automatic `PROBE{<net>}`) therefore shows, in each time
step, one of the wavelengths written in that step, in an unspecified order (it depends on the order in which the
simulation kernel runs the devices, which changed in v0.3, F-15). Use MLPROBE for per-wavelength values; they do
not depend on that order.

In `.tran`, the probes are disabled during the operating point. When the transient starts, after the operating
point (`.nodeset` included) and after any initial conditions, every probe samples the current value of its net
(B-048, sprint 2026-10b): a net set by a CW source shows its operating-point value from t = 0, as printed in the
operating-point listing on stdout. Before, such a net read 0 in the VCD for the whole transient. A net that was
never written (undriven, or fed only by sources that start later) stays at 0 until its first event.
