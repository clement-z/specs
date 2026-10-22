# F-14: Fabrication variability models (width/thickness to neff/ng, correlated sampling, wafer maps)

value 4 | effort M | risk med (needs-human for sensitivities and defaults) | prerequisites: F-01 (seeds),
F-02 (Monte-Carlo runner and `.param` distributions) | horizon 2

## Problem / user story
*As a PUF or MZI-mesh researcher I need device-to-device variation that is physically correlated, not just random
lengths.* Jimenez's PUF study simulated 20 instances with per-ring radius offsets in a custom code; the MZI-mesh
thesis lists fabrication variation among un-modelled threats; Cardoso measured a 7.73 % normalised variability
and ran 100-run averages elsewhere; Shastri et al. and Peserico et al. name variability trimming as a core
challenge (SOURCES B). Today the only randomness in SPECS is the CROW benchmark's per-waveguide lengths drawn in
C++ (`src/tb/crow_tb.cpp`), with no seed, and every waveguide takes `neff`, `ng` directly
(`src/devices/waveguide.h:11-53`): there is no notion of geometry, so a 5 nm width variation cannot be expressed.

## Prior art (SOURCES D §9, A)
Lu et al. 2017 (UBC/Lumerical): width and thickness maps extracted from 2074 racetracks on a 200 mm wafer,
"virtual wafer" layout-dependent Monte-Carlo with six numbers (mean, sigma, correlation length for width and
thickness). Columbia process-variation-aware strip-waveguide compact model (rational in width, Taylor in
wavelength, one PCA variable, Verilog-A). Sensitivity `dneff/dw ~ 3e-3 per nm` at 400 nm width on 220 nm SOI,
thickness sensitivity larger. Xing/Bogaerts 2023 hierarchical wafer/die/local model. Tools: INTERCONNECT
Monte-Carlo/corner on statistical compact models (AIM PDK), Luceda Circuit Analyzer (global/local/CMV),
Simphony `layout_aware_simulation(sigma_w, sigma_h, correlation_length)`, PhotonForge `RandomVariable`.

## Proposed design
1. **Geometry-aware waveguide parameters (elaboration time, no kernel change).** Optional kwargs on `WG`
   (and composites that build waveguides): `W`, `H` (nominal geometry), `DW`, `DH` (deltas), and sensitivities
   `DNEFF_DW`, `DNEFF_DH`, `DNG_DW`, `DNG_DH` (defaults for 220 nm SOI strip from the literature, overridable by a
   `MATERIAL`/`TECH` preset). `neff` and `ng` are corrected linearly at construction; everything downstream is
   unchanged. Golden-neutral when deltas are zero.
2. **Sampling.** Independent: `.param dw=gauss(0, 5e-9)` per instance (F-02 draws per `.subckt` instance at
   flatten time). Correlated: `.variation width sigma=5e-9 corr=4.5e-3 thickness sigma=2e-9 corr=...` plus
   optional `X=`, `Y=` coordinates on instances (or on `X` subcircuit instances, inherited by their devices);
   at elaboration a Gaussian random field with the given correlation length is sampled on the instance
   coordinates (Cholesky for tens of instances, or a Fourier-synthesised field on a grid for hundreds), and each
   waveguide gets its `DW`, `DH`. Coordinates can come from a gdsfactory netlist's `placements` (F-13).
3. **Corners.** `.corner width=+5e-9 thickness=-2e-9` as a deterministic variant of the same mechanism, run
   through the F-02 plan runner.
4. **Reporting.** The run manifest (F-01) records every sampled delta so a PUF instance can be regenerated;
   pyspecs aggregates yield histograms.

## Touch points
`src/devices/waveguide.{h,cpp}` (`waveguide.h:11-53` parameters; `waveguide.cpp:6-30` prologue) ·
`src/parser/parse_element.cpp:30-127` (WG kwargs) · `src/parser/parse_directive.cpp` (`.variation`,
`.corner`), `src/parser/parser.y` directive rules · `src/parser/parse_tree.cpp` (flatten: per-instance draws,
coordinates) · new `src/variation.{h,cpp}` (random field sampler) · `pyspecs` (wafer-map helpers, F-07) ·
composites (`mzi_active.cpp`, `crow.cpp`, `clements.cpp`, `octane_*`) to forward geometry kwargs.

## Validation plan
- Linear sensitivity unit test: `DW = 1 nm` shifts a ring resonance by `lambda * DNEFF_DW * dw / ng` within 1 %.
- Correlated field: two waveguides at distance 0 get identical deltas, at 10 correlation lengths independent
  deltas (statistics over 1000 seeded draws).
- Reproduction of the CROW benchmark whiskers (`bench/RESULTS.md`) with `.param L=gauss()`; a 20-instance PUF
  Monte-Carlo yields uniqueness/reliability histograms comparable to Jimenez's published values.
- Golden check unchanged when no variation directives are present.

## Effort breakdown
Geometry kwargs + presets 2 d · per-instance sampling via F-02 1 d · correlated field sampler 3 d · corners 1 d ·
tests/docs 3 d. Total about 10 days after F-02.

## Open questions for the maintainer
- Which sensitivity defaults to ship (220 nm SOI strip at 1550 nm from the Columbia model? SiN values for the
  PUF work?) and whether `neff/ng` or `W/H` should be the primary netlist interface.
- Where do instance coordinates come from for hand-written netlists (optional kwargs) versus layouts (F-13)?
- Should variation also apply to coupler coupling ratios and PCM parameters (proposed: yes, through the same
  `gauss()` mechanism, no dedicated model)?
