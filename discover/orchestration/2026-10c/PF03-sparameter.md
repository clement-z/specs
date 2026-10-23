# Phase F03 — S-parameter device `SPAR` (branch `ws/spar`)

Follow `COMMON.md` and the gate of `PD-common-f15.md`. Phase id `PF03`. Brief: `discover/scout/F-03-sparameter-device.md`
("Proposed design" items 1–3 and 5, "Validation plan"). Decisions: "Decisions of 2026-10-23, third round", row
"S-parameter device", in `backlog/DECISIONS-2026-10-21.md`. Phase 2 of the brief (broadband time-domain
realisation) is **out of scope** (needs-human).

## Scope
1. Loader (`src/io/sparam_file.{h,cpp}` or as the brief proposes): Lumerical/SiEPIC `.sparam`, Touchstone 1.x/2.x,
   the brief's CSV form, and a **Taylor-coefficient form** (per port pair: amplitude and phase as polynomials in
   the wavelength offset from a centre wavelength, plus a delay; look at the removed `TransmissionMatrix`, cc1e52e,
   for what it offered, but design the input format cleanly and document it).
2. **Phase convention:** convert every file's phases to SPECS's convention (waveguides apply `exp(+j*beta*L)`).
   Decide the conversion per format from its documented time convention, and **prove it with a test**: a plain
   waveguide section written as S-parameters in each format (generated analytically by a script in the repository)
   must give, through `SPAR`, the same transmission phase and delay as a SPECS `WG` of the same length.
3. Device `SParamDevice`, written from scratch as the brief says: state-holding (one memory per input port and
   wavelength), full recomputation per input event at that wavelength, absolute writes, per-wavelength `S_ij` cache,
   event delay = rounded group delay (same rounding and delay checks as waveguides, B-014). Uni and bidirectional
   like the other devices. **Wavelength outside the data's range: error** (with the range in the message).
4. Netlist element `SPAR` with a variable number of nets, keywords as the brief proposes; parser, docs
   (`doc/netlist.md`), KiCad symbol if simple.
5. Export: write a `.dc` sweep as `.sparam`/Touchstone (for the round-trip test and reuse).
6. Tests: loader unit tests per format; the phase-convention test above; the round trip of `circuits/add_drop.cir`
   (export, load, sweep again); passivity check; golden cases. Fixtures are generated in the repository by scripts
   (analytic delay line, exported SPECS sweeps). Third-party data files only if their licence allows it and you can
   fetch them; otherwise skip and say so.
7. Docs: `doc/netlist.md` (SPAR, formats, phase convention, what `.tran` does: per-wavelength S with a group delay,
   no dispersion within a pulse), CHANGELOG `[unreleased]`, the F-03 brief's status.

Existing outputs must stay bit-identical (new cases only); counters identical.
