# Sources consulted by the feature scout (2026-10-20)

Every external source consulted, one line each with the takeaway that mattered for `FEATURES.md`.
Collected by four research strands (A: comparable tools and formats; B: user needs in photonic computing;
C: engineering, packaging and output formats; D: physics models) plus the maintainer's own documents.
Items marked *[search summary]* were read through a search-result excerpt only (vendor site gated).
Internal evidence (code, paper, discovery reports, bench runs) is cited inline in the feature files, not here.

## A. Comparable tools, S-parameter formats, netlist and schematic interop

### Ansys Lumerical INTERCONNECT
- https://optics.ansys.com/hc/en-us/articles/360036107914 — "Optical N Port S-Parameter" element: `.dat` header options 1-3, `"modulation"` type, FIR/IIR fit knobs, passivity/reciprocity enforcement, Touchstone import since 2021R1.1 with `! Optical Port List` / `! Port[i] = {...}` comments.
- https://optics.ansys.com/hc/en-us/articles/360036618513 — S-parameter file formats: phase in radians, optional group-delay column, sweep `.idat`, `importsparameter` / `touchstoneload` script commands.
- https://optics.ansys.com/hc/en-us/articles/360034399174 — Transient simulator: data-flow scheduler, Sample mode (digital-filter fits of S-params) vs Block mode (FFT), per-mode and per-band filters.
- https://optics.ansys.com/hc/en-us/articles/360055833233 — Statistical simulation: Monte Carlo and Corner utilities fed by CML Compiler `.lib` (Gaussian / Pearson IV, local and global variation).
- https://optics.ansys.com/hc/en-us/articles/360034930553 — `exportsweep` writes Lumerical or Touchstone v1.1; one Touchstone port per (port, mode); passivity > 1.03 and reciprocity > 0.03 warnings.
- https://optics.ansys.com/hc/en-us/articles/360037304774 — INTERCONNECT reference: parameter sweeps, optimisation, Monte Carlo (incl. spatially correlated), corners, `importnetlist` / `exportnetlist`.
- https://optics.ansys.com/hc/en-us/articles/12147882417043 — *[search summary]* FIR tap guidance: ~64 taps for dispersive devices, 1 tap where latency matters.
- https://innovationspace.ansys.com/forum/?p=429428 — *[search summary]* FIR vs IIR trade-offs for time-domain S-parameter fits.
- https://optics.ansys.com/hc/en-us/articles/360034416594-INTERCONNECT-application-example — *[search summary]* Monte Carlo scripted through the Python `lumapi`.
- https://www.ansys.com/products/photonics/cml-compiler — *[search summary]* CML Compiler builds INTERCONNECT and Verilog-A compact models from measured or simulated data.
- https://optics.ansys.com/hc/en-us/articles/49698331688467-Photonic-Verilog-A-co-design-with-foundry-PDKs-OptoCompiler-models — *[search summary]* Verilog-A CMLs for OptoCompiler/HSPICE with the GlobalFoundries "Foundry A" port convention.

### Luceda IPKISS / Caphe
- https://academy.lucedaphotonics.com/guides/caphe/ — Caphe: frequency and time domain, bidirectional multimodal ports, Python models compiled to machine code.
- https://academy.lucedaphotonics.com/tutorials/circuitmodels/ — `CompactModel` API: `calculate_smatrix`, `calculate_signals` (with past-time access for delays), `calculate_dydt` for ODE states; `get_smatrix`, `get_time_response(dt, integration_method)`.
- https://academy.lucedaphotonics.com/tutorials/circuitmodels/smatrix_compactmodel — Touchstone `.sNp` import, `BSplineSModel.from_smatrix`, CST/Lumerical phase-convention importers.
- https://docs.lucedaphotonics.com/reference/io/ref/ipkiss3.io.touchstone.import_touchstone_smatrix.html — *[search summary]* `import_touchstone_smatrix(term_mode_map)` reads Lumerical header metadata; `to_touchstone(MA|RI)`.
- https://academy.lucedaphotonics.com/learn/circuit_analyzer — Circuit Analyzer: corner analysis, four Monte Carlo variants (global, local, CMV w/t/h), wafer-map sweeps, light-path tracer.
- https://academy.lucedaphotonics.com/history/relnotes2025030 — *[search summary]* corner / Monte Carlo launched from IPKISS Canvas schematics (2025.03).

### Photontorch, SAX, Simphony, gdsfactory, circulax
- https://github.com/flaport/photontorch — AGPL-3.0, circuits as recurrent networks on PyTorch, CUDA; dormant since 2022 (v0.4.1); the tool the SPECS paper benchmarked against.
- https://raw.githubusercontent.com/flaport/photontorch/master/photontorch/environment/environment.py — `Environment(dt=1e-14, wl, freqdomain, grad)` defaults.
- https://raw.githubusercontent.com/flaport/photontorch/master/photontorch/networks/network.py — delays rounded to integer time steps (warns on zero-delay), memory-less nodes eliminated through `(1 - C S)^-1 C`, per-wavelength S batched every step.
- https://raw.githubusercontent.com/flaport/sax/main/README.md — SAX 0.18.2: a model is a function returning an S-dict; netlist dict; KLU backend by default.
- https://raw.githubusercontent.com/flaport/sax/main/src/sax/__init__.py — exported API: Touchstone and Lumerical parsers/writers, backends (klu/fg/additive/forward), multimode helpers, xarray interpolation.
- https://raw.githubusercontent.com/gdsfactory/sax/main/src/sax/parsers/touchstone.py — Touchstone via scikit-rf; `! ports:` comment; default `o1..oN`; `port@mode`.
- https://raw.githubusercontent.com/gdsfactory/sax/main/src/sax/parsers/lumerical.py — `.dat/.sparam` grammar adapted from Simphony; group delay re-applied as `2*pi*tau_g*(f - f_c)`.
- https://raw.githubusercontent.com/gdsfactory/sax/main/src/sax/models/straight.py — canonical model signature `straight(*, wl, wl0, neff=2.34, ng=3.4, length=10.0 [um], loss_dB_cm=0.0) -> SDict`.
- https://github.com/gdsfactory/circulax — 2026: JAX/Diffrax mixed electronic-photonic DC/AC/transient/harmonic-balance simulator with a `sax_component` wrapper (new competitor for mixed-signal use).
- https://simphonyphotonics.readthedocs.io/en/stable/tutorials/intro.html — Simphony 0.7 is built on SAX; `ClassicalSim.add_laser/add_detector/run`; SiEPIC library usage.
- https://simphonyphotonics.readthedocs.io/en/latest/_modules/simphony/plugins/lumerical.html — Lark grammar for `.sparam/.dat`; `load_sparams` / `save_sparams`; `df_to_sdict`.
- https://github.com/BYUCamachoLab/simphony — v0.7.3 MIT; classical/quantum/libraries (ideal, siepic, sipann); no time-domain module.
- https://raw.githubusercontent.com/BYUCamachoLab/simphony/master/simphony/libraries/siepic/source_data/bdc_TE_source/EBeam_1550_TE_BDC.sparam — verbatim SiEPIC `.sparam` header: `('port 1','TE',1,'port 1',1,'transmission')`, `(51,3)`, rows `f |S| phase`.
- https://raw.githubusercontent.com/SiEPIC/SiEPIC-Tools/main/Examples/CustomComponentTutorial/ebeam_taper_475_500_te1550_sparams.txt — SiEPIC-Tools custom-component S-param file, same layout.
- https://github.com/SiEPIC/SiEPIC_EBeam_PDK — the EBeam compact-model library ships as compiled Lumerical `.ice` elements plus FDTD sources.
- https://gdsfactory.github.io/gdsfactory/notebooks/10_yaml_component/ — YAML netlist syntax: instances / placements / connections / routes / ports with `"instance,port"` references.
- https://raw.githubusercontent.com/gdsfactory/gdsfactory/main/gdsfactory/schematic.py — pydantic `Netlist / Instance / Placement / Bundle / Net` field definitions (the import target for F-13).
- https://gdsfactory.github.io/gplugins/notebooks/11_get_netlist/ — `get_netlist(recursive=True)` keys and `write_netlist` YAML.
- https://gdsfactory.github.io/gplugins/notebooks/sax_01_sax/ — gdsfactory to SAX flow, `model_from_csv`, wafer-map Monte Carlo, `jax.grad` optimisation.
- https://github.com/gdsfactory/gplugins — gplugins 2.1.5 plugin list (sax, vlsir, tidy3d, meep, Lumerical FDTD, femwell, palace); Simphony no longer listed.
- https://gdsfactory.github.io/gplugins/ — netlist extractors (YAML / SPICE) and vlsir export to Spectre / ngspice / Xyce.

