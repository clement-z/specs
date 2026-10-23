# F-03: S-parameter device (`SPAR`): Touchstone and Lumerical/SiEPIC `.sparam` import

value 5 | effort M (frequency-domain semantics) / M+ needs-human (broadband time-domain) | risk med |
prerequisites: none (the generic device it was to refactor is removed, decision 2026-10-23) | horizon 2

> **Status 2026-10-23: phase 1 implemented (sprint 2026-10c, phase PF03, branch `ws/spar`, report
> `backlog/sprint-2026-10c/PF03-REPORT.md`).** Design items 1-3 and 5 and the validation plan: element `SPAR`
> (`doc/netlist.md`), loader `src/io/sparam_file.{h,cpp}` (+ the Taylor form, decision 2026-10-23), device
> `src/devices/sparam_device.{h,cpp}`, export `utils/sparam_export.py`, formats and conventions
> `doc/sparam.md`, tests `tests/sparam/` and 11 golden cases. Open questions answered by the decisions:
> phases converted to SPECS's convention (Touchstone conjugated, `.sparam`/CSV kept), Taylor form yes, out of
> range is an error. Not done: phase 2 (broadband time-domain realisation, needs-human), `.npz` export, the
> SiEPIC BDC fixture (licence of the Simphony data unclear, not committed), reciprocity fill.

> **Update 2026-10-23, phase PF03b (branch `ws/spar2`, report `backlog/sprint-2026-10c/PF03b-REPORT.md`),
> after the SPAR discussion (`backlog/sprint-2026-10c/SPAR-discussion.md`).** `.tran` uses one delay per path
> (delayed copies, replacing one delay per output); negative group delays are still clamped to 0, reported
> per path; a grid-adequacy warning at load time (leave-one-out); real/imaginary interpolation near nulls;
> data files (S-parameter, VLSRC) in the run manifest; the SiEPIC BDC fixture is committed
> (`tests/sparam/data/siepic`, MIT) with the brief's `.dc` case (`cir/spar-siepic-bdc`); the ring test file
> re-exported on a 0.05 nm grid. Still not done: phase 2, `.npz` export, reciprocity fill.

> **Update 2026-10-23, phase PF03c (branch `ws/spar3`, report `backlog/sprint-2026-10c/PF03c-REPORT.md`).**
> `delay_per="path"` (default) or `"output"` (the per-output delay of phase 1, kept as an option); B-190: near
> nulls, the real and imaginary parts of each entry are interpolated with its median group delay taken out (an
> unbalanced MZI with 28 points per FSR: 0.023 -> 0.0016 between grid points).

> **Update 2026-10-23, phase PF03d (branch `ws/spar4`, report `backlog/sprint-2026-10c/PF03d-REPORT.md`), after the
> second SPAR discussion (`backlog/sprint-2026-10c/SPAR-discussion-2.md`).** `delay_per` removed
> (`delay="auto"|"none"|<s>`, one delay per path); the grid check decides real/imaginary interpolation on the two
> actual intervals around a point; with `interp=pchip` the magnitude/phase slopes are taken per run between the
> real/imaginary intervals (the MZI 0.0165 -> 9.2e-4). The pulse-pair estimate of the removed delay is prepared on
> `PF03d-wip/item4` but not merged (a golden change beyond the prediction: see the report).

> **Update 2026-10-23 (maintainer decision, phase D2b).** `GenericTransmissionDevice`, `Generic2x2Coupler`,
> `GenericWaveguide` and the output ports' delta-write mode (`m_use_deltas`) are removed (54b15dc). F-03 is to be
> written **from scratch** as a state-holding, per-wavelength device: one memory per input port and wavelength,
> and on every input event a full recomputation of the outputs from those memories, with absolute-value writes,
> like the other multi-input devices (merger, directional coupler). The removed code, referenced below with its
> former line numbers, is in the history at cc1e52e.

## Problem / user story
*As a designer I have an FDTD, EME or measured S-matrix of a coupler, grating, MMI or PDK cell and want to drop it
into a SPECS netlist.* `todo.txt` item 1, GitHub issue #4, and the paper's claim of "generic S-matrix devices".
Until 2026-10-23, `GenericTransmissionDevice` was abstract (`src/devices/generic_transmission_device.h:67-94`, `prepareTM()`
pure at `:82`) with a Taylor-coefficient matrix (`:19-65`); its process latched `S_ij` at the first wavelength
(`generic_transmission_device.cpp:181-183`) and called `sc_stop()` on any other wavelength (`:202-208`), ran in
delta mode (`:76`, which the DC sweep reset broke) and spawned one process per active (i, j) (`:88-94`).
`GenericWaveguide` was disabled with `exit(1)` (`src/devices/generic_waveguide.h:18-19`) and had formula errors
(B-040). All of it was removed on 2026-10-23 (code at cc1e52e). No netlist element exists. Every comparable tool imports S-parameters (SOURCES A matrix); the SiEPIC EBeam
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
3. **Runtime semantics (phase 1, exact for CW/`.dc` and narrowband pulses).** A new `SParamDevice`, written from
   scratch (decision 2026-10-23; not a refactor of the removed `GenericTransmissionDevice`): state-holding, one
   memory per input port and wavelength; on an input event, a full recomputation of the outputs at that
   wavelength from the memories, with a per-wavelength `S_ij(lambda)` lookup (cached per wavelength), written as
   absolute values like every other device; event delay = round(tau_g(lambda)) ticks (same rounding as
   `src/devices/waveguide.cpp`), one process per input port writing all active outputs. This is
   the "per-wavelength S" model Photontorch uses; dispersion *within* a pulse's bandwidth is ignored and
   documented.
4. **Phase 2 (needs-human).** Broadband time-domain realisation: FIR/IIR fit in a sampled island (F-08) or an
   edge-staircase, with INTERCONNECT's tap/tolerance knobs as the user interface.
5. **Export** (F-13): write a `.dc` sweep as `.sparam`/Touchstone/`.npz` so SPECS blocks can be reused.

## Touch points
new `src/devices/sparam_device.{h,cpp}` (the former `generic_transmission_device.{h,cpp}`,
`generic_waveguide.h` and `generic_2x2_coupler.h`, at cc1e52e, are reference only) · `src/parser/parse_element.{h,cpp}` (new
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
Loader + fixtures 3 d · element + parser plumbing 2 d · device (new) 3 d · export 1 d · tests/docs 2 d.
Phase 2 is a separate M+ item.

## Open questions for the maintainer
- Phase sign convention: SPECS waveguides apply `exp(+j beta L)` while the detector beat term uses
  `exp(+j omega t)` (discovery report 02 §4); Lumerical/scikit-rf phases follow `exp(-j omega t)`. Decide
  whether to conjugate on import and document it once.
- Is a Taylor-coefficient form (as the removed `TransmissionMatrix`, cc1e52e) wanted as an input format
  (`format=taylor`)?
- Default behaviour outside the file's frequency range: clamp, extrapolate, or error (proposed: error).
