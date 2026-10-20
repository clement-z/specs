# F-03: S-parameter device (`SPAR`): Touchstone and Lumerical/SiEPIC `.sparam` import

value 5 | effort M (frequency-domain semantics) / M+ needs-human (broadband time-domain) | risk med |
prerequisites: fix the generic device's single-wavelength latch (backlog) | horizon 2

## Problem / user story
*As a designer I have an FDTD, EME or measured S-matrix of a coupler, grating, MMI or PDK cell and want to drop it
into a SPECS netlist.* `todo.txt` item 1, GitHub issue #4, and the paper's claim of "generic S-matrix devices".
Today `GenericTransmissionDevice` is abstract (`src/devices/generic_transmission_device.h:67-94`, `prepareTM()`
pure at `:82`) with a Taylor-coefficient matrix (`:19-65`); its process latches `S_ij` at the first wavelength
(`generic_transmission_device.cpp:181-183`) and calls `sc_stop()` on any other wavelength (`:202-208`), runs in
delta mode (`:76`, which the DC sweep reset breaks) and spawns one process per active (i, j) (`:88-94`).
`GenericWaveguide` is disabled with `exit(1)` (`src/devices/generic_waveguide.h:18-19`) and has formula errors.
No netlist element exists. Every comparable tool imports S-parameters (SOURCES A matrix); the SiEPIC EBeam
library alone ships 44 `.sparam` files inside Simphony; Parini et al. fed FDTD S-matrices into SystemC blocks in
2012 (SOURCES B).

## Prior art
- INTERCONNECT "Optical N Port S-Parameter": `.dat` text with blocks `("out","TE",1,"in",1,"transmission")`,
  `(Nf, 3)`, rows `f_Hz |S| phase_rad`, optional group-delay column (phase stored minus 2 pi tau_g (f - f_c)),
  port-side metadata, Touchstone since 2021R1.1 with `! Port[i] = {"name","mode",id,"SIDE"}` comments,
  passivity/reciprocity test/enforce, FIR/IIR fitting for time domain.
- Touchstone 2.1 (IBIS 2024): `# Hz S RI|MA|DB R n`, `[Number of Ports]`, `[Two-Port Data Order]`,
  `[Matrix Format]`; scikit-rf as reference reader/writer; SAX `parse_touchstone`/`parse_lumerical_dat`
  (via scikit-rf; `! ports:` comment); Simphony `load_sparams` (Lark grammar); gplugins `.npz` with keys
  `o1@0,o2@0`; PhotonForge pole-residue fitting; Ye et al. 2022 vector fitting (SOURCES A, C).

## Proposed design
1. **Netlist element.** `SPARn p1 p2 ... pN file="bdc.sparam" [format=lumerical|touchstone|csv]
   [ports="o1,o2,..."] [interp=linear|pchip] [passivity=ignore|check|enforce] [delay=auto|none|<s>]`.
   The port count comes from the file, so `SPARElement` needs a variable `n_nets` (the `DECLARE_*` macros fix
   it, `src/parser/parse_element.h:29-75`; `XElement` already collects nets as positional args,
   `src/parser/parser.y` element.x rules).
2. **Loader** `src/io/sparam_file.{h,cpp}`: Lumerical/SiEPIC blocks, Touchstone 1.x/2.x (RI/MA/DB, Hz units,
   port names from `! Port[i]`/`! ports:` comments), CSV `wavelength_m, o1@0,o2@0_re, ..._im`. Store per (i, j):
   frequency grid, |S| and **unwrapped** phase; interpolate magnitude and phase separately (gplugins PR 253);
   derive group delay tau_g(i,j) = -d phi/d omega (per frequency or band centre); reciprocity fill optional.
3. **Runtime semantics (phase 1, exact for CW/`.dc` and narrowband pulses).** Refactor
   `GenericTransmissionDevice` into `SParamDevice`: per-wavelength `S_ij(lambda)` lookup at event time (per-λ
   map instead of the latch at `:181-183`; delete the `sc_stop` at `:202-208`), absolute-value outputs (drop
   `m_use_deltas`, `:76`) so it behaves like every other device, event delay = round(tau_g(lambda)) ticks (same
   rounding as `src/devices/waveguide.cpp:72`), one process per input port writing all active outputs. This is
   the "per-wavelength S" model Photontorch uses; dispersion *within* a pulse's bandwidth is ignored and
   documented.
4. **Phase 2 (needs-human).** Broadband time-domain realisation: FIR/IIR fit in a sampled island (F-08) or an
   edge-staircase, with INTERCONNECT's tap/tolerance knobs as the user interface.
5. **Export** (F-13): write a `.dc` sweep as `.sparam`/Touchstone/`.npz` so SPECS blocks can be reused.

## Touch points
`src/devices/generic_transmission_device.{h,cpp}` (lines above) · `src/devices/generic_waveguide.h`,
`generic_2x2_coupler.h` (retire or re-express as `format=taylor`) · `src/parser/parse_element.{h,cpp}` (new
element, variable nets) · `src/parser/parser.l:362-457` (prefix `SPAR`), `src/parser/parser.y:248-392` ·
`src/devices/alldevices.h` · `utils/specs.kicad_sym` (symbol) · docs.

## Validation plan
- Round trip: `.dc` sweep of `circuits/add_drop.cir` exported as `.sparam`, loaded into a `SPAR` 4-port, swept
  again: equal within interpolation error (1e-4) at the grid points, 1e-6 with the same grid.
- SiEPIC `EBeam_1550_TE_BDC.sparam` (MIT licence) as a fixture: split ratio vs wavelength equals Simphony/SAX.
- scikit-rf-generated Touchstone of a pure delay line (`phi = -2 pi f tau`): event delay equals `tau`;
  `# GHz S DB` and 2-port `21_12` ordering handled.
- Passivity check flags a file with |S|^2 row sum > 1.03 (INTERCONNECT's warning threshold).
- Golden: a `cir/spar_bdc` case; unit test for the loader.

## Effort breakdown
Loader + fixtures 3 d · element + parser plumbing 2 d · device refactor 3 d · export 1 d · tests/docs 2 d.
Phase 2 is a separate M+ item.

## Open questions for the maintainer
- Phase sign convention: SPECS waveguides apply `exp(+j beta L)` while the detector beat term uses
  `exp(+j omega t)` (discovery report 02 §4); Lumerical/scikit-rf phases follow `exp(-j omega t)`. Decide
  whether to conjugate on import and document it once.
- Is `TransmissionMatrix`'s Taylor form still wanted (`format=taylor`) or can it go?
- Default behaviour outside the file's frequency range: clamp, extrapolate, or error (proposed: error).