### Flexcompute PhotonForge
- https://docs.flexcompute.com/projects/photonforge/en/latest/getting_started.html — PhotonForge 2.0 Community tier free; pip PDKs (SiEPIC OpenEBL, Luxtelligence); cached cloud FDTD models.
- https://docs.flexcompute.com/projects/photonforge/en/latest/examples/Time_Domain_MZI.html — `SMatrixTimeStepper`: pole-residue fit of S over a band with an RMS tolerance; `TimeSeries` inputs per `port@mode`.
- https://docs.simulation.cloud/projects/photonforge/en/latest/guides/S_Parameters.html — `SMatrix` keyed `("P0@0","P1@0")` in (input, output) order.
- https://docs.simulation.cloud/projects/photonforge/en/latest/guides/Verilog_A_Export.html — Verilog-A export with `_re/_im` terminals per optical port, `absdelay`, `white_noise`; validated with OpenVAF-Reloaded + ngspice-46.
- https://docs.simulation.cloud/projects/photonforge/en/latest/_autosummary/photonforge.load_snp.html — `load_snp(filename) -> (S, f)` Touchstone loader.
- https://docs.simulation.cloud/projects/photonforge/en/latest/monte_carlo.html — `RandomVariable(name, parent, value, stdev)` and `s_matrix(component, frequencies, *variables)` yield flow.

### Synopsys, VPIphotonics, Cadence, Keysight, Optiwave, Photon Design
- https://www.synopsys.com/content/dam/synopsys/photonic-solutions/pdf/photonic-solutions-overview-datasheet.pdf — OptoCompiler/OptSim: bidirectional multi-wavelength optical + electrical signals, Monte Carlo and corners, HSPICE/PrimeSim co-simulation.
- https://www.synopsys.com/photonic-solutions/optsim.html — OptSim: embedded SPICE engine, HSPICE interface, Monte Carlo, laser fitting toolkit.
- https://www.vpiphotonics.com/Tools/PhotonicCircuits — cascaded S-matrix engine for passives + time domain for actives; sweeps, yield, optimisation; Python/MATLAB/C++ co-simulation.
- https://vpiphotonics.com/Tools/PhotonicCircuits/Features — automatic FIR design for passives, TLM lasers/SOAs, TE/TM, S-matrix load/save incl. measured devices, Keysight ADS link.
- https://community.cadence.com/cadence_blogs_8/b/cic/posts/unifying-electronic-and-photonic-circuit-simulation — Spectre Photonics Option (17 Mar 2026): EIC + PIC in one Spectre X netlist, Verilog-A and optical S-parameter blocks.
- https://www.cadence.com/ko_KR/home/tools/custom-ic-analog-rf-design/circuit-simulation/spectre-photonics-option.html — *[search summary]* single-netlist product page; library includes optical S-parameter blocks.
- https://www.keysight.com/us/en/product/W3803B/photonic-designer.html — Keysight Photonic Designer in ADS: S-param, AC, DC, transient (convolution), harmonic balance, circuit envelope; template SiPh PDK.
- https://www.businesswire.com/news/home/20200511005655/en/ — Keysight-VPI "VPI Optical Link" for PathWave ADS (2020).
- https://optiwave.com/optispice-overview/ — OptiSPICE: HSPICE-compatible netlists, WDM and multimode, transient/AC/noise, S-parameter models via pole-residue extraction (the closest commercial "SPICE for photonics").
- https://photond.com/picwave/features — PICWave: time-domain travelling-wave, multimode, S-matrix import from FIMMPROP, chi(2)/chi(3) and TPA, Python/MATLAB scripting.

### Standards, Verilog-A photonics, KiCad, other open source
- https://www.ibis.org/touchstone_ver2.1 — Touchstone 2.1 ratified 2024-01-26; identical to 2.0 apart from the version string.
- https://www.ibis.org/touchstone_ver2.1/touchstone_ver2_1.pdf — option line `# <unit> S <DB|MA|RI> R <n>`, `[Version]`, `[Number of Ports]`, `[Two-Port Data Order]`, `[Matrix Format]`, `[Network Data]`; no standard optical port/mode naming.
- https://scikit-rf.readthedocs.io/en/latest/_modules/skrf/io/touchstone.html — reference loader: v1.0/1.1/2.x keywords, RI/MA/DB, HFSS `! Port[n] = name` comments.
- https://openepda.org/ — openEPDA standards (uPDK, chip/measurement description files; netlist format "under development"); no S-parameter or compact-model format.
- https://pypi.org/project/openepda/ — `openepda` 0.1.20 (2022-04-13), TU/e + PITC.
- https://www.mos-ak.org/london_2014/presentations/02_Patrick_Martin_MOS-AK_London_2014.pdf — CEA-Leti Verilog-A photonics: 9-line optical bus (lambda + Re/Im x polarisation x direction), Eldo/Virtuoso, 1000-run Monte Carlo.
- https://api.semanticscholar.org/graph/v1/paper/DOI:10.1109/JLT.2022.3206818 — Ye, Ullrick, Bogaerts, Dhaene, Spina, JLT 40(24) 2022: complex vector fitting of S-parameters into SPICE equivalent circuits (baseband, bidirectional, dispersive).
- https://www.imec-publications.be/entities/publication/bf60e625-2840-484d-bb8c-46cc69338261/full — same paper, imec record.
- https://arxiv.org/abs/2402.10971 — Azhigulov, Lu, Pond, Chrostowski, Shekhar 2024: bidirectional power-wave Verilog-A on a single `inout` port, S-parameter lookup tables (no code release).
- https://github.com/dwarning/VA-Models/blob/main/README.md — *[search summary]* OpenVAF compiles Verilog-A to OSDI for ngspice >= 39.
- https://docs.kicad.org/9.0/en/eeschema/eeschema.html — KiCad 9: ngspice simulator, `Sim.Device/Sim.Type/Sim.Params/Sim.Pins/Sim.Library` fields, OP/DC/AC/TRAN/PZ/NOISE/SP/FFT; **custom netlist generators** run an arbitrary command on the intermediate XML (`%I %O %B %P`).
- https://mintlify.wiki/KiCad/kicad-source-mirror/advanced/spice-simulation — unofficial source-mirror notes on `Sim.Pins`, the netlist exporter class and external ngspice.
- https://forum.kicad.info/t/sim-params-doesnt-add-model-parameters/61158 — KiCad 9 regression: `Sim.Params` not applied to custom `.model` cards.
- https://github.com/siepic/opics — OPICS (MIT, last push 2023): `Network / add_component / connect / simulate_network`, frequency domain only.
- https://raw.githubusercontent.com/SiEPIC/opics/master/opics/libraries/catalogue.yaml — OPICS component libraries downloaded as zips (`ebeam` 0.3.34).
- https://github.com/mpasson/lekkersim — lekkersim (AGPL, active 2026): hierarchical S-matrix solver with power-flow monitors.
- https://github.com/joamatab/awesome_photonics — curated list of S-parameter linear solvers (SAX, lekkersim, simphony, photontorch, opics, scikit-rf).
- https://piel.readthedocs.io/en/stable/sections/microservices/integration/index.html — piel: cocotb + sax + gdsfactory + PySpice/ngspice/Xyce co-design glue.
- https://github.com/ScopeX-ASU/SimPhony — DAC 2025 architecture-level EPIC modelling framework (name clash with Simphony; not a circuit simulator).

