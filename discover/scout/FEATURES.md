# SPECS feature scout: ranked missing features and roadmap (sprint 2026-10)

Scope: *which missing features would add the most value to SPECS's users (photonic-circuit researchers) and to
its adoption, at reasonable cost?* Bugs are out of scope here (the review workstream owns `backlog.md`); where a
feature is blocked by a known defect the defect is named. Everything below is grounded in the code at 184041a,
the ECOC 2023 paper, the five discovery reports, four web-research strands (`SOURCES.md`, 288 sources) and
measurements on a Release build (`bench/RESULTS.md`). Design notes for the top features are in `F-NN-*.md`.

## 0. What the evidence says

**Who uses SPECS today.** Inside INL only the OCTANE / PCM strand (Cardoso: PCM accumulation model, 1e5 random
matrix multiplications per tile size, COMET RISC-V core wrapped as a SystemC module next to the photonic GEMM).
The PUF strand (Jimenez) used a custom temporal coupled-mode code plus Lumerical INTERCONNECT; the reservoir
(Abdalla) and MZI-mesh PNN (Gomes de Queiroz) strands used Photontorch; the architecture strands use SNIPER and
gem5. The ECOC paper has 5 citations (4 from INL, 1 from the no-code tool Connect-A-PIC); the repository has 9
stars and 15 issues, all opened by the maintainer in January 2025 (temperature #3, generic S-matrix #4,
nonlinearity #6, FIR filters #7, sampled-time #9, convergence check #12, activity report #1, logging #2,
SystemC-AMS #14). The maintainer's own thesis benchmark on a 5-ring, 25-wavelength PUF found SPECS only 2.2x
faster than INTERCONNECT because feedback loops multiply events and the cost is linear in wavelengths.

**What those users could not do in SPECS and did elsewhere** (strand B, Tier 1): a bandwidth-limited receiver
chain (Butterworth photodiode, TIA noise, ADC quantisation, RIN); seeded Monte-Carlo over device variability
with batch statistics; a richer PCM model (complex-valued state-dependent transmission, asymmetric SET/RESET,
stochastic levels, electrical actuation); thermal effects; and a mixed-mode engine for continuous waveforms.

**What every comparable tool has and SPECS lacks** (strand A matrix): S-parameter import (Touchstone and the
Lumerical `.sparam` text format used by SiEPIC, Simphony and SAX), arbitrary parameter sweeps, Monte-Carlo /
corners, a packaged Python API, tabular export, and PDK/compact-model libraries. SPECS is the only open-source
*time-domain* engine in that matrix besides the dormant Photontorch, and the only one built on SystemC.

**What the measurements say** (`bench/RESULTS.md`): the paper's plateau reproduces (user time flat from 30 to
300 rings, ~86 ms normalised vs the paper's ~40 ms on another CPU); the *default* all-net VCD tracing costs 4.4x
the engine time and writes a 1 GB file at 30 rings; WDM cost is linear in channel count; 1 fs resolution costs
240x over 100 fs; the `.dc` fixed point matches the closed-form add-drop response to 1e-6; ~16 % of engine time
is coroutine switching, 11 % is `hypot` in the tolerance gate, 4 % is `sincos` recomputed per event.

**What the event-driven approach uniquely enables** for these users: digital-driven, lossy, large circuits
(the 512-ring CROW; PCM crossbars at 1 GHz with 1e5 random inputs; Feldmann-class 16-wavelength tensor cores if
WDM cost is reduced), PUF Monte-Carlo over tens of instances and hundreds of challenges, spiking / event-based
neuromorphic systems, and cycle-accurate co-simulation with SystemC processor models. Continuous-waveform
reservoirs and gradient-based training stay better served by Photontorch / TorchONN, which argues for
interoperating with them (Python API, S-dict export) rather than competing.

## 1. Ranked table

Value is for the user groups above plus adoption (5 = blocks current work or is expected by every tool user).
Effort: S < 1 week, M 1-4 weeks, L 1-3 months, XL more (one experienced developer). Risk covers physics/numerics
sign-off (`needs-human` per CLAUDE.md), golden-output churn and architectural uncertainty.

| ID | Feature | Value | Effort | Risk | Prerequisites | Rank rationale |
|---|---|---|---|---|---|---|
| F-01 | **Reproducible runs**: `--seed` (echoed like Xyce's `-randseed`), `--version` (git hash, SystemC, compiler), name-ordered object iteration, run manifest in VCD/JSON, no dark-field RNG injection | 5 | S | low (golden churn: VCD order only) | none | Every Monte-Carlo, PUF and golden result depends on it; today `Detector::init()` reseeds from `random_device` and object order follows pointer values. Cheapest high-leverage item. |
| F-02 | **Parameter sweeps and Monte-Carlo**: `--param NAME=VALUE` CLI overrides, `.step` over any element parameter, `.mc N seed=` with `gauss()/uniform()` in `.param`, process-parallel runner (fork per point) and aggregated outputs | 5 | M | low-med | F-01, F-05 | INL Tier-1 need (PUF: 20 instances x 200 challenges; GEMM: 1e5 runs); every competitor has it; `.dc` today sweeps only a CW source's lambda/P/f and device parameters are frozen at thread start, so re-elaboration per point is the robust design. |
| F-03 | **S-parameter device** `SPAR`: Touchstone 1.x/2.x and Lumerical/SiEPIC `.sparam` import, N ports, per-wavelength magnitude/phase interpolation, group delay from the phase slope | 5 | M (FD) / M+ (TD, needs-human) | med | none: written from scratch as a state-holding, per-wavelength device (the generic device is removed, decision 2026-10-23; former code at cc1e52e) | `todo.txt` item 1, issue #4, a paper claim ("generic S-matrix devices"), and the one capability users of every other tool take for granted; unlocks FDTD/PDK data and the SiEPIC library. |
| F-04 | **Receiver chain**: photodiode bandwidth (n-th order low-pass), TIA gain/noise, ADC quantiser, RIN on sources, electrical output actually driven and traceable | 5 | M | med (needs-human) | backlog fix of `p_readout` write | The bandwidth-limited readout decided the group's own results (43 % vs 95 % MNIST at 1 GHz; TDRC papers); SPECS has shot/thermal noise but no bandwidth, TIA, quantiser or RIN. |
| F-05 | **Outputs and tracing control**: `.save`/probe selection, `--no-trace`, CSV / NumPy `.npz` / Parquet table export per probe, decimation, FST later; pyspecs reader on `pywellen` | 5 | S-M | low | none | Measured: default all-net VCD is 4.4x the engine time and 1 GB at 30 rings; `.save` also restores the paper's Fig. 1b netlist; pyDigitalWaveTools has had no release since 2022. |
| F-06 | **Benchmark and accuracy suite**: CROW generator, paper-plateau assertion, 1 fs vs 100 fs RMSE, analytic ring/CROW oracles (Yariv, Bogaerts, Poon), optional Photontorch cross-check, nightly timing | 4 | S-M | low | none (prototype in `bench/`) | Protects the paper's claims and turns physics into cheap deterministic tests; the add-drop oracle already passes at 1e-6. |
| F-07 | **Python package**: `pip install pyspecs` with the binary bundled (meson-python + cibuildwheel, SystemC linked statically), `run/sweep/mc` API, pandas/xarray results, netlist builder, notebook examples | 5 | M | med (depends on Meson migration) | Meson (ws-meson), F-05 | Adoption: every PNN/PUF workflow in the group is Python/PyTorch; today `pyspecs` is an unpackaged subprocess wrapper with undeclared deps and a VCD parser bug. |
| F-08 | **Mixed-mode foundation**: sampled-time islands behind the existing `SAMPLED_TIME` port mode (SystemC-AMS TDF pattern) plus quantized-state (QSS) self-scheduled device states for slow dynamics | 5 (strategic) | L | high (needs-human) | F-01, F-06 | Paper future work #1, issues #9/#12, thesis conclusion; enabler for F-09, F-10, F-17, F-18 and lasers; QSS maps directly onto SPECS's abstol/reltol semantics. |
| F-09 | **Thermal model**: global `.temp`, thermo-optic coefficient in waveguides/rings, heater power-to-phase with tau_th, thermal crosstalk matrix, ring self-heating | 4 | S (static) / M (dynamic) | med (needs-human) | F-08 for dynamics | Issue #3; named as un-modelled by three INL theses; the static part is a one-line change in the waveguide phase law. |
| F-10 | **PCM cell v2** for OCTANE: continuous crystalline fraction, complex T(x)/phi(x), peak-power-gated amorphisation vs energy-gated crystallisation, dead time, stochastic levels, drift/endurance counters, optional electrical heater port | 5 (core user) | M | high (needs-human; published model) | F-01 | The only strand using SPECS asks for exactly this (Cardoso perspectives; Dwivedi/Pavanello 2026 electrical actuation); current defaults also make OCTANE cells opaque (backlog). |
| F-11 | **Embedding / co-simulation API**: parser and `build_circuit` in `libspx`, `Circuit` object instead of the global config, documented SystemC testbench example (processor model + photonic accelerator), TLM/electrical bridge | 4 | M-L | med | F-27 for clean errors | SPECS's unique asset (already wired to a RISC-V core in a thesis) has no public API: the library excludes the parser and `sc_main` owns everything. |
| F-12 | **Netlist compatibility pack**: SI suffixes, `.ends NAME`, `.dc WL(src)` alias, `.save`, `.include` relative to the including file, `.temp`, several analyses per run, function calls in expressions, `--param` | 4 | S-M | low | none | First-contact friction: 4 of 9 shipped netlists and the paper's own netlist do not parse today; SI suffixes are the most common user error. |
| F-13 | **Design-tool interop**: KiCad 8/9 custom netlist generator (XML to SPECS) and refreshed symbol library; gdsfactory YAML netlist import; SAX/Simphony-compatible S-dict/`.npz` export of `.dc` sweeps | 4 | M | low | F-03 (round trip) | Makes SPECS a drop-in time-domain backend for the gdsfactory/SAX community and repairs the advertised KiCad flow (today net names like `Net-(WG1-Pad2)` do not lex). |
| F-14 | **Fabrication variability**: width/thickness deltas mapped to neff/ng (sensitivities or a compact model), independent or layout-correlated sampling with instance coordinates, wafer maps | 4 | M | med (needs-human) | F-02 | PUF and MZI-mesh studies need correlated variation (Lu 2017, Simphony layout-aware MC); SPECS already samples lengths in the CROW benchmark but nothing else. |
| F-15 | **Engine performance and accounting**: squared-norm tolerance gate, per-wavelength S-parameter caches, `SC_METHOD` for stateless devices and output ports, single timed queue, O(1) same-time merge; `--stats` with events per port, energy per source, wall time (issue #1) | 4 | M | med (must stay bit-identical; coalescing is needs-human) | F-06 | Profile: 16 % coroutine switching, 11 % `hypot`, 4 % `sincos`, 4 % allocation; 1.5-2x available without touching physics. |
| F-16 | **In-process Python and Python-defined devices**: nanobind + meson-python extension, `PyDevice` with `SC_METHOD` semantics, process-isolated multi-run | 4 | L | med-high (GIL vs coroutine stacks) | F-07, F-11 | The paper promised a "generic Python module for fast prototyping"; positions SPECS as the time-domain checker for TorchONN/Photontorch models. |
| F-17 | **Waveguide nonlinearity**: instantaneous Kerr (SPM/XPM) and TPA per event; free-carrier state with lifetime via QSS; saturable absorber | 3 | S (instantaneous) / M (carriers) | high (needs-human; ring bistability) | F-08 for carriers | `todo.txt`, issue #6, PUF ML-resistance (Kieninger 2026); the per-event form is native to the formulation. |
| F-18 | **Sources and modulators**: PRBS/bitstream and PWL sources in the netlist, CW laser with RIN and phase noise (sampled), MZM with V_pi/ER/bandwidth, depletion ring modulator | 4 | M | med | F-04, F-08 (noise) | Every PNN/TDRC testbench starts with a modulator and a noisy laser; `BitstreamSource` exists in C++ but is unreachable from netlists. |
| F-19 | **Standard-cell library**: shipped `.subckt` library (add-drop, all-pass, CROW(N), Clements mesh, MRR weight bank, OCTANE cell/matrix, TDRC loop) and netlist access to the C++ composites | 3 | M | low-med | F-12 | Neuromorphic users think in weight banks and meshes; C++ composites (`CROW`, `Clements`, `OctaneMatrix`) are not `spx_module`s and have no netlist element. |
| F-20 | **Documentation and community files**: netlist reference generated from the parser tables, Sphinx+MyST site with 4 tutorials, `CITATION.cff`, CHANGELOG, CONTRIBUTING, Zenodo DOI per release, JOSS plan | 4 | M (`CITATION.cff` S) | low | F-26 helps | The only syntax doc is a stale PDF cheatsheet; JOSS/Zenodo are what reviewers and citers expect in 2026. |
| F-21 | **Distribution**: conda-forge recipe, Docker/Apptainer images, demo hardening; WebAssembly spike (Emscripten with the pthread/std::thread backend, Asyncify fiber backend) | 3 | M (containers) / L (wasm) | high (wasm) | F-07 | HPC users want Apptainer; a client-side demo is attractive but SystemC has never been shipped in wasm beyond one blog experiment. |
| F-22 | **WDM vectorised events**: one event carrying all wavelengths, wavelength-dependent device parameters | 3 | L | high | F-15 | Measured linear cost in channels (16 channels = 18x); constant-factor gain only, since the physics is per wavelength. |
| F-23 | **Impulse / frequency response from time domain** (`STEP` source + FFT of the event derivative, compared with `.dc`) | 2 | S | low | F-05 | Paper claim; mostly a pyspecs post-processing function. |
| F-24 | **Polarisation / multimode**: (wavelength, mode) event keys, 2x2 Jones devices, PSR/PBS | 2 | L | high | F-22 | Needed by few current users; touches every device. |
| F-25 | **Dispersion pulse broadening**: per-wavelength group delay now; edge staircase or FIR island later (issue #7) | 2 | M-L | med | F-08 | `todo.txt` item 3; negligible for cm-scale silicon at ns symbols. |
| F-26 | **Data-driven device registry**: one table per device (prefix, ports, parameters, units, defaults) generating parser kwargs, JSON, docs and KiCad symbols | 3 | M | low | none | Adding an element touches 5 files today; the same table feeds F-20 and F-13. |
| F-27 | **Diagnostics for embedding**: exceptions / `SC_REPORT` instead of ~119 `exit(1)` sites, file:line on semantic errors, `--verbosity` (issue #2) | 3 | M | low | none | Prerequisite for F-11/F-16; also fixes the one-error-per-run experience. |
| F-28 | **Frequency-domain convergence diagnostics**: iteration cap, residual report, non-convergence error (issue #12) | 3 | S | med (numerics) | none | Lossless loops hang the `.op` fixed point today with no message. |
| F-29 | **Multi-threaded SystemC kernel** | 2 | XL | very high | — | Not recommended: research kernels gain on loosely-timed TLM, not on fine-grained delta networks; process-level parallelism (F-02) gives N x on N cores for free. |
| F-30 | **Lasers and amplifiers**: rate-equation DML, SOA (Agrawal-Olsson), EDFA | 2 | L | high | F-08 | Sampled-island physics; valuable for the spiking-laser roadmap but after the enablers. |
| F-31 | **Activity map**: event activity from `--stats` drawn on the circuit graph (devices coloured by processed events, nets labelled with emitted/suppressed counts), time-resolved later, KiCad back-annotation after F-13 ([brief](F-31-activity-map.md)) | 3-4 | S / M / L | low | F-15 (done), `--json` | Explains a run's cost at a glance; a figure for the paper's activity discussion. Added 2026-10-23 at the maintainer's request. |
| F-32 | **Schematic from a netlist**: KiCad schematic inferred from the netlist with placement rules and an LLM in the loop (about 27B class, through an API key, model-family agnostic); gives F-31 a real schematic to colour ([brief](F-32-schematic-from-netlist.md)) | 4 | L | med | F-13, F-31 | Maintainer idea (2026-10-23), not scoped yet. |

## 2. Recommended roadmap

### Horizon 1: quick wins (each under one week, no physics change, golden-neutral or re-recorded once)
1. **F-01** seed/version/manifest and name-ordered iteration (re-record the golden baseline once for VCD order).
2. **F-05 (part 1)** `.save` and probe selection, `--no-trace`, CSV/`.npz` export, default `traceall=0` when probes exist.
3. **F-12 (part 1)** SI suffixes, `.ends NAME`, `.dc WL(src)` alias, `--param` overrides, `.include` relative to the file.
4. **F-06** move `bench/` into the tree, add the add-drop/all-pass oracles as unit tests and a nightly CROW timing job.
5. **F-28** iteration cap and residual message for the `.op`/`.dc` fixed point.
6. `CITATION.cff` + Zenodo integration (from F-20); `--stats` event counter (from F-15).

### Horizon 2: next quarter (the "INL can switch" package)
1. **F-02** sweeps and Monte-Carlo with the fork-per-point runner and aggregated outputs.
2. **F-07** packaged `pyspecs` with wheels (right after the Meson migration lands), `pywellen` reader, notebooks.
3. **F-03** `SPAR` device, frequency-domain semantics first (CW/`.dc` exact), then time-domain with group delay.
4. **F-04** receiver chain and RIN; **F-18 (part 1)** PRBS/PWL sources and MZM with V_pi and bandwidth.
5. **F-10** PCM v2 designed with Cardoso's measurements as the acceptance test; **F-09 (static)** temperature.
6. **F-13** KiCad generator + gdsfactory import + S-dict export; **F-15** engine pass (bit-identical refactors).
7. **F-14** variability models on top of F-02; **F-20** docs site; **F-26** registry if F-20 is done by generation.

### Horizon 3: strategic (quarters 2-4)
1. **F-08** mixed-mode foundation (sampled islands + QSS states) with a written numerics spec signed off by the
   maintainer; then **F-09 (dynamic)**, **F-17**, **F-18 (noise)**, **F-30**.
2. **F-11** embedding API and the processor + accelerator co-simulation example; **F-27** diagnostics first.
3. **F-16** in-process Python and `PyDevice`.
4. **F-19** standard-cell library; **F-22** WDM vectorisation if WDM workloads dominate; **F-21** containers and
   the WebAssembly spike; **F-24/F-25** only on demand. **F-29** is explicitly declined.

## 3. Dependencies between the top features

```
F-01 seed/determinism ──► F-02 sweeps/MC ──► F-14 variability
        │                      │
        │                      └──► F-07 python package ──► F-16 in-process python
F-05 outputs ───────────────────────┘                          ▲
F-06 bench/oracles ──► F-15 engine pass ──► F-22 WDM vectors    │
F-12 netlist pack ──► F-13 interop ◄── F-03 SPAR device         │
F-27 diagnostics ──► F-11 embedding API ───────────────────────┘
F-08 mixed-mode (QSS + sampled islands) ──► F-09 dyn, F-10 thermal proxy, F-17 carriers, F-18 noise, F-30 lasers
F-04 receiver chain (sampled island already exists in Detector)
```

## 4. Method and limits

Code anchors were checked against the worktree at 184041a (`src/`, `pyspecs/`, `utils/`, `circuits/`,
`tests/golden/`); measurements used `/tmp/ws-scout-build/specs` (CMake Release). Web facts come from the four
strands in `SOURCES.md`; items the strands could not verify are marked there. The backlog being written in
parallel (`ws-review`) owns the defects referenced here (detector output never driven, MZI bidirectional
miswire, PCM opaque defaults, pointer-ordered sets, `-m sampled-time` dropping events, GTD single-wavelength
latch); features that depend on them say so. No product code was modified; the only artefacts are under
`discover/scout/`.
