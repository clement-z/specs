# S-parameter files (`SPAR`)

The netlist element `SPAR` (`doc/netlist.md`, "SPAR") reads the S-matrix of an N-port device from a file
and uses it at each wavelength the device receives. This page describes the file formats, the phase
convention and what the device does with the data.

## Model

- `S[j][i]` is the transmission (field, √W/√W) from port `i` (input) to port `j` (output): the outgoing
  fields are `b_j = sum_i S[j][i] a_i`. Ports are numbered from 1 in the files and in this page.
- **Phase convention.** SPECS waveguides apply `exp(+j beta L)` (`beta = 2 pi neff / lambda`): a delay
  makes the transmission phase grow with frequency and the group delay is `tau = +dphi/domega`. Every
  file is converted to this convention when it is read. The conversion of each format follows its
  documented time convention:

  | Format | Time convention of the file | Delay line in the file | Conversion |
  | --- | --- | --- | --- |
  | Lumerical/SiEPIC `.sparam` | physics, `exp(-i omega t)` (Lumerical's FDTD/INTERCONNECT) | `exp(+i beta L)` | none |
  | Touchstone 1.x/2.x | RF engineering, `exp(+j omega t)` (Touchstone/IBIS, scikit-rf) | `exp(-j beta L)` | complex conjugate |
  | CSV (below) | physics (SAX, gplugins) | `exp(+i beta L)` | none |
  | Taylor form (below) | SPECS | `exp(+j beta L)` | none |

  `convention=physics|engineering` on the netlist element overrides the default of the format (e.g. for a
  Touchstone file written with the physics convention). A test proves the conversion: a plain waveguide,
  written analytically in each format by `tests/sparam/gen_fixtures.py`, gives through `SPAR` the same
  transmission phase and the same delay as a `WG` of the same length (`tests/sparam/test_sparam.py`).
- **Interpolation.** For tabulated data (every format but the Taylor form), each `S[j][i]` is stored on its
  frequency grid as a magnitude and an **unwrapped** phase (unwrapped along increasing frequency, steps
  taken within (-pi, pi]), and as real and imaginary parts. Between two grid points it is interpolated in
  frequency, linearly (`interp=linear`, the default) or with a monotone cubic (`interp=pchip`,
  Fritsch-Carlson, on each quantity with its own slopes), and the quantities depend on the interval:
  - magnitude and phase, separately (exact for a delay line, whose phase is linear in frequency), away from
    transmission nulls;
  - near a null, real and imaginary parts **of the entry with its delay taken out** (B-190):
    `S~(f) = S(f) exp(-j 2 pi (f - f_ref) tau0)`, the delay put back after the interpolation. `tau0` is the
    median of the entry's group delays at the grid points (below), `f_ref` its first grid point; `tau0` is 0
    when it would turn S by less than 1e-3 rad over the largest grid step. Near a null the phase turns
    quickly and S passes close to 0 along an almost straight line, which magnitude/phase interpolation
    follows badly; on a path that also rotates fast (an unbalanced MZI: two delays), that line is straight
    only once the common delay is taken out. An interval is "near a null" when |S| at either end, or the
    segment between `S~` at its two ends, comes closer to 0 than 0.07 times the largest |S| of the entry
    (the segment test catches a null between two points whose |S| is larger).

  The choice is made per interval, so S stays continuous; at a grid point the file's value is used as is.
  The threshold 0.07 was chosen from the error between grid points of exported devices (PF03b report): the
  add-drop ring of `circuits/add_drop.cir` on a 0.05 nm grid, evaluated between its points, is within 4e-4
  (linear) or 1e-4 (pchip) of the circuit where |S| >= 0.1, and within 6e-4 (linear and pchip) at its
  through-port null at 1.55 um, where magnitude/phase interpolation alone gave 0.020 and 0.015
  (`tests/sparam/test_sparam.py`, round-trip/between-points); with the delay removed, the results are the same
  for thresholds from 0.03 to 0.1. Measured between the grid points of exports (PF03c report), the delay
  removal takes an unbalanced MZI sampled 28 times per free spectral range from 0.023 to 0.0016 (linear) and
  the ring from 9.7e-4 to 5.4e-4; delay lines, the SiEPIC coupler and smooth dispersive paths are unchanged
  (no interval near a null). With `interp=pchip` the magnitude/phase intervals are as without delay removal
  (PCHIP on the detrended phase would help the MZI more, 0.0035 instead of 0.016, but degrades smooth
  dispersive paths: PF03c report). The grid must be fine enough for the phase to
  change by less than pi between two points (else the unwrapping, and the group delay, are wrong), and to
  resolve the device's features: see "Grid check".
- **Range.** A wavelength outside the data range is an error, with the range in the message (relative
  tolerance 1e-12 at the ends, as for wavelength matching, `doc/numerics.md`). The range of a tabulated file
  is the intersection of the grids of its non-zero entries; the Taylor form gives it explicitly.
- **Group delay.** `tau[j][i](f) = dphi/domega` of the unwrapped phase (SPECS convention), at each grid
  point by finite differences (three points with non-uniform spacing inside the grid, two at its ends),
  then interpolated linearly in frequency (whatever `interp` and the interval's interpolation). The Taylor
  form gives it or derives it from its phase polynomial.
- **Grid check.** When a tabulated file is loaded, each entry the device uses (unidirectional: from an input
  to an output) is checked at each interior grid point: S interpolated there from the grid without that
  point (leave-one-out, with the device's own interpolation above) is compared with the file's value. When
  the largest difference exceeds 0.02 (field), one warning per device names the path, the wavelength and the
  difference: the grid does not resolve a feature there (a resonance or a null narrower than the spacing),
  so the values between those points and the group delays near them are unreliable. Leaving a point out
  doubles the spacing, so 0.02 stands for about 0.005 between the points of the actual grid. A delay line
  gives 1e-12, the SiEPIC broadband coupler (`tests/sparam/data/siepic`) 0.0024, the add-drop ring on a
  0.05 nm grid 0.0021 and an unbalanced MZI sampled 28 times per free spectral range 0.0064 (no warning;
  0.043 with `interp=pchip`, a warning); the ring on a 1 nm grid gives 0.71 (a warning). Before the delay
  removal (B-190) the MZI gave 0.11 and the ring 0.0039. The check tests S, not the group delay: the ring's
  through-port null, whose phase turns within about 4 pm, passes on the 0.05 nm grid (S between the points
  is within 6e-4 there) while its group delay is not resolved (-37 ps instead of -621 ps); so do the MZI's
  nulls. Not applicable to the Taylor form.
- **Passivity.** For each input `i`, the power leaving the device, `sum_j |S[j][i]|^2`, is checked at every
  grid point of the file (the Taylor form: 201 points over its range). `passivity=check` (default) warns
  once per device when it exceeds 1.03 (INTERCONNECT's threshold), with the worst input and wavelength;
  `passivity=ignore` does not check; `passivity=enforce` checks like `check` and, at each wavelength the
  device uses, scales the whole matrix by `1/sqrt(max_i sum_j |S[j][i]|^2)` where that maximum exceeds 1
  (a simple scaling, not a passivity enforcement that keeps the matrix closest to the data).

## Formats

The format comes from `format=` or, by default, from the file extension: `.sparam`, `.dat` → `sparam`;
`.sNp` (`.s2p`, `.s4p`, ...) and `.ts` → `touchstone`; `.csv` → `csv`; `.taylor` → `taylor`.
Port names come from the file (below); `ports="a,b,..."` on the netlist element picks the file ports, by name
or by 1-based index, in the order of the element's nets.

### Lumerical/SiEPIC `.sparam` (INTERCONNECT "Optical N-port S-parameter")

    ["port 1","LEFT"]                                  optional port declarations, in port order
    ["port 2","RIGHT"]
    ("port 2","TE",1,"port 1",1,"transmission")        block header: output port, mode, mode id, input port, mode id
    (51,3)                                             number of rows, 3 columns
    1.8737e+14  0.70857  -0.594472                     frequency (Hz), |S|, phase (rad)
    ...

- Quotes may be single or double. Ports are numbered in the order of the declaration lines, or, without
  them, in the order in which they first appear in the block headers (output, then input).
- One mode per port; a port that appears with two different modes, a header with a group-delay field
  (7th field), a block type other than `transmission` and a column count other than 3 are errors (not
  supported).
- Pairs without a block are 0.

### Touchstone 1.x and 2.x

- Version 1: the port count comes from the extension (`.s4p`: 4 ports). Option line
  `# <unit> S <format> R <z>`: unit `Hz|kHz|MHz|GHz` (default GHz), format `RI|MA|DB` (default MA; DB is
  `20 log10 |S|`, angles in degrees), the reference impedance is ignored; only S-parameters (`S`) are
  supported. Data: a frequency, then the N^2 values (pairs), over as many lines as needed; for 2 ports the
  order is S11 S21 S12 S22, for N >= 3 row by row (S11 S12 ... S1N, S21 ...). A 2-port file's noise data
  (the rows after the frequency decreases) are skipped.
- Version 2 (`[Version] 2.0` or `2.1`): `[Number of Ports]`, `[Two-Port Data Order] 12_21|21_12` (2-port
  files), `[Number of Frequencies]`, `[Matrix Format] Full|Lower|Upper`, `[Network Data]` ... `[End]`;
  `[Reference]` is ignored; mixed-mode data and noise data are not supported.
- Port names: comments `! Port[1] = name` (INTERCONNECT writes them; the name is the first quoted string
  if there is one) or `! ports: a, b, c`; else `1`, `2`, ...
- Phases are conjugated on import (engineering convention, above).

### CSV

    wavelength_m,"o1@0,o2@0_re","o1@0,o2@0_im","o2@0,o1@0_re","o2@0,o1@0_im"
    1.5e-06,0.97,0.12,0.97,0.12
    ...

- A header line, then one line per wavelength (any order). The first column is `wavelength_m`; the others
  are pairs of columns `<in>@<mode>,<out>@<mode>_re` and `_im` (the SAX/gplugins key from input to output,
  quoted because it contains a comma), real and imaginary parts of `S[out][in]`. Mode 0 only.
- Ports are numbered in the order in which they first appear in the header (input, then output). Pairs
  without columns are 0.

### Taylor form (SPECS)

    specs-taylor 1                       first line: format and version
    ports in out                         port names (or a count: "ports 2")
    center 1.55e-6                       wavelength lambda0 (m) of the expansions
    range 1.5e-6 1.6e-6                  wavelength range (m) where the expansions hold; outside: error
    S out in amp 0.97                    |S[out][in]| = c0 + c1 d + c2 d^2 + ...,  d = lambda - lambda0 (m)
    S out in phase 912.5 -4.1e8 2.6e14   phase (rad, SPECS convention) = c0 + c1 d + ...
    S out in delay 1.4e-12               group delay (s) = c0 + c1 d + ...; optional
    S in out amp 0.97                    ports by name or by 1-based index
    S in out phase 912.5 -4.1e8 2.6e14

- Plain polynomial coefficients in `d = lambda - lambda0` (not derivatives). Keywords are case-insensitive;
  `#` starts a comment. Pairs without `amp` are 0; `phase` defaults to 0. Without `delay`, the group delay
  is derived from the phase: `tau = dphi/domega = -(lambda^2 / (2 pi c)) dphi/dlambda`.

## Behaviour in the simulation

- `.op`, `.dc`: the outputs are `b = S(lambda) a` at each wavelength, exactly (up to the interpolation).
- `.tran`: **one delay per path**. The device keeps a delayed copy of the field of each path (input `i` to
  output `j`) and wavelength. An input event at time t with field `a_i` reaches the end of each path it
  starts at t + `tau[j][i]` (the delay of that path at that wavelength): the copy `m_ji` then becomes `a_i`,
  and output `j` is recomputed, `b_j = sum_i S[j][i] m_ji`, and written without further delay. The writes to
  an output are therefore in time order, and paths with different delays into one output arrive at their
  own times (two waveguides of different lengths into a coupler give, through one `SPAR`, the transient of
  the explicit circuit, `tests/sparam/test_sparam.py` per-path-tran). The output port merges values written
  for the same time and wavelength and applies its emission tolerances as for any device. **The S-matrix is
  the one of the carrier wavelength: dispersion within a pulse's bandwidth is ignored** (a broadband
  time-domain model, e.g. a fitted filter, is not implemented): a path whose response is not a delay times
  a constant over the pulse's bandwidth (a resonance, a null) is only approximated. The steady state is
  exact. In `.op` and `.dc` (and the operating point before a `.tran`) there is no delay.
- Delay (`delay=`): `auto` (default) uses for each path and wavelength the group delay of the data, rounded
  to the time step like a waveguide's. `delay=none` is 0; `delay=<seconds>` is that delay on every path. A
  delay that is rounded badly (B-014) gives one warning per device in `.tran`; a `delay=<seconds>` that
  rounds to 0 time steps is an error, like a waveguide's length. Cost: one entry in the device's timed queue
  per delayed path and input event.
- Negative group delay (`delay=auto`): a passive device can have one (the through port of an undercoupled
  ring near resonance; the finite-difference noise of a constant phase), but it only describes pulses whose
  spectrum fits inside the phase feature, and the front of a signal is never advanced: it is **replaced by 0**
  for that path (the smallest timing error for the signals the model describes; F-03 phase 2 would give
  such paths a filter). In `.tran` it is reported once per path, when the path is first used and the value
  is at least half a time step, with the path (port names), the wavelength and the value; when the value
  depends on grid points flagged by the grid check, the warning says it is probably an artefact of the grid.
  A clamped path is a connection without delay: inside a feedback loop it makes a loop without delay, which
  nothing else reports.
- Unidirectional and bidirectional, like `COUPLER`: with an even number of nets and no bidirectional net,
  the first half are inputs and the second half outputs. Light then only enters the inputs, and only the
  paths from an input to an output are used: the paths from an input back to an input (reflections,
  couplings between inputs) are dropped, with one warning giving the largest when it exceeds -40 dB. With an
  odd number of nets, or if one of its nets is bidirectional (`&`, or two writers), the device is
  bidirectional and uses every entry.
- One `SC_METHOD` reacts to all the inputs and to the device's timed queue: in a delta cycle it takes the
  arrivals due, then handles every input with a new value, in net order.
- String values of the keywords are written in quotes (`format="touchstone"`, `delay="none"`).

## Export: `utils/sparam_export.py`

Writes the S-matrix of a subcircuit, from one `.dc` wavelength sweep per port, in any of the formats above
(but the Taylor form):

    python3 -I utils/sparam_export.py --specs build/specs --netlist ring.cir --subckt ring \
        --wl 1.535e-6 1.565e-6 0.05e-9 --format sparam -o ring.sparam -j 8

For each port `k`, it runs a `.dc` sweep of a CW source (power 1 W, phase 0) on that port, with every port
saved (`.save`), and reads the traces (`--format csv`): `S[j][k]` is the field leaving port `j`. On the
excited port the field written by the source is told apart from the reflected one (bidirectional nets);
on a unidirectional device the reflections are 0. See `--help` for the options (tolerances of the runs,
default `abstol=1e-12 reltol=1e-9`; port names; Touchstone version).

- The `--netlist` file must only define the subcircuit (and what it needs): a top-level element or an
  analysis in it ends up in every run.
- Exciting an output port of a unidirectional subcircuit gives its net two writers (the source and the
  device), so SPECS makes the device bidirectional for that sweep: those columns are the bidirectional
  model's (the same values for the reciprocal devices of SPECS).
- The grid must resolve the device (above): the tool does not check it, `SPAR` does when it loads the file
  ("Grid check").
- Round trip (`tests/sparam/test_sparam.py`): the ring of `circuits/add_drop.cir` exported at the grid of
  its sweep and used as one `SPAR` gives the circuit's fields within 7.2e-7 (the circuit's own reltol is
  1e-6), in each of the three formats.