Gaps strand A could not verify: OptSim and VPI S-parameter file formats; INTERCONNECT result export formats; Keysight ADS 2026 bundling; exact MIEL 2014 citation for Martin et al.; Xyce photonic efforts (no query run); Simphony 0.7 has `sigma_*` model parameters but no Monte Carlo driver.

## D. Physics and device models for a piecewise-constant event-driven formulation

### Waveguide nonlinearity, free carriers, dispersion
- https://sites.rochester.edu/agrawal/wp-content/uploads/2019/08/paper_2007_11.pdf — Lin, Painter, Agrawal, Opt. Express 15, 16604 (2007): silicon NLSE with Kerr, TPA, free-carrier terms; beta_TPA ~ 0.5 cm/GW, sigma_FCA = 1.45e-17 cm^2, carrier lifetime sub-ns to tens of ns.
- https://zenodo.org/record/1232209 — Soref & Bennett, IEEE JQE 23, 123 (1987): free-carrier dispersion/absorption coefficients for silicon.
- https://optics.ansys.com/hc/en-us/articles/7945837183763-NLSE-Waveguide-NLSE-WGD-INTERCONNECT-Element — INTERCONNECT has a block-mode (split-step) NLSE waveguide element; the ordinary waveguide is linear.
- https://academy.lucedaphotonics.com/ipkiss/samples/plot_resonator_kerr_nonlinearity — Caphe Kerr ring: coupled-mode ODE with Kerr detuning, fixed dt = 0.1 tau, bistability demo (a good golden case for a nonlinear ring).
- https://opg.optica.org/abstract.cfm?URI=josab-29-5-896 — Fiers et al., JOSA B 29, 896 (2012): the Caphe node formalism, instantaneous S-relation plus state ODEs plus access to input history.
- https://docs.flexcompute.com/projects/photonforge/en/latest/examples/Self_Pulsing.html — PhotonForge self-pulsing ring: full coupled-mode set (field, free carriers, temperature) with numeric parameters, BDF solver.
- https://optics.ansys.com/hc/en-us/articles/360036618653 — INTERCONNECT straight waveguide: loss, neff, ng, dispersion at a centre frequency; realised as FIR/IIR digital filters in sample mode.
- https://optics.ansys.com/hc/en-us/articles/360034399274-TSM-Sample-rate — transient sample rate must cover the signal band; filter accuracy depends on it.
- https://optics.ansys.com/hc/en-us/articles/360042323534-Optical-filters — FIR (feed-forward) vs IIR (rings): 256 default / 4096 max taps; ~64 taps when dispersion matters, 1 where delay is critical.
- https://pmc.ncbi.nlm.nih.gov/articles/PMC6459821/ — Photontorch paper (Sci. Rep. 9, 5918, 2019): fixed-dt S/C-matrix recurrence, memory-containing nodes, wavelengths batched on GPU.
- https://raw.githubusercontent.com/flaport/photontorch/master/photontorch/components/waveguides.py — Photontorch waveguide: delay ng L/c, linear neff(lambda), constant loss, no dispersion.
- https://arxiv.org/abs/2605.13599 — 2026 preprint: delay-based adaptive time-domain cavity simulator (round-trip recursion), validated on Virgo, not on integrated rings.

### Thermal effects
- https://authors.library.caltech.edu/records/pm95j-ryj39 — Johnson, Borselli, Painter, Opt. Express 14, 817 (2006): lumped thermal + free-carrier ring model (self-pulsing).
- https://authors.library.caltech.edu/records/4gyen-3s411 — Carmon, Yang, Vahala, Opt. Express 12, 4742 (2004): thermal self-stability, line broadening and hysteresis theory.
- https://arxiv.org/abs/2003.09978v1 — Gray et al. 2020: thermo-optic multistability; relaxation roughly logarithmic in time (2-D diffusion), not single-exponential.
- https://iris.unipv.it//retrieve/handle/11571/1482396/564032/on_the_modeling.pdf — Borghi et al. 2021: Newton cooling inaccurate for planar Si resonators; fitted corrected thermal law.
- https://ifte.de/mitarbeiter/lienig/SMACD_2025.pdf — open-source FEM study: bi-exponential thermal relaxation, ~30 % dependence on waveguide geometry.
- https://ar5iv.arxiv.org/html/1507.00686 — Jayatilleka et al. 2015: in-resonator photoconductive heaters, 0.25 nm/mW tuning efficiency.
- https://re.public.polimi.it/bitstream/11311/1121622/1/Milanizadeh_OFC19.pdf — Milanizadeh et al. 2019: thermal crosstalk as a linear coupling matrix, cancelled by "thermal eigenmode decomposition".
- https://optics.ansys.com/hc/en-us/articles/47617107334291-Modeling-thermal-crosstalk-in-photonic-circuit-simulation-with-INTERCONNECT — INTERCONNECT thermal crosstalk via a coupling matrix (accurate vs simplified variants).
- https://docs.flexcompute.com/projects/photonforge/en/latest/examples/Microring_Thermal_Stabilization.html — PhotonForge heater example; ~10 GHz/K resonance sensitivity.
- https://repository.gatech.edu/entities/publication/e142638f-be51-46a2-8189-c7f32a97ff0f — Atabaki et al. 2013: sub-100 ns pulsed micro-heater reconfiguration.
- https://sandia.gov/research/publications/details/wide-range-and-fast-thermally-tunable-silicon-photonic-microring-resonators-2016-10-03 — Sandia dual-ring heaters: 0.6 us 10-90 % thermal response.

### Lasers, amplifiers, modulators, detectors
- https://optics.ansys.com/hc/en-us/articles/360036107574-DM-Laser-DML-INTERCONNECT-Element — DML element: 0-D rate equations with Langevin noise; RIN and linewidth are outputs; linewidth-enhancement factor default 5.
- https://optics.ansys.com/hc/en-us/articles/39727136204051-INTERCONNECT-as-a-Laser-Design-Platform — TWLM: 1-D travelling-wave laser/SOA model, bidirectional ports, L-I / RIN / linewidth outputs.
- https://sites.rochester.edu/agrawal/wp-content/uploads/2019/08/paper_1989_10.pdf — Agrawal & Olsson, IEEE JQE 25, 2297 (1989): lumped SOA gain-integral model (dh/dt form).
- https://docs.flexcompute.com/projects/photonforge/en/latest/guides/Directly_Modulated_Laser.html — PhotonForge DML time-stepper (rate equations in a time-stepped model).
- https://optics.ansys.com/hc/en-us/articles/360042329094-Ring-modulator-time-domain-INTERCONNECT — ring modulator time-domain workflow: modulation tables; electrical RC bandwidth from a separate CHARGE solve.
- https://optics.ansys.com/hc/en-us/articles/360036107694-Optical-Ring-Modulator — Optical Ring Modulator element: quasi-static and time-varying transfer function.
- https://optics.ansys.com/hc/en-us/articles/360037865793 — MZM statistical template: `Vpi_electrical`, electrical 3 dB bandwidth, lumped vs travelling-wave electrodes as first-class (statistical) parameters.
- https://optics.ansys.com/hc/en-us/articles/360034710353 — `photodetector_simple`: responsivity, dark current, shot/thermal noise, saturation, bandwidth as a low-pass filter.
- https://arxiv.org/pdf/2311.16174 — Verilog-A depletion microdisk/ring modulator with integrated heater (real-valued coupled ODEs, 7x transient speed-up).

