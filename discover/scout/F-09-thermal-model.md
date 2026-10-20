# F-09: Thermal model (global temperature, thermo-optic tuning, heaters, crosstalk, self-heating)

value 4 | effort S (static) / M (dynamic) | risk med (needs-human) | prerequisites: F-08 for dynamics |
horizon 2 (static) / 3 (dynamic)

## Problem / user story
*As a ring/MZI designer I need resonances to move with temperature and heater power, with realistic time
constants and crosstalk.* GitHub issue #3 ("global temperature dependence"); the reservoir thesis centres its
perspectives on wavelength drift and a compensation loop; the MZI-mesh thesis lists thermal crosstalk among
un-modelled threats; Cardoso ignored thermal crosstalk "but SPECS is capable of representing it" (SOURCES B).
Today the waveguide phase law has no temperature term and a hard-coded 1.55 um reference
(`src/devices/waveguide.cpp:25, 28, 60, 84`), the phase shifter is an instantaneous `exp(j sens V)` with no
power, no `tau` (`src/devices/phaseshifter.cpp:41, 59`), there is no global temperature option
(`src/parser/parse_directive.cpp:18-48`) and no coupling between heaters.

## Prior art
dn/dT(Si) = 1.86e-4 /K (PhotonForge) to 1.9e-4 (Columbia model), SiO2 1e-5; ring shift ~70-80 pm/K
(~10 GHz/K); heater time constants 0.6-14 us (Sandia, OSA Continuum), sub-100 ns pulsed micro-heaters
(Atabaki), ring body ~275 ns (PhotonForge); crosstalk as a linear coupling matrix (INTERCONNECT thermal
crosstalk note; Milanizadeh "thermal eigenmode decomposition"); self-heating bistability (Carmon 2004,
Johnson 2006); caveat that relaxation is often bi-exponential or logarithmic (Gray 2020, Borghi 2021,
SMACD 2025). (SOURCES D §3.)

## Proposed design
1. **Static temperature (golden-neutral at defaults).** `.options temp=300` and `--temp`; waveguide kwargs
   `DNDT` (default 1.86e-4), `TREF` (300 K), `GAMMA` (confinement, default 1), optional per-element `TEMP=`;
   phase law becomes `neff(lambda, T) = neff(lambda) + GAMMA * DNDT * (T - TREF)`; `LAMBDA0` becomes a parameter
   while touching these lines. `MZI`, `CROW`, `Clements`, `Octane*` inherit through their waveguides.
2. **Heaters.** `PSHIFT` kwargs `PPI` (W per pi) or `ETA` (rad/W), `R` (ohm, so a voltage net gives
   `P = V^2 / R`), `TAU` (s); with `TAU = 0` the model stays instantaneous (exact for steps long against `tau`);
   with `TAU > 0` the phase follows a first-order QSS state (F-08), scheduling its own output events.
3. **Crosstalk.** `.thermal` directive listing heater pairs with coupling coefficients (rad/W or K/W), or a
   `COUPLING=[...]` list on each heater; applied as a static matrix at elaboration and through the same
   time constants dynamically.
4. **Self-heating.** `SELFHEAT=1` on waveguides/rings: absorbed power `alpha P L` feeds the same state with
   a thermal resistance `RTH` (K/W); reproduces thermal bistability in a ring under a power sweep.

## Touch points
`src/devices/waveguide.{h,cpp}` (`waveguide.h:11-53` parameters; `waveguide.cpp:6-30, 51-100` phase law,
three copies in Uni/Bi) · `src/devices/phaseshifter.{h,cpp}` · `src/parser/parse_element.cpp` (WG and PSHIFT
kwargs) · `src/parser/parse_directive.cpp:18-48` (`temp`), new `.thermal` rule in `src/parser/parser.y` ·
`src/specs.h:95-117` (global temperature) · `src/devices/mzi_active.cpp`, `octane_*` (pass-through).

## Validation plan
- Static: a 1550 nm silicon ring shifts by `lambda * GAMMA * DNDT * dT / ng` (78 pm/K for ng 4.3) within 1 %
  in a `.dc` sweep at `temp=310` vs 300 (oracle test, F-06).
- Heater step: phase approaches `ETA * P` exponentially with `TAU` (QSS unit test); crosstalk: two heaters,
  measured phase matrix equals the coupling matrix.
- Self-heating power sweep up and down shows the Carmon hysteresis width within 10 %.
- Golden check unchanged at defaults (`temp=300`, `TAU=0`, no coupling).

## Effort breakdown
Static temperature 2 d · heater power/tau on QSS 3 d · crosstalk directive 2 d · self-heating 2 d ·
tests/docs 3 d.

## Open questions for the maintainer
- Material defaults: one `DNDT` per waveguide, or a `MATERIAL=si|sin|lnoi` keyword with a table (the group
  works on Si, SiN rings and LNOI)?
- Apply temperature to `ng` as well (`dng/dT` is small; proposed: ignore, document)?
- Single exponential versus bi-exponential relaxation option (literature says single is often wrong)?
