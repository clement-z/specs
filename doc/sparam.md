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
  taken within (-pi, pi]), and both are interpolated in frequency, separately: linearly (`interp=linear`,
  the default) or with a monotone cubic (`interp=pchip`, Fritsch-Carlson). At a grid point the file's value
  is used as is. The grid must be fine enough for the phase to change by less than pi between two points
  (else the unwrapping, and the group delay, are wrong). Near a transmission null the phase turns quickly
  and this interpolation is poor: the add-drop ring of `circuits/add_drop.cir` exported on a 0.05 nm grid
  and evaluated between its points is within 4e-4 (linear) or 1e-4 (pchip) of the circuit where
  |S| >= 0.1, but 0.02 (field) at the through-port null at 1.55 um, where |S| = 0.03
  (`tests/sparam/test_sparam.py`, round-trip/between-points).
- **Range.** A wavelength outside the data range is an error, with the range in the message (relative
  tolerance 1e-12 at the ends, as for wavelength matching, `doc/numerics.md`). The range of a tabulated file
  is the intersection of the grids of its non-zero entries; the Taylor form gives it explicitly.
- **Group delay.** `tau[j][i](f) = dphi/domega` of the unwrapped phase (SPECS convention), at each grid
  point by finite differences (three points with non-uniform spacing inside the grid, two at its ends),
  then interpolated linearly in frequency. The Taylor form gives it or derives it from its phase
  polynomial.
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
- `.tran`: the device holds the last input field of each port and wavelength; on every input event it
  recomputes the outputs it reaches at that wavelength and writes them after a delay. **The S-matrix is the
  one of the carrier wavelength: dispersion within a pulse's bandwidth is ignored** (a broadband
  time-domain model, e.g. a fitted filter, is not implemented).
- Delay (`delay=`): `auto` (default) uses, for each output port and wavelength, the group delay of the
  largest (|S| at that wavelength) of the paths into that port, rounded to the time step like a waveguide's;
  all the writes to that output use it, so that the last write is always the newest value (with a delay per
  path, a later input event could be written before an earlier one and leave a stale output). Paths with
  different delays into the same output therefore arrive together: exact in steady state, approximate in
  transients. A negative group delay (the through port of a ring near resonance, or the finite-difference
  noise of a constant phase) is replaced by 0; in `.tran` it is reported once per device when it is at least
  half a time step. `delay=none` is 0; `delay=<seconds>` is that delay on every path. A delay that is
  rounded badly (B-014) gives one warning per device in `.tran`; a `delay=<seconds>` that rounds to 0 time
  steps is an error, like a waveguide's length.
- Unidirectional and bidirectional, like `COUPLER`: with an even number of nets and no bidirectional net,
  the first half are inputs and the second half outputs. Light then only enters the inputs, and only the
  paths from an input to an output are used: the paths from an input back to an input (reflections,
  couplings between inputs) are dropped, with one warning giving the largest when it exceeds -40 dB. With an
  odd number of nets, or if one of its nets is bidirectional (`&`, or two writers), the device is
  bidirectional and uses every entry.
- One `SC_METHOD` reacts to all the inputs: in a delta cycle it handles every input with a new value, in
  net order.
- String values of the keywords are written in quotes (`format="touchstone"`, `delay="none"`).

## Export: `utils/sparam_export.py`

Writes the S-matrix of a subcircuit, from one `.dc` wavelength sweep per port, in any of the formats above
(but the Taylor form):

    python3 -I utils/sparam_export.py --specs build/specs --netlist ring.cir --subckt ring \
        --wl 1.535e-6 1.565e-6 1e-9 --format sparam -o ring.sparam

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
- The grid must resolve the device's phase (above): the tool does not check it.
- Round trip (`tests/sparam/test_sparam.py`): the ring of `circuits/add_drop.cir` exported at the grid of
  its sweep and used as one `SPAR` gives the circuit's fields within 7.2e-7 (the circuit's own reltol is
  1e-6), in each of the three formats.