### Phase-change photonic memory cells
- https://ore.exeter.ac.uk/repository/handle/10871/18574 — Rios et al., Nat. Photon. 2015: GST on Si3N4, 8 levels, 13.4 pJ switching, ~1 GHz, WDM addressing.
- https://ora.ox.ac.uk/objects/uuid:deec1ef9-6141-488e-8633-ed67644a1536 — Li et al., Optica 6, 1 (2019): 34 levels; write = one 20-50 ns pulse whose *peak power* sets the amorphous fraction; erase = fixed 50 ns + 200 ns double step; ~200 ns thermal decay; level spread < 0.5 %.
- https://pmc.ncbi.nlm.nih.gov/articles/PMC6522354 — Feldmann et al., Nature 2019: all-optical spiking network with 60 PCM synapses, WDM.
- https://ideas.repec.org/a/nat/nature/v589y2021i7840d10.1038_s41586-020-03070-1.html — Feldmann et al., Nature 2021: PCM photonic tensor core with frequency comb, > 14 GHz read.
- https://www.ncbi.nlm.nih.gov/pmc/articles/PMC6768866/ — Zhang et al., Nat. Commun. 2019 (GSST): delta n ~ 2.0 with low loss, microsecond amorphisation.
- https://eprints.soton.ac.uk/442770 — Delaney et al., Adv. Funct. Mater. 2020: Sb2S3 delta n 0.60, Sb2Se3 delta n 0.77, k < 1e-5, > 4000 cycles.
- https://pmc.ncbi.nlm.nih.gov/articles/PMC8208718 — Delaney et al., Sci. Adv. 2021 (Sb2Se3): 50 ms / 19 mW crystallise, 400 ns / 35 mW amorphise, ~200 C switching, 600 cycles.
- https://pmc.ncbi.nlm.nih.gov/articles/PMC10261021/ — Chen et al., Nat. Commun. 2023 (Sb2S3 with PIN heaters): RESET 150-550 ns, SET ~200 ms; 32 levels at 0.5 dB/step with identical partial-amorphisation pulses; ~10 us thermal relaxation; 77-day retention.
- https://lamarr.ece.uw.edu/research/amlab/Papers_Journals/Jiajiu_PCM_Heater_Modeling.pdf — Zheng et al., ACS AMI 2020: electrothermal PCM model, melt at 888 K, crystallise at 673 K window, 1e10-1e11 K/s quench, "dead time" concept, aJ/nm^3 energies.
- https://lamarr.ece.uw.edu/research/amlab/Papers_Journals/Roger_Graphene_PCM_Nat_Nano.pdf — Fang et al., Nat. Nanotech. 2022: graphene micro-heaters, 8.7 aJ/nm^3, > 1000 cycles.
- https://arxiv.org/pdf/2605.07715 — Lorentz-Lorenz effective-medium mixing with a crystallisation fraction for partial GST states (the standard T(x), phi(x) mapping).
- https://digitalcommons.njit.edu/fac_pubs/8244 — electrical PCM Verilog-A compact model with a crystalline-fraction state (JMAK kinetics): template for a photonic one, none found in the literature.
- https://cpl.iphy.ac.cn/article/doi/10.1088/0256-307X/28/1/018501 — another electrical PCM compact model (crystalline fraction + 1-D heat).

### Polarisation / multimode
- https://optics.ansys.com/hc/en-us/articles/360034408514-setmodes-Script-command — INTERCONNECT orthogonal identifiers 1/2 (TE/TM) via `setmodes`.
- https://innovationspace.ansys.com/forum/forums/topic/simulating-more-than-two-orthogonal-modes-in-interconnect/ — more than two modes is awkward; active elements carry a single data set for all modes.
- https://nbviewer.org/github/flaport/sax/blob/0.12.0/examples/04_multimode_simulations.ipynb — SAX multimode: `"{port}@{mode}"` keys, single-mode models auto-lifted without cross-polarisation terms.

### Mixed-mode precedents and quantized-state integration
- https://en.wikipedia.org/wiki/SystemC_AMS — SystemC-AMS (IEEE 1666.1-2016): TDF / LSF / ELN models of computation, converter ports to the DE kernel.
- https://www.eda.org/images/resources/videos/Accellera-SystemC-AMS-Tutorial-2014.pdf — TDF cluster = one SC_THREAD with a static schedule; converter-port semantics.
- https://vhdl.org/images/resources/articles/amsdynamictdf/Whitepaper_SystemC_AMS_Dynamic_TDF_September_2011.pdf — Dynamic TDF: `change_attributes()` / `request_next_activation(time | event)` lets a sampled block re-time itself from events.
- https://designers-guide.org/verilog-ams/VlogAMS-2.4.0.pdf — Verilog-AMS LRM: connect modules at the analog/digital boundary (a solver-based precedent).
- https://accellera.org/downloads/standards/v-ams — current Verilog-AMS standard page.
- https://ep.liu.se/ecp/047/012/ecp4710012.pdf — Floros, Cellier, Kofman (Modelica 2011): QSS1-3 definitions with hysteretic quantum; QSS3 beats DASSL on sparse systems; LIQSS removes steady-state chatter.
- https://en.wikipedia.org/wiki/Quantized_state_systems_method — QSS1 definition; global error of stable LTI systems bounded proportionally to the quantum (Kofman).
- https://icc.fcen.uba.ar/en/discrete-event-simulation-of-continuous-time-systems-evolution-and-state-of-the-art-of-quantized-state-system-methods/ — Castro, Bergonzi, Pecker Marcosig, Fernandez, Kofman 2024 review of QSS methods (SIMULATION, DOI 10.1177/00375497241230985).
- https://build.openmodelica.org/Documentation/ModelicaDEVS.UsersGuide.QSS.html — OpenModelica / ModelicaDEVS QSS user guide (PowerDEVS bridge).
- https://www.sld.cs.columbia.edu/pubs/chan_date10.pdf — PhoenixSim (DATE 2010): OMNeT++ discrete-event, message-level physical-layer metrics; fields are not simulated.

### Fabrication variability and Monte Carlo
- https://picmagazine.net/article/101317/Simulations_evaluate_variations_in_PIC_manufacturing — Lu et al. 2017 (Opt. Express 25, 9712): 2074 racetracks on a 200 mm wafer, extracted width/thickness maps, layout-dependent correlated Monte Carlo ("virtual wafer").
- https://ar5iv.labs.arxiv.org/html/2301.01689 — Columbia process-variation-aware strip-waveguide compact model: rational in width, Taylor in lambda, one PCA variable, delivered as Verilog-A for Virtuoso Monte Carlo.
- https://arxiv.org/pdf/2205.11481 — dneff/dw ~ 3e-3 per nm at 400 nm width on 220 nm SOI; wider guides reduce it.
- https://www.imec-publications.be/entities/publication/ceb5b999-785c-4f7e-aa4c-dc3f4120603b/full — Xing, Dong, Khan, Bogaerts, ACS Photonics 10, 928 (2023): wafer maps of line-width/thickness, hierarchical wafer/die/local variation model.
- https://optics.ansys.com/hc/en-us/articles/360043451834 — INTERCONNECT Monte Carlo + corner analysis example on statistical compact models.
- https://www.ansys.com/resource-center/case-study/aim-photonics-analog-photonic-deliver — AIM Photonics PDK: 25+ statistical compact models (width/height/doping).
- https://simphonyphotonics.readthedocs.io/en/latest/tutorials/layout_aware.html — Simphony layout-aware Monte Carlo: sigma_w, sigma_h, correlation length from component coordinates.

### Frequency-response extraction and analytic oracles
- https://imec-publications.be/handle/20.500.12860/20376 — Bogaerts et al., Laser Photon. Rev. 6, 47 (2012): all-pass and add-drop closed forms (r, a, phi), critical coupling.
- https://resolver.caltech.edu/CaltechAUTHORS:YARel00 — Yariv, Electron. Lett. 36, 321 (2000): universal coupling relations.
- https://authors.library.caltech.edu/records/5y6h2-20105 — Yariv, IEEE PTL 14, 483 (2002): critical coupling in add-drop rings.
- https://scholars.cityu.edu.hk/en/publications/microring-resonator-channel-dropping-filters/ — Little et al., JLT 15, 998 (1997): coupled-mode theory in time, N-ring continued-fraction response.
- https://authors.library.caltech.edu/records/d1g33-5y822 — Poon et al., Opt. Express 12, 90 (2004): 2x2 transfer-matrix cascade for CROWs, dispersion relation, group delay.
- https://optics.ansys.com/hc/en-us/articles/360036617973 — INTERCONNECT ONA: "scattering data" vs "impulse response" analysis types, compared to validate the transient sample rate.
- https://innovationspace.ansys.com/courses/courses/lumerical-interconnect-time-domain-simulations/lessons/tips-for-transient-sample-mode-simulations-in-ansys-lumerical-interconnect-lesson-4/ — sample-rate check example (10 THz, 1e4 points).

Cited from memory by strand D, not fetched (verify before quoting): Padmaraju & Bergman, Nanophotonics 3, 269 (2014); Wuttig, Bhaskaran, Taubner, Nat. Photon. 11, 465 (2017); Sacher & Poon, Opt. Express 16, 15741 (2008); Kolner, IEEE JQE 30, 1951 (1994); Henry 1982 linewidth; Coldren & Corzine rate equations; Agrawal, Nonlinear Fiber Optics; beta_2 ~ 1-4 ps^2/m and n_2 ~ 4.5e-18 m^2/W for SOI strips.

## C. Engineering: Python bindings, packaging, WebAssembly, parallel kernels, output formats, hygiene

### Python bindings and in-process APIs
- https://github.com/accellera-official/PySysC — PySysC (cppyy/Cling): Python SC_THREADs since 0.2, no SC_METHOD; last push 2023-12-26.
- https://www.minres.com/work/pysysc — *[search summary]* PySysC adopted by Accellera's SystemC Common Practices subgroup (page now 404).
- https://git.minres.com/SystemC/PySysC — Minres repository archived (read-only) on 2025-10-31.
- https://pypi.org/project/pysystemc/ — `pysystemc` 0.11.0 (2026-10-19): cppyy, Python >= 3.14, manylinux_2_28 wheel **bundling libsystemc.so** (SystemC 2.3.4, pthreads); README warns that Python callbacks per event are costly.
- https://github.com/amal-khailtash/pysystemc — source of the above.
- https://pypi.org/project/systemc/ — dau-dev `systemc` 3.0.2.1 (2026-05): `py3-none` wheels that contain headers and a CLI only; not a usable precedent.
- https://nanobind.readthedocs.io/en/latest/meson.html — nanobind via Meson WrapDB, `python.extension_module(limited_api: '3.12')`, Meson >= 1.3.
- https://nanobind.readthedocs.io/en/latest/why.html — nanobind vs pybind11: 24 vs 56 bytes per instance, no LTO needed, stable ABI from 3.12, free-threaded support, narrower C++ subset.
- https://pybind11.readthedocs.io/en/latest/compiling.html — pybind11 with Meson (`dependency('pybind11')`) and meson-python; sdists need a git checkout.
- https://pybind11.readthedocs.io/en/latest/advanced/cast/functional.html — `std::function` callbacks: per-call wrapper and conversion cost; only stateless lambdas are unwrapped for free.
- https://hackmd.io/@pycontw/r15eC2OKxx — ~150 ns per Python-to-C++ call for pybind11 (slide notes, no methodology).
- https://mesonbuild.com/meson-python/ — meson-python 0.22.1 backend overview (the natural wheel backend for the Meson tree).
- https://mesonbuild.com/meson-python/how-to-guides/shared-libraries.html — project-internal shared libs go to `.mesonpy.libs` with RPATH rewrite; external libs still need auditwheel/delocate/delvewheel.
- https://mesonbuild.com/meson-python/reference/limitations.html — only stated limitation: `datadir` files are not locatable at runtime.
- https://docs.python.org/3/library/multiprocessing.html — Python 3.14 default start method on POSIX is `forkserver`; `fork` warns in multithreaded processes.
- https://man7.org/linux/man-pages/man3/dlmopen.3.html — `LM_ID_NEWLM` namespaces: max 16, GNU-only, `RTLD_GLOBAL` interactions crash; not a production isolation path.
- https://forums.accellera.org/topic/6665-issue-while-calling-sc_main-multiple-times-from-main/ — the reference kernel is not re-entrant (IEEE 1666 section 4.3.4.2); swapping `sc_simcontext` is an unsupported hack.
- https://forums.accellera.org/topic/1651-running-sc_run-two-times/ — same: one elaboration per process, or one elaboration with repeated `sc_start()`.

### Packaging: SystemC availability, wheels, conda
- https://anaconda.org/conda-forge/libsystemc — conda-forge `libsystemc` 3.0.2 (linux_64, osx_64, osx_arm64), CMake shared build at SystemC's default **C++17**.
- https://github.com/conda-forge/systemc-feedstock — the feedstock (rattler recipe, last update 2026-09-02, skipped on Windows).
- https://github.com/microsoft/vcpkg/blob/master/ports/systemc/vcpkg.json — vcpkg `systemc` 3.0.2#1.
- https://conan.io/center/systemc — Conan Center `systemc/3.0.1` (2025-04-02), Linux/Windows/macOS.
- https://formulae.brew.sh/formula/systemc — Homebrew `systemc` 3.0.2 bottles for macOS and Linux.
- https://tracker.debian.org/pkg/systemc — Debian: 3.0.2-1 testing/unstable, 3.0.1-1 stable, 2.3.4-2 oldstable.
- https://pkgs.alpinelinux.org/package/v3.24/community/x86/systemc — Alpine 3.24: SystemC 2.3.4 (relevant to the old GitLab CI image).
- https://packages.spack.io/package.html?name=systemc — Spack: only 2.3.3, `cxxstd` variant.
- https://cibuildwheel.pypa.io/en/stable/options/ — cibuildwheel 4.3.1: backend-agnostic, manylinux_2_28 default, `before-all` to build SystemC once per container, auditwheel/delocate/delvewheel repair.
- https://learn.scientific-python.org/development/guides/gha-wheels/ — sdist + wheel matrix + trusted publishing workflow for compiled projects.
- https://raw.githubusercontent.com/accellera-official/systemc/main/INSTALL.md — `ENABLE_PTHREADS`, `CMAKE_CXX_STANDARD` default 17, IEEE 1666-2023 mandates C++17 baseline.
- https://github.com/accellera-official/systemc — `ENABLE_STD_THREADS` coroutine backend exists in 3.x; QuickThreads `md/` ports for x86_64, aarch64, riscv64, ppc, sparc, mips (no wasm32).
- https://raw.githubusercontent.com/accellera-official/systemc/main/RELEASENOTES.md — 3.0.2 is the latest release; cheaper `sc_time` operations in 3.0.

### WebAssembly and browser demos
- https://www.linkedin.com/pulse/how-i-tried-compile-systemc-webassembly-run-browser-petre — the only known SystemC-in-wasm attempt: SystemC 2.3.3 with pthreads under Emscripten ran in Node and the browser, slowly; file-I/O examples failed.
- https://emscripten.org/docs/api_reference/fiber.h.html — Emscripten Fiber API (ucontext-like) requires Asyncify.
- https://emscripten.org/docs/porting/asyncify.html — Asyncify: roughly 50 % slowdown and larger code; JSPI removes the size cost.
- https://emscripten.org/docs/porting/pthreads.html — wasm threads need SharedArrayBuffer (COOP/COEP headers), `PROXY_TO_PTHREAD`, no `fork`.
- https://v8.dev/blog/jspi-ot — JSPI origin trial from Chrome M123, Emscripten >= 3.1.47.
- https://blog.openreplay.com/jspi-javascript-wasm-bridge/ — *[secondary]* JSPI shipping in Chrome 137+, Safari 27 beta, Firefox 153 planned.
- https://github.com/jamestiotio/EEcircuit — ngspice compiled to wasm: a fully client-side browser SPICE (the model for a client-side SPECS demo).
- https://github.com/danchitnis/EEsim — same family (EEsim).
- https://pyodide.org/en/stable/usage/wasm-constraints.html — Pyodide has no threading / multiprocessing / subprocess: can host analysis only.
- https://www.zonca.dev/posts/2026-05-18-jupyterlite-no-server-needed — *[secondary]* JupyterLite ships NumPy/pandas/SciPy/matplotlib in-browser.

### Parallel SystemC and engine performance
- https://www.cecs.uci.edu/~doemer/publications/Talk_SystemCEvolution_20220407.pdf — Doemer 2022: seven obstacles to parallel SystemC in IEEE 1666; RISC out-of-order PDES up to 212x on a Xeon Phi with SIMD; calls for a parallel 3.x standard.
- https://past.date-conference.com/proceedings-archive/2016/pdf/0325.pdf — SCale (CEA LIST, DATE 2016): atomic parallel evaluation with repeatable execution, 34x on 64 cores, TLM-oriented.
- https://www.ice.rwth-aachen.de/research/tools-projects/closed-projects/parsc-parallel-systemc — parSC: synchronous parallelism within delta cycles (barrier per delta).
- https://past.date-conference.com/date14/files/file/date14/ubooth/2582.pdf — SCope: conservative lookahead, per-thread time, IEEE 1666-2011 compliant.
- https://hal.archives-ouvertes.fr/hal-00748266 — SystemC-SMP / TLM-DT: 1.8x on a dual core, requires model changes.
- https://hal-cea.archives-ouvertes.fr/cea-01817866 — RAVES is a many-core *hardware* architecture for SystemC simulation, not a kernel.
- https://www.cecs.uci.edu/~doemer/publications/DVCon_19.pdf — RISC integrated into Intel Simics.
- https://systemc.org/events/scef202204/ — Simics 6 runs separate SystemC subsystems in parallel, limited by kernel semantics.
- https://forums.accellera.org/topic/7689-sc_thread-parallel-execution/ — cooperative multitasking is mandated; 1666-2023 additions need C++17.
- https://forums.accellera.org/topic/5947-what-is-the-efficiency-of-sc_thread/ — *[search summary]* SC_THREAD overhead "a few percent" with QuickThreads.
- https://arxiv.org/pdf/0710.4845 — converting 3 of 17 threads to methods gave 2 % (anecdotal; SPECS's profile shows ~16 % in coroutine switching, see `bench/RESULTS.md`).
- https://hdl.telecom-paris.fr/systemc/doxygen/html/a00416_source.html — `sc_ppq` is a binary-heap priority queue (O(log n)).
- https://sigsim.acm.org/conf/mskr/Courseware/Fujimoto/Slides/FujimotoSlides-03-FutureEventList.pdf — calendar/ladder queues beat heaps only for well-behaved timestamp distributions.
- https://angelo.furfaro.dimes.unical.it/node/139 — ladder queue background.

### Waveform and tabular output formats
- https://blog.timhutt.co.uk/fst_spec/ — reverse-engineered FST spec: block compression, f64 reals, u64 ticks; 10-50x smaller than VCD.
- https://github.com/mschlaegl/pylibfst — pylibfst 0.2.1 (2025-06).
- https://pypi.org/project/pywellen/ — pywellen 0.25.7 (2026-10-13): Rust `wellen`, multi-threaded VCD/FST/GHW reader, reals as f64, BSD-3 (recommended replacement for pyDigitalWaveTools).
- https://github.com/ekiwi/wellen — wellen source.
- https://pypi.org/project/pyvcd/ — pyvcd 0.5.1 (2026-10-19): VCD writer + tokenizer; recommends pywellen for bulk reads.
- https://pypi.org/project/vcdvcd/ — vcdvcd 2.6.0 (2025-10).
- https://pypi.org/project/pyDigitalWaveTools/ — SPECS's current VCD reader: last release 1.2 on 2022-06-22.
- https://gitlab.com/surfer-project/surfer — Surfer waveform viewer: VCD/FST/GHW, analog drawing, browser build, EUPL-1.2.
- https://docs.surfer-project.org/book/ — Surfer documentation.
- https://toem.io/resources/impulse-extensions/eda-waveform-extension/ — impulse (Eclipse/VS Code) reads VCD/FST with analog rendering.
- https://github.com/Minres/SCViewer — SCViewer shows reals as analog traces.
- https://pyvcd.readthedocs.io/en/stable/vcd.gtkw.html — GTKWave `.gtkw` analog step/interpolated flags (automatable save files).
- https://man.archlinux.org/man/extra/ngspice/ngspice.1.en — ngspice CLI: `-v`, `-b`, `-r rawfile`, `-o log`; binary rawfile by default.
- https://git.bibbit.duckdns.org/epilectrik/pyNgSpice/src/commit/f2cfb9b35973c992a0d2310d46016360185e459c/src/maths/misc/randnumb.c — ngspice `set rndseed=<n>`, random seed by default.
- https://xyce.sandia.gov/files/xyce/Xyce_Reference_Guide_7.9.pdf — Xyce: `.PRINT FORMAT=STD|CSV|RAW|...`, `-r` binary rawfile, `-randseed <n>` overriding the netlist and **echoing the seed used** (pattern to copy), `.LIN` writes Touchstone.
- https://optics.ansys.com/hc/en-us/articles/360034411174-savedata — INTERCONNECT `savedata` (.ldf); `matlabsave`, `exportsweep` for other formats.

### Research-software hygiene
- https://citation-file-format.github.io/ — CITATION.cff 1.2.0 with `preferred-citation`; consumed by GitHub, Zenodo, Zotero.
- https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-citation-files — GitHub "Cite this repository" from CITATION.cff.
- https://docs.github.com/en/repositories/archiving-a-github-repository/referencing-and-citing-content — Zenodo DOI per GitHub release.
- https://lasp-developer-guide.readthedocs.io/en/latest/workflows/open_source/citing_software.html — concept DOI vs version DOI.
- https://joss.readthedocs.io/en/latest/submitting.html — JOSS: OSI licence, > 6 months public history, tests, docs, CONTRIBUTING, AI-usage disclosure.
- https://joss.readthedocs.io/en/latest/review_criteria.html — JOSS review criteria.
- https://learn.scientific-python.org/development/guides/docs/ — Sphinx + MyST (furo/pydata) vs mkdocs-material; nbsphinx for notebooks.
- https://learn.scientific-python.org/development/guides/gha-basic/ — GitHub Actions matrix, pre-commit, concurrency, "alls-green" gate.
- https://readripman.readthedocs.io/en/latest/guides/jupyter.html — nbsphinx galleries via sphinx-gallery.
- https://github.com/executablebooks/MyST-NB/issues/396 — MyST-NB has no gallery feature.
- https://www.conventionalcommits.org/en/v1.0.0/ — Conventional Commits (SPECS already uses `fix(B-012):`).
- https://semver.org/spec/v2.0.0.html — SemVer; 0.y.z means anything may change.
- https://pytest-benchmark.readthedocs.io/en/latest/ — pytest-benchmark: fixture, JSON export, compare; no history dashboard.
- https://asv.readthedocs.io/en/latest/ — asv: benchmark history dashboards (NumPy/SciPy).
- https://lincc-ppt.readthedocs.io/en/latest/practices/ci_benchmarking.html — asv on PRs, main and nightly.
- https://github.com/eWaterCycle/setup-apptainer — Apptainer in GitHub Actions.
- https://apptainer.org/docs/user/1.4/cli/apptainer_build.html — Apptainer `.def` from a Docker image.
- https://hpc.gla.ac.uk/guides/apptainer — Apptainer usage on HPC.

Could not access (strand C): Ansys article 360036618513 (403); HAL cea-01843186; minres.com/work/pysysc (404); Accellera forum topic 5947 (403).

## B. Users: INL Lyon strands, citers of SPECS, the photonic-computing community, prior event-driven simulators

### INL Lyon / OCTANE / NEUROPULS (who uses what)
- https://hal.science/INL/hal-04302200v1 — the ECOC 2023 paper (local copy in `doc/papers/`): CROW to 512 rings, 6290x / 5090x vs Photontorch / INTERCONNECT; mixed-mode and co-simulation named as future work.
- https://api.archives-ouvertes.fr/search/?q=authFullName_s:"Fabio Pavanello" AND producedDateY_i:[2021 TO 2026] — 41 HAL records 2021-2026 for the group (the list mined for tool usage).
- https://theses.fr/api/v1/document/2023ECDL0053 — Zrounba thesis (tel-04991728): PUF benchmark (5 rings, 5/25 wavelengths, 128 bits): SPECS only 8.1x / 2.2x faster than INTERCONNECT because loops make the event count "sky-rocket" and cost is linear in wavelengths; future work: components, real mixed-mode, co-simulation; Fig. V.10 counts events per component.
- https://theses.fr/api/v1/document/2024ECDL0037 — Cardoso thesis (tel-04919509): the PCM accumulation model was implemented in SPECS; measured normalised device variability 7.73 %; 1e5 random matrix multiplications per tile size run in SPECS; COMET RISC-V core wrapped as a SystemC module and connected to the photonic GEMM; perspectives: phase as a function of PCM state, endurance, volatile VO2.
- https://theses.fr/api/v1/document/2025ECDL0022 — Gomes de Queiroz thesis (tel-05326710): MZI-mesh PNN work done in **Photontorch** under ideal conditions; un-modelled threats listed: phase drift, thermal noise, thermal crosstalk, fabrication variation.
- https://theses.fr/api/v1/document/2025ECDL0026 — Jimenez thesis (tel-05447328): SiN ring PUFs; simulations in a custom temporal coupled-mode (TCMT) code + INTERCONNECT + 2-D FDTD; 20 instances x 200-bit challenges at 100/200 GHz; perspectives: noise, more levels, coherent detection, LN nonlinearity, analog test metrics.
- https://theses.fr/api/v1/document/2024ECDL0051 — Abdalla thesis (LNOI time-delay reservoir computing): **Photontorch**, dt = 1/100 of the input clock, 4th-order Butterworth photodetector + Gaussian noise (responsivity, load, dark current, TIA NEP), 5-50 virtual nodes; perspectives: wavelength drift with temperature, microcomb WDM, LNOI nonlinearities.
- https://past.date-conference.com/proceedings-archive/2023/DATA/3046.pdf — Cardoso et al. DATE 2023 PCM MAC cell: Python behavioural environment, 100 runs, 0.1 % perturbation gives 600 % error for a static PCM scheme.
- https://www.aspdac.com/aspdac2024/archive/pdf/8D-1.pdf — Cardoso et al. ASP-DAC 2024 slides: demo.zrounba.fr/specs used; 28x28 Sobel at 6 bits / 1 GHz; ~1 ns per amorphisation event; endurance and PCM randomness listed as open issues; cell taped out.
- https://opg.optica.org/jlt/abstract.cfm?uri=jlt-42-22-8024 — Cardoso et al. JLT 2024 photonic GEMM: 2x2 tiles, 95 % MNIST under conservative noise vs 43 % for the state of the art at 1 GHz (noise-limited); SPECS runs with 20 MHz and 1 GHz photodetector bandwidths.
- https://anr.fr/Project-ANR-20-CE24-0019 — OCTANE project page: Charbonnier (CEA-LETI) coordinator, ICB / LETI / INL / Concordia, goal 10x better energy per MAC.
- https://arxiv.org/abs/2305.03139 — NEUROPULS (Pavanello et al. 2023): three platforms, **gem5-based** simulation platform, PUF-secured RISC-V accelerator.
- https://zenodo.org/records/11920648 — Hejda et al., DAC 2024 invited: gem5-based accelerator modelling with PCM + III-V platforms.
- https://api.archives-ouvertes.fr/search/?q=halId_s:(hal-05606779) — Costa et al. VLSI-SoC 2025: photonic convolution accelerator noise/quantisation study with "a fast simulation tool, more than 100x faster than conventional methods" (very likely SPECS; unverified, PDF gated); Kieninger/Jimenez/Pavanello OE 2026: 200 challenge-response pairs suffice to clone a linear optically-encoded PUF; Dwivedi/Pavanello 2026 PCM review (electrically actuated GeSe multilevel phase shifters).
- https://api.archives-ouvertes.fr/search/?q=halId_s:(hal-04978314) — Marchesin/Pavanello OE 2025 braided MZI mesh with loss/imbalance/crosstalk non-idealities; Zolfaghari/Le Beux PCM ONoC evaluated in SNIPER (hal-03512251); Shahroodi/Cardoso DATE 2023/2024 (WDM parallelism ~3113x); Jimenez "disentropy of autocorrelation" PUF metric (hal-04740477); Pavanello DATE 2024 security layers.
- https://arxiv.org/abs/2406.18757v2 — Gomes de Queiroz et al., APL ML 2024: "simulated using the Photontorch Python package ... excluding noise and component imperfections".
- https://arxiv.org/abs/2207.06245v1 — Abdalla et al. 2022 TDRC: 1 Gbit/s, sampling 5-50 GHz, Photontorch, photodetector noise model.
- https://opg.optica.org/abstract.cfm?uri=FiO-2023-JTu4A.82 — Jimenez FiO 2023 microring PUF (the OPG page marks it RETRACTED; reason not stated); Zenodo copy https://zenodo.org/records/8436663.
- https://hal.archives-ouvertes.fr/hal-03336585 — Pavanello et al. ETS 2021 photonic PUF review (entropy, ML-attack resilience).

### Citers of SPECS and the repository
- https://api.openalex.org/works?filter=doi:10.1049/icp.2023.2480 — OpenAlex: 5 citations of the ECOC paper.
- https://api.semanticscholar.org/graph/v1/paper/DOI:10.1049/icp.2023.2480/citations — the five citers: Cardoso JLT 2024, Cardoso ASP-DAC 2024, Costa VLSI-SoC 2025, Costa PGC 2025, and Aigner/Kissner/Paesler/Del Bino "Connect-A-PIC" (SPIE OPTO 2024).
- https://api.semanticscholar.org/graph/v1/paper/DOI:10.1117/12.2692187 — Connect-A-PIC: open-source no-code PIC design tool (citation context not retrievable).
- https://github.com/clement-z/specs/issues?q=is%3Aissue — 9 stars, 0 forks, 15 issues all opened by the maintainer (Jan 2025): #1 activity report, #2 logging, #3 global temperature dependence, #4 generic S-matrix device, #6 non-linear phenomena, #7 FIR filters, #9 sampled-time behaviour, #12 convergence checking, #14 SystemC-AMS evaluation.
- https://demo.zrounba.fr/specs/ — browser demo: netlist editor, probe / photodetector outputs, VCD, DataFrame and graph export.

### Photonic neural networks, in-memory computing, PUFs, reservoirs
- https://arxiv.org/pdf/2002.00281 — Feldmann et al., Nature 2021: 16x16 PCM crossbar fabricated, 16 comb wavelengths at 100 GHz spacing, 2 GHz modulation, 13.5 GHz on a 2x1 block.
- https://arxiv.org/pdf/2102.09360 — Feldmann et al., Nature 2019: all-optical spiking network, 4 neurons, 60 synapses, 140 elements, WDM, PCM ring neuron.
- https://arxiv.org/pdf/1801.06228 — Rios et al., Sci. Adv. 2019: GST scalar multiplication with 13 SNR-limited levels.
- https://api.semanticscholar.org/graph/v1/paper/DOI:10.1109/JLT.2021.3099914 — Carrillo et al., JLT 2021: compact behavioural PCM model for commercial PIC simulators, demonstrated on a spiking network and a tensor-core crossbar (the direct alternative to SPECS's PCM cell).
- https://ar5iv.labs.arxiv.org/html/2210.10851 — Sturm & Moazeni, DATE 2023: 128x128 PCM crossbar, 64 levels, ~100 pJ / 100 ns programming, evaluated with SCALE-Sim (no circuit simulation).
- https://arxiv.org/abs/2602.19031 — SKYLIGHT 2026: PCM photonic processors "limited to ~10 channels"; 144x256 core; SimPhony with signal-proportional analog noise.
- https://arxiv.org/pdf/2109.01126 — ADEPT (DAC 2022): 256x256 MZI, 10/12-bit DACs for 8-bit output, 0.1 % phase error, laser power from shot-noise SNR.
- https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=(DOI:10.1038/ncomms4541) — abstracts: Vandoorne 2014 (16-node passive swirl reservoir, 12.5 Gbit/s), Tait 2017 (24-node simulated MRR weight-bank network), Grubel 2017 PUF (chaotic Si microcavity, 17.1 kbit keys), Lightmatter Nature 2025.
- https://arxiv.org/pdf/2011.00111 — Shastri et al., Nat. Photon. 2021: challenges = thermal tuning (mW per pi), variability trimming, analog memory precision, DAC/ADC, crosstalk, multi-die thermal.
- https://api.semanticscholar.org/graph/v1/paper/DOI:10.1515/nanoph-2020-0172 — de Lima et al. 2020 primer (architecture + compiler; cited by the ECOC paper for the missing intermediate abstraction level).
- https://arxiv.org/pdf/2501.07917 — Roadmap on Neuromorphic Photonics 2025 (Pavanello and Charbonnier among authors): Verilog-A "allowing for large scale PIC-EIC simulations"; spiking lasers section.
- https://arxiv.org/abs/2401.12942 — Morison, Shastri et al. 2024: Verilog-A photonic CTRNN neurons; "accurate and scalable simulation tools ... critical".
- https://arxiv.org/pdf/2211.01476 — Peserico et al. review: MZI meshes sensitive to fabrication variability and phase noise; MRR weights to temperature.
- https://www.ebi.ac.uk/europepmc/webservices/rest/search?query=(TITLE:"photonic" AND TITLE:"Ising") — Shastri-group photonic Ising machine, Nature 2025: 256 spins / 65,536 couplings, > 200 GOPS (sparse-activity system).
- https://physicsworld.com/a/photonic-computer-chips-perform-as-well-as-purely-electronic-counterparts-say-researchers/ — *[news]* Lightelligence PACE: 64x64, > 16,000 components at 1 GHz.

### Comparable PNN tools and simulators
- https://www.ebi.ac.uk/europepmc/webservices/rest/PMC6459821/fullTextXML — Photontorch paper full text: GPU batching makes CROW cost "practically zero up to about 100 rings"; photodetector with Butterworth + thermal/shot noise; SOA conceptual; active components inefficient.
- https://github.com/JeremieMelo/pytorch-onn — TorchONN: MZI / MRR / FFT-ONN / PCM layers with phase quantisation, gamma error, phase variation and crosstalk noise; CUDA; ~330 stars.
- https://github.com/fancompute/neuroptica — neuroptica: NumPy chip-level MZI meshes with EO nonlinear activations; ~279 stars.
- https://github.com/solgaardlab/neurophox — Neurophox: MZI mesh unitary parameterisation; ~131 stars.
- https://www.emergentmind.com/topics/simphony — SimPhony (ASU, DAC 2025, arXiv 2411.13715): device-to-architecture cross-layer modelling, TorchONN noise injection, ~15 pJ/MAC estimates.
- https://isqed.org/English/Archives/2025/Technical_Sessions/144.html — Illuminati (ISQED 2025): C++ PNN design-space exploration with DSENT power models.
- https://ansys.synopsys.com/products/photonics/interconnect — INTERCONNECT product page: sample/block transient, bidirectional, TWLM laser, Monte Carlo + corners, EIC co-simulation, Python API.
- https://lightwave.ee.columbia.edu/ — James, Bergman et al. 2019 "Time-efficient photonic variability simulator": Chaospy polynomial chaos around OptSim because brute-force Monte Carlo was "too computationally intensive".
- https://arxiv.org/abs/2402.10971 — Azhigulov et al. 2024 (also in A): reflections affecting lasers; data-driven bidirectional Verilog-A models.

### Prior event-driven and SystemC photonic simulators
- https://www.sld.cs.columbia.edu/pubs/chan_tcad11.pdf — PhoenixSim (Chan et al., TCAD 2011): OMNeT++ event-driven, device library with per-port latency and insertion-loss matrices, inter/intra-message crosstalk, receiver noise, energy per bit incl. SerDes; thermal and nonlinearity listed as future work.
- https://omnetpp.org/software/2010/07/09/phoenixsim-a-simulator-for-photonic-interconnection-networks.html — PhoenixSim release note.
- https://api.semanticscholar.org/graph/v1/paper/DOI:10.1145/2857058.2857061 — Rumley et al. 2016: PhoenixSim cross-layer (physical / network / application) with an HPCG case study.
- https://api.crossref.org/works?query.bibliographic=Heterogeneous+Modelling+of+an+Optical+Network-on-Chip+with+SystemC+Briere — Briere et al., RSP 2005 (DOI 10.1109/RSP.2005.25): SystemC ONoC model estimating power, latency, area and bandwidth.
- https://api.crossref.org/works?query.bibliographic=Parini+Ramini+Bellanca+Bertozzi+SystemC — Parini et al. 2011-2012: FDTD-derived S-matrices as SystemC blocks, 160-microring network, BER-driven minimum launch power.
- https://api.crossref.org/works/10.1007/978-3-319-75178-8_57 — PhotoNoCs (Crespo et al., Euro-Par workshops 2018): metadata only; abstract not retrievable.

Unverified by strand B (search budget exhausted): whether Costa et al. 2025 used SPECS; Connect-A-PIC's citation context; the reason for the FiO 2023 retraction; photonflux details; any SystemC-AMS photonic publication (none found).
