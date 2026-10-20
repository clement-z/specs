# 05 — Paper, history & conventions (discovery probe 5/5)

Scope: intent (ECOC 2023 paper), docs, git history/workflow, code style, licensing.
Repo: worktree `sprint/2026-10` @ d9eb1e6. Method: static reading + `clang-format --dry-run` + parse-only/benchmark runs of the pre-built
`/home/czrounba/cernbox/perso/git/specs/build/specs` (Feb 2025, dev branch, **unoptimised build**). All scratch files in `/tmp`, deleted at the end.

---

## 1. Summary

- **What the paper says**: SPECS is a SystemC event-driven simulator for photonic circuits. Signals are a baseband complex field plus a carrier wavelength. Waveforms become step "events". Devices compute their outputs and schedule them after device-specific delays. abstol/reltol settings stop tiny changes from propagating. Netlists are SPICE-like and can come from KiCad. The paper claims fs accuracy and a speed-up of several thousand times over Photontorch and Interconnect on a CROW benchmark.
- **Does the code match?** The core claims are implemented and easy to trace:
  - Event queue plus emit decision: `src/optical_output_port.cpp:95-188,326-358`.
  - Tolerances: defaults `1e-8`/`1e-4` (`src/specs.cpp:31-32`), which match the paper's benchmark settings exactly.
  - Hierarchy (`.subckt`), KiCad compatibility, and `.op/.dc/.tran` analyses.
- **Gaps against the paper**:
  - The paper's own Fig. 1b netlist does not parse today. `.save` is lexed but has no grammar rule, `.dc WL(cwsrc1)` syntax is unsupported, and the `dc1` coupler prefix is now `COUPLER` (verified by running the binary).
  - The "generic Python module for fast prototyping" does not exist. `pyspecs/` only runs the binary and parses VCD.
  - "Generic S-matrix devices" exist only as an abstract class, not usable from a netlist.
  - There is no impulse-response analysis.
  - Mixed-mode (sampled-time) is an empty stub that silently drops every output event.
- **CROW benchmark**: the testbench exists (`-t ac_crow`) but uses different parameters from the paper, and `--nrings_crow` does nothing. With a paper-like netlist (via a generator) and the **default CMake build (no `CMAKE_BUILD_TYPE` → -O0)**, a 20 ns run takes 2.3 s (N=3), 14.6 s (N=10) and 21.3 s (N=30). After the paper's ×0.015 normalisation to 300 ps that is about 35 / 220 / 320 ms, against the paper's ≤~40 ms plateau. The plateau trend does appear; the absolute numbers are 3–8× off, which most likely comes from -O0 plus VCD output.
- **Docs are thin and partly stale**:
  - README points to an external website.
  - `QUICKSTART.md` was deleted, but Doxyfile and mainpage still reference it.
  - The cheatsheet and SVGs describe old syntax and old classes.
  - CHANGELOG is missing most changes since v0.1.
  - CI is GitLab-only on a GitHub remote.
- **History**: 33 commits by 2 people under 6 identities, in bursts: Sep–Dec 2023 (v0.1 + post-ECOC), Jan–Feb 2025 (CMake migration, SystemC 3.0.1, gtest), then nothing until Oct 2026. Branch model: work on `dev`, no-ff merge into `master`. `dev` is one commit ahead of `origin/dev` and not pushed (252a7f4 gtest).
- **Style**: `#pragma once` everywhere, includes relative to `src/` (`"devices/x.h"`), `m_` members, `p_*` ports, PascalCase classes, mixed camelCase/snake_case methods, `exit(1)`-style errors, partial Doxygen. **104/108 files violate `.clang-format`** (3421 warnings), so do not mass-reformat.
- **Licence**: LGPL-3.0-only text, with no headers in source files. Dependencies are compatible (SystemC Apache-2.0, args MIT, googletest BSD-3 for tests only). The GPL-3.0 text and the SystemC NOTICE are not shipped.

---

## 2. Paper digest — "Introducing SPECS" (ECOC 2023, Glasgow; HAL hal-04302200; Zrounba, Cardoso, Gomes de Queiroz, Jimenez, Abdalla, Bosio, Le Beux, Pavanello, O'Connor; INL/ECL; funded by ANR OPTICALL² & OCTANE)

**Motivation**
- Photonics has device-level EM simulation, circuit-level simulators (Lumerical Interconnect, Luceda Caphe/IPKISS, Photontorch) and system-level metrics. There is nothing in between ("intermediate abstraction level … still inexistent").
- Electronics solved this with HDLs and AMS event-driven solvers. Photonic circuits have sparse activity (ns signals vs ps propagation), so event-driven simulation fits them.
- Earlier event-driven photonic tools (PhoenixSim, PhotoNoCs, the SystemC ONoC work) only model interconnect/system metrics.

**Approach**
- Built on SystemC. This enables co-simulation with electronic and system-level models.
- Netlists are SPICE-like. They are hierarchical, and KiCad can generate them (Fig. 1).
- The current implementation is event-driven only. It could be extended to **mixed-mode** (for non-linear effects), or used to extend time-driven frameworks.

**Fig. 1 (example flow)**
- (a) Add-drop ring schematic in KiCad.
- (b) Netlist:
  - `.param wl0`, `.save P(out) P(drop)`
  - `dc1 in 1 out 2 k=0.15`, `dc2 add 3 drop 4 k=0.15`
  - `wg_ring_l 4 1 length=300e-6`, `wg_ring_r 2 3 length=300e-6`
  - `cwsrc1 in wl={wl0} power=1`, `.dc WL(cwsrc1) 1549.9e-9 1550.1e-9 1e-12`
  - `vlsrc1 in values="pulse.txt"`, `.tran 3e-9`
- (c) Results: frequency-domain P(out)/P(in) around 1550 nm, and time-domain P(out)/P(drop) over 3 ns.

**Signal model**
- Baseband: complex field amplitude E(t) plus carrier λ0.
- Waveforms are reduced to square pulses, and each pulse becomes at most 2 events (Fig. 2a).
- "Event-driven is naturally more suited to digital waveforms than continuously varying ones."

**Device model (Fig. 2b)**
- On an input event, the device computes `f(E,…)` and schedules output events at t1, t2… into a per-output queue.
- An "emit?" check runs at emission time.
- Delays are component-specific.

**Components claimed**
- Waveguides, couplers, phase-shifters, generic S-matrix devices, arbitrary sources, probes.
- "Functionally equivalent to other circuit simulators [Photontorch]".
- New components can be added in C++ **or via "a generic Python module for fast prototyping"**.

**Performance claim**
- Feed-forward circuits with digital inputs reach near-optimal simulation time by design.
- Circuits with feedback need the accuracy/runtime trade-off tuned.

**Accuracy settings**
- *Temporal resolution*: precision of event times. A coarser resolution is faster but less precise.
- *Absolute/relative tolerance*: changes below the threshold are not emitted. Without this, infinitesimal events loop forever in feedback circuits. These are not the same as a time-driven solver's tolerances, which control the time step.

**Other claims**
- Frequency-domain simulation and impulse-response calculation are "special use-cases" that run without external software.

**Benchmark (Fig. 3)**
- CROW with N rings (Fig. 3a):
  - 1 source emitting **1 mW, 1 ns pulses at 500 MHz**.
  - **2N waveguides, Li = (33.2 ± 5.0) µm**, normally sampled.
  - **N+1 couplers, κ = 0.15 power cross-coupling**.
- Machine: single thread on an i7-1165G7 with 16 GB RAM. No tuning on any tool.
- SPECS settings: **abstol 1e-8 V/m (0.1 fW)**, **reltol 0.01 %**.
- Validation (Fig. 3b): N=3 with long delays (L≈350 µm).
  - At 100 fs, Interconnect does not match Photontorch and SPECS.
  - At 1 fs all three agree (pairwise RMSE < ~3 µW), and **SPECS is 6290× faster than Photontorch and 5090× faster than Interconnect**.
- Scaling (Fig. 3c): N up to 512.
  - Photontorch and Interconnect simulate 3 ns (30000 steps at 100 fs).
  - SPECS simulates **10 pulses / 20 ns, normalised to 3000 timesteps (300 ps)**, averaged over 100 runs. Whiskers show the spread from random lengths.
  - SPECS saturates at **~40 ms beyond N≈20**, because losses stop meaningful signal (within tolerances) from propagating. The paper calls this "native input-dependent optimisation".
  - It is an order of magnitude faster than the published Photontorch GPU results.

**Limitations**
- SPECS cannot optimise circuits through backpropagation the way Photontorch can.
- Mixed-mode is needed for more complex (non-linear) circuits.

**Future work / vision**
- Mixed-mode simulation.
- Co-simulation of computing cores with photonic accelerators.
- Integration into other platforms.

**Poster (doc/resources/gdrsoc_poster.pdf, Jun 2023)**
- Same message.
- Adds: SPECS is usable "as a SPICE-like standalone program or as a SystemC library".
- Adds: SPECS is better at sub-timestep delays than Interconnect.
- Adds: SPECS is slower only when activity is high (temporal/spatial saturation).
- Adds: traces can be viewed in GTKWave or loaded into Pandas.

---

## 3. Paper ↔ code traceability

| # | Claim | Code location | Status |
|---|---|---|---|
| 1 | Built on SystemC (3.0.1 now) | `CMakeLists.txt:14`, `.gitmodules`, submodule `thirdparty/systemc` @3.0.1 | implemented |
| 2 | Baseband complex field + carrier λ | `src/optical_signal.h:19,26-27` (`complex<double> m_field`, `m_wavelength_id` → global `wavelengths_vector` `src/specs.h:108`) | implemented (multi-λ supported). Note: the "V/m" field unit is really √W (`power()=norm(field)`, `optical_signal.h:88-90`), and the paper uses the same convention (1e-8 V/m ≙ 0.1 fW) |
| 3 | Waveforms → steps → ≤2 events per pulse | `src/devices/value_list_source.h` (VLSRC entries `[t,P,λ]`), `parse_element.cpp:697-848` | implemented |
| 4 | Per-device delay scheduling + emit decision (Fig. 2b) | `src/optical_output_port.cpp:326-358` (queue, merge same-t/λ), `:95-188` (pop, tolerance check, write) | implemented |
| 5 | Absolute/relative tolerance | `optical_output_port.cpp:60-73,161-165` (emit only if both pass); defaults `optical_output_port.h:40-41`, `specs.cpp:31-32` = paper values; `.options abstol/reltol` `parse_directive.cpp:30-33`; CLI `--abstol/--reltol` `main.cpp:235-242,314-337` | implemented. Undocumented extra: outputs below 10·abstol are forced to 0 (`:167-170`, "TODO: check this !!") |
| 6 | Temporal resolution setting | Engine tick `.options timescale` / `-R` → `SPECSConfig::applyEngineResolution` `specs.h:131-134`; per-port multiplier `.options resolution` `parse_directive.cpp:36-37`; snapping `optical_output_port.cpp:301-324` (round-to-nearest) | implemented. Default is 100 fs (`specs.cpp:26`) but the header says 1 fs (`specs.h:96`) |
| 7 | fs accuracy / better sub-timestep handling than Interconnect | Phase computed analytically from length and λ, independent of the time quantisation: `src/devices/waveguide.cpp:60-72` | implemented (mechanism). No regression test |
| 8 | SPICE-like netlists | `src/parser/parser.l`, `parser.y`, `parse_*.cpp` | implemented, but syntax has drifted from the paper (rows 9-11) |
| 9 | `.save P(out) P(drop)` (Fig. 1b) | `parser.l:461` returns `T_DIRECTIVE_SAVE`; `parser.y:82` declares it, but no grammar rule uses it | **contradicted**: parse error (verified) |
| 10 | `.dc WL(cwsrc1) start stop step` (Fig. 1b, cheatsheet) | `parser.y:786-806` only accepts `/elem/ATTR` | **contradicted**: parse error (verified). `.dc /cwsrc1/WL …` works |
| 11 | `dc1 …` coupler element (Fig. 1b) | Element prefixes `parser.l:363-457` (`COUPLER…`, no `DC`) | **contradicted** (renamed). `circuits/add_drop_pyspecs.cir` is the current equivalent of Fig. 1b |
| 12 | `.param` | `parser.l:460` (alias of `.assign`) | implemented |
| 13 | Hierarchical descriptions | `.subckt/.ends` `parser.l:274-330,470-481`; `X…` `parse_element.cpp:1319`; `.include` | implemented |
| 14 | KiCad flow | `utils/specs.kicad_sym`; `.title/.end` ignored `parser.l:521-534` ("added fast with relatively no checking") | implemented (lightly) |
| 15 | Waveguide | `src/devices/waveguide.{h,cpp}` (`WG`: length, neff, ng, att dB/cm, D) | implemented |
| 16 | Coupler | `src/devices/directional_coupler.{h,cpp}`; `parse_element.cpp:278-330` | implemented. `k` is the **field** cross-coupling, `kp` the **power** one. The paper's κ=0.15 is power, so a reproduction needs `kp=0.15` |
| 17 | Phase-shifter | `src/devices/phaseshifter.{h,cpp}` (`PSHIFT`, electrical input) | implemented |
| 18 | Generic S-matrix devices | `src/devices/generic_transmission_device.h:19-93` (abstract, `prepareTM()=0` at `:82`); `generic_2x2_coupler.h` (unused); `generic_waveguide.h:18` "disabled (not working)"; no netlist element | **partial**. `todo.txt` still lists it as open |
| 19 | Arbitrary sources | `CWSRC` `cw_source.h`, `VLSRC` (list or file) `value_list_source.h:54-128`, `EVLSRC` (electrical) | implemented. `BitstreamSource` exists but is not in the netlist |
| 20 | Probes | `PROBE`/`MLPROBE` `probe.h:18,92`, `PDET` `detector.h`, `PWR` `power_meter.h` | implemented |
| 21 | New components in C++ | Base `src/devices/spx_module.h:12-36` + `OpticalOutputPort`; register in `parser.l`/`parser.y`/`parse_element.cpp` | implemented (requires touching 4+ files) |
| 22 | "Generic Python module for fast prototyping" of components | `pyspecs/specs.py` only runs the binary (subprocess); `specsparse.py` parses VCD into pandas. No Python device API or bindings | **missing** |
| 23 | Frequency-domain simulation | `.op`/`.dc` set ports to `NO_DELAY` (`specs.cpp:62-188`, `optical_output_port.cpp:371-405`), sweep `parse_analysis.cpp:29-80` | implemented. Limits: only 1 sweep per `.dc` (`parser.y:405-409`); only CWSource WL/P/F can be swept |
| 24 | Impulse-response calculation | — | **missing** (no analysis; can be approximated with a VLSRC short pulse + post-processing) |
| 25 | Mixed-mode (paper future work) | `OpticalOutputPortMode::SAMPLED_TIME` `optical_output_port.h:24`; `delayedWriteSampledTime` is an empty TODO `optical_output_port.cpp:360-369`; CLI `-m sampled-time` `main.cpp:297-301` | **missing**, and dangerous: selecting this mode silently drops every output event |
| 26 | Usable as a SystemC library / co-simulation | `libspx` SHARED, EXCLUDE_FROM_ALL `CMakeLists.txt:50` (excludes parser); electrical signal types `specs.h:24-51` | **partial**: no embedding API, example or docs; netlist parsing is not in the library |
| 27 | Feedback circuits terminate thanks to tolerances | Row 5 | implemented |
| 28 | Single-threaded | SystemC kernel | as claimed. Parallelisation is in `todo.txt` |
| 29 | CROW benchmark circuit | `src/devices/crow.{h,cpp}`, `src/tb/crow_tb.cpp`, `-t ac_crow` (`tb/alltestbenches.cpp:20`) | **partial**: fixed lengths (no N(33.2,5) µm sampling), κ_power=0.2, 1 W field=1 (not 1 mW), 4 pulses, 50 ns, λ reassigned 8× (`crow_tb.cpp:42-49`), `nrings_crow=3` hard-coded (`:33`), `--nrings_crow` is a no-op (`main.cpp:244-247,341-345`) |
| 30 | Speed figures (6290×/5090×, ~40 ms plateau), RMSE < 3 µW | — | **missing** as tests/harness. No reference data from Photontorch or Interconnect in the repo |
| 31 | GTKWave / Pandas (poster) | VCD output; `utils/openwave.sh`; `pyspecs/specsparse.py` (needs `pyDigitalWaveTools`) | implemented |

---

## 4. Reproducible benchmarks from the paper

| ID | Benchmark | How to reproduce now | Suggested test type |
|---|---|---|---|
| B1 | Fig. 1 add-drop: 2×300 µm half-rings, coupling 0.15, DC sweep 1549.9–1550.1 nm step 1 pm; TRAN 3 ns pulse | `circuits/add_drop_pyspecs.cir` (`.tran 3e-9`; the `.dc` is commented). Use `.dc /cwsrc1/WL …`. Choose `k` vs `kp` explicitly | **Regression/accuracy**: compare the DC sweep to the closed-form add-drop transfer function (FSR, extinction, drop peak) within tolerance. Cheap, deterministic, no golden files |
| B2 | Fig. 3b CROW N=3, L≈350 µm, 100 fs vs 1 fs | Netlist generator (below) with `timescale=-13` and `-15` | **Convergence regression**: RMSE(P_through@100 fs, P_through@1 fs) below a threshold; golden trace at 1 fs stored in-repo (small, CSV). Optional cross-check against Photontorch (pip, open source), not Interconnect |
| B3 | Fig. 3c CROW scaling N ∈ {3,10,32,100,512} | 1 mW 1 ns pulses @500 MHz ×10 (20 ns), Li ~ N(33.2,5) µm with a **fixed seed**, `kp=0.15`, `abstol=1e-8 reltol=1e-4 timescale=-13 traceall=0`; time × 0.015 to normalise to 300 ps; repeat over seeds | **Perf benchmark** (not unit test): track wall time per N and assert the plateau, e.g. t(512)/t(32) < 2. Requires Release build |
| B4 | Feed-forward "optimal time" claim | Splitter/WG tree with digital VLSRC | **Perf invariant**: number of events ≈ inputs × devices (count emitted events) |

**Measured during this probe** (pre-built `build/specs`, `CMAKE_BUILD_TYPE` empty → no `-O`, VCD with 2 probes on; generator in `/tmp`, now deleted). Wall times are for a 20 ns run; the normalised column is ×0.015.

| N | att=0 (WG default) | att=2 dB/cm | att=2 normalised to 300 ps | Paper (read off Fig. 3c) |
|---|---|---|---|---|
| 3 | 2.8 s | 2.3 s | ~35 ms | ~ms |
| 10 | 14.7 s | 14.6 s | ~220 ms | ~10–40 ms |
| 30 | 66 s | 21.3 s | ~320 ms | ~40 ms plateau |

- With loss the curve flattens (the plateau mechanism is visible). Without loss it does not.
- The paper does not give neff, ng, loss, or the build flags used.
- The VCD for 2 probes was 16–34 MB (88k–200k timestamps), so trace I/O probably accounts for a large share of the runtime.
- Generator sketch, to put into a future `bench/` folder: N+1 `COUPLERi l_in_i r_in_i l_out_i r_out_i kp=0.15`, with ends `in/through` and `add/drop`; ring i = `WGai r_out_i l_in_{i+1}` + `WGbi l_out_{i+1} r_in_i`.

---

## 5. Docs inventory & staleness

| Doc | Content | Status |
|---|---|---|
| `README.md` | 1 paragraph: points to external docs `demo.zrounba.fr/specs/doc` + web demo; contributing → `todo.txt`; citation (added in d9eb1e6); "Copyright INL" | **Thin**. No build, usage or licence section in the repo. The external site was not checked (offline) |
| `CHANGELOG.md` | `[unreleased]`: CMake switch ("Makefile should still function"); `v0.1.0` initial | **Stale**: omits SystemC 3.0.1, C++20, gtest, `libspx` target, multi-file netlists (13c2d1a), CLI-over-netlist priority, PWR element, PDET noise params, PCM update. The tag is `v0.1`, not `v0.1.0` |
| `todo.txt` | 4 items: general S-matrix device, non-linear WG, dispersion pulse flattening, parallelisation | Current but short. The longer pre-ECOC list was deleted in ce91d2d (see §10 feature seeds) |
| `doc/mainpage.md` | "Introduction TODO"; links `readme.html`, `quickstart.html` | **Stale/broken**: QUICKSTART.md deleted in e6da8de |
| `doc/Doxyfile` | Doxygen 1.8.16; `INPUT = src/ doc/mainpage.md README.md QUICKSTART.md` (`:832-835`); EXTRACT_ALL | **Stale**: missing input; only `make doc` (Makefile) runs it, there is no CMake doc target |
| `doc/custom.css` | Doxygen style | OK |
| `doc/resources/cheatsheet.pdf` (Jun 2023) | Signal model; directives `.op/.tran/.dc WL()`; options `timescale reltol abstol traceall`; elements CWSRC, VLSRC, EVLSRC, PROBE, COUPLER, WG, PSHIFT, SPLITTER, MERGER, MLPROBE, PD | **Partly stale**: `.dc WL()` does not parse; shows `pd 1 2` but the prefix is `PDET`; merger lists both `il` and `loss`; PSHIFT shows a stray comma. Missing: `kp`/`t` coupler args, `L` alias, VLSRC `file=`/`scale=`, PWR, PCMCELL, CROSSING, MZI, X/.subckt/.include/.param/.assign/.nodeset, `resolution`/`ts` options, CLI flags |
| `doc/resources/gdrsoc_poster.pdf` (Jun 2023) | Same as paper, plus "SPICE-like standalone or SystemC library" | Consistent with paper |
| `doc/resources/delayedwriter.svg`, `signals.svg` | UML of `DelayedWriter<T>` and `OpticalSignal{Type NONE/WRITE/ERASE/READOUT, parameter union}` | **Obsolete**: both classes have been replaced (`OpticalOutputPort`, field + wavelength_id). `waveguide.h` still has `@sa DelayedWriter` |
| `doc/papers/zrounba2023-specs-ecoc.pdf` | Paper (added d9eb1e6) | New |
| Deleted `QUICKSTART.md` (in history at `e6da8de^`) | Build with make, C++14/17, flex/bison, **SystemC 2.3.3**, `./sim -f`, GitLab URL `gitlab.inl90.ec-lyon.fr:czrounba/systemc-pcm-matrix-multiplication-simulator` | Confirms the project's pre-history (PCM matrix-multiplication simulator at INL GitLab, history squashed into "Initial release") |
| `.gitlab-ci.yml` | alpine + `systemc-dev` from edge/testing, `make bin lib`, test `./specs -t wg` only, doxygen + pages on master | **Dead on GitHub** (remote is `github.com/clement-z/specs`, no `.github/`). Uses the Makefile with C++17; artifact `doc/pcm-doc.tar.xz` is no longer produced |
| `install.sh` | Submodules → build SystemC (`build/install`) → `cmake .. -DBUILD_TB=1` | Does **not** build `thirdparty/args`, which `CMakeLists.txt:15` requires; no build type → -O0 |
| `config.mk` / `Makefile` | Legacy build, `-std=c++17 -O2`, SystemC at `thirdparty/systemc/sc_install` | Path differs from install.sh (`build/install`); `make rc` → missing `circuits/rc_mohab.cir`; `make test` covers 8 testbenches |
| `circuits/*.cir` | 10 examples | Verified with `specs -n`: OK: add_drop, add_drop_pyspecs, add_drop_sub, subckt_tests. **Fail**: double_ring, triple_ring, paul_simple_ring (legacy `one/clk/bsrc/dc/pdet` syntax), subckt_test (`cwsrc` with no suffix, `pow=`), paul_3ring_puf (needs a CSV that is not in the repo) |

---

## 6. Git history & workflow conventions

- **Remote**: `origin = ssh://git@github.com/clement-z/specs`. Pre-v0.1 history lived at the INL GitLab and was squashed into `ff9b8bb Initial release` (145 files, 23.8k lines).
- **Branches**:

  | Branch | Head | Notes |
  |---|---|---|
  | `master` = `origin/master` | `60dc3ab Merge branch 'dev'` | Jan 2025 |
  | `origin/dev` | `68785d4` | — |
  | **local `dev`** | `252a7f4` "Finally got gtest unit cases really working" | **1 ahead, not pushed**; checked out in the main worktree `/home/czrounba/cernbox/perso/git/specs` |
  | `sprint/2026-10` (this worktree) | `d9eb1e6` | = dev + paper commit |
  | `plan-repo-discovery-meson-migration-backlog-fixes` | `68785d4` | Local only, no commits of its own. The name hints at a planned Meson migration (unknown) |

- **Branch model**: day-to-day work on `dev`; releases via no-ff `Merge branch 'dev'` into `master`. No feature branches or PRs in the history. One annotated tag `v0.1` ("Public release", 2023-09-30).
- **Cadence** (commits/month): 2023-09: 2, 2023-10: 5, 2023-11: 9, 2023-12: 4 → gap → 2025-01: 11, 2025-02: 1 → gap → 2026-10: 1. The work comes in bursts tied to milestones: ECOC Oct 2023, then a build modernisation in Jan 2025.
- **Authors**: Clément Zrounba (19 commits, under the GitHub noreply, ec-lyon.fr, cern.ch and "Clement Viken Zrounba" identities); Raphael Cardoso (7 commits, as `rcardoso@ec-lyon.fr` and `cardosorapha` gmail). There is no `.mailmap`.
- **Recent changes (2025)**:
  - 8237968 → 911ea9c: CMake migration; include paths normalised to `"dir/file.h"` relative to `src/` (821f6fd, 94 files); testbenches moved into an OBJECT lib.
  - 1f08662: `libspx` shared target + gtest boilerplate.
  - 50ae939: SystemC → 3.0.1 (C++20).
  - 68785d4: cmake/googletest + coverage scripts.
  - 252a7f4: gtest **fork-per-test** macros, because the SystemC kernel cannot be re-elaborated within one process.
- **Commit message style**:
  - Short one-line subjects with mixed tense and case ("add changelog", "Fix bug in bidirectional devices handling", "removed outdated quickstart").
  - An occasional `area:` prefix (`cmake:`, `doc:`). Bodies are rare.
  - No issue references, no Conventional Commits.
  - **Recommendation for CLAUDE.md**: imperative mood, ≤72 characters, optional `area:` prefix (`parser:`, `devices:`, `cmake:`, `doc:`, `test:`), and a body when the change is non-trivial.
- **CI**: none effective (GitLab YAML on a GitHub repo). Tests are run manually: `./specs -t <tb>` and `unit_tests`.

---

## 7. Code style conventions (input for CLAUDE.md)

**Language and build**
- C++20 (`CMakeLists.txt:11`), SystemC 3.0.1, flex/bison parser (reentrant, caseless, `%option bison-bridge`).
- Flags: `-Wall -Wextra -Wfatal-errors -march=native`.
- Build with `cmake -DCMAKE_BUILD_TYPE=Release`. The default is unoptimised.

**Headers**
- Always `#pragma once`. No include guards and no licence headers anywhere.

**Includes**
- Project headers use quotes, relative to `src/`: `#include "devices/waveguide.h"`, `"utils/…"`, `"parser/…"`. This is enforced through `-iquote src` since 821f6fd.
- `<systemc.h>`, not `<systemc>`.
- Generated parser headers are included as `"parser.h"` / `"scanner.h"`.

**Naming**
- Classes: PascalCase (`OpticalSignal`, `WaveguideBi`, `DirectionalCouplerUni`). Exceptions: `CROW`, `SPECSConfig`, `spx_module`.
- Uni/Bi variants share a `…Base` class (`WaveguideBase`, `PhaseShifterBase`, `CrossingBase`).
- Members: `m_` prefix with units in the name (`m_length_cm`, `m_attenuation_dB_cm`).
- Ports: `p_in`/`p_out`, or `p0_in`/`p1_out` for bidirectional devices. Output writers: `m_out_writer`, `m_p1_out_writer` (type `OpticalOutputPort`).
- SystemC thread methods: `on_<port>_changed()`.
- Ordinary methods mix camelCase (`delayedWrite`, `getWavelength`, `setLength`) and snake_case (`drop_queue`, `snap_to_next_valid_time`, `build_circuit`). **Follow the surrounding class.**
- Files are snake_case `.h/.cpp`. Testbenches: `src/tb/<dev>_tb.{h,cpp}` with a `<Dev>_tb_run()` function registered in `src/tb/alltestbenches.cpp`. Unit tests: `src/test/devices/<dev>.cpp`.
- Type aliases use `typedef` (74 uses; `using X =` is never used). Shared signal and port types live in `namespace spx` (`specs.h:21-52`).

**Units**
- SI in netlists (metres, W). Internal units are mixed: cm for waveguide length, dB/cm, ns delays via `sc_time(x, SC_NS)`.
- The field is √W but labelled V/m.
- The 1.55 µm reference wavelength is hard-coded in device models.

**Error handling**
- `cerr << …; exit(1);` (≈119 sites), plus `assert` (63) and `sc_stop()`. Almost no exceptions (3 `throw`).
- Parser errors go through `yylerror(...)` and then `exit`.
- New code should keep this pattern unless a refactor is agreed.

**Global state**
- A single global `specsGlobalConfig` (`specs.cpp:19`) holds options, analyses and the trace file.
- Ports get their config in `prepareSimulation()`, which must run before `sc_start`.

**Tests**
- One SystemC elaboration per process. Unit tests wrap each test body in `SPECS_TEST_HEADER()/SPECS_TEST_FOOTER()` (fork + pipe, `src/test/utils.h:28+`).
- Helpers: `EXPECT_CPLX_EQ` and `EXPECT_ANGLE_EQ`.
- gtest is fetched with FetchContent, which needs network. It is built only when `BUILD_TB=ON`; legacy testbenches need `BUILD_OLD_TB=ON`.

**Comments and docs**
- Doxygen `/** … */` on some headers (16/57) with `@param` and `@sa`. Many `//` comments and commented-out code blocks; `#if 0` appears 5×. 41 TODO/FIXME.
- Banner comments: `/** ****…**** **/`.

**Formatting**
- `.clang-format`: LLVM-like, 4 spaces, ColumnLimit 88, `BreakBeforeBraces: Custom` with only `AfterFunction: true`, `BreakConstructorInitializers: BeforeComma`, `PointerAlignment: Right`, `SortIncludes`, `Standard: Cpp11` (outdated).
- Reality: **104/108 files are non-compliant** (3421 dry-run warnings; 376 lines > 88 columns).
- Control statements mostly use Allman braces (`if (...)\n{`), which contradicts `AfterControlStatement: false`.
- **Rule: never run `make format` (it runs `clang-format -i` on the whole tree); match the local style; reformat only lines you touch.**

**Do not**
- Add `using namespace std;` to new headers. It is already in `optical_signal.h:15`, `bitstream_source.h` and `spx_module.h`, and leaks everywhere.
- Add new globals.
- Print in hot paths: `crow.cpp` prints per element, and VLSRC logs every emission.

**Adding a netlist element** touches: lexer prefix (`parser.l` ~363-457), token and rule (`parser.y`), `XxxElement` class (`parse_element.h`) and `::create` (`parse_element.cpp`, with an `INSTANTIATE_AND_CONNECT_{UNI,BI}` macro), `devices/alldevices.h`, and the cheatsheet/docs.

---

## 8. Licensing

- **SPECS**: `LICENSE.txt` is the **GNU LGPL v3** text only.
  - No "or later" statement, no copyright line, no SPDX tags or per-file headers.
  - README says "Copyright © Institut des Nanotechnologies de Lyon (INL)".
  - LGPLv3 is a set of extra permissions on top of GPLv3, so the **GPLv3 text (COPYING) should ship as well**. It does not.
- **thirdparty/systemc** (submodule, tag 3.0.1): **Apache-2.0** with a NOTICE file (Accellera, Apple, Arm…).
  - Apache-2.0 → (L)GPLv3 is compatible in one direction (FSF).
  - If binaries are distributed (releases, the demo site, `libspx.so`), ship the SystemC LICENSE and NOTICE.
- **thirdparty/args** (Taywee/args 6.2.2-11): **MIT**, header-only. Compatible; keep the notice.
- **googletest** (FetchContent, release-1.12.0): BSD-3. Test-only, not distributed. Compatible.
- **Python runtime deps** (not declared anywhere: no `requirements.txt`/`pyproject`): numpy, pandas, matplotlib (BSD/PSF-style), pyDigitalWaveTools (MIT), graphviz (MIT). All compatible.
- **Paper PDF** (`doc/papers/`): HAL deposit "©2023 The Author(s)". The redistribution licence is not stated (see §9).
- **KiCad symbol library** `utils/specs.kicad_sym`: original work, covered by the repo licence. This is an assumption.

---

## 9. Unknowns

- Exact paper benchmark configuration: waveguide neff, ng and loss; SPECS commit/flags (v0.1 of 2023-09-30 is the likely candidate); probes and tracing on or off. Fig. 3c values were read off the figure and are approximate.
- How much of the measured 3–8× gap is -O0 versus VCD I/O versus code changes since 2023. No Release binary was available (`cmake-build-release/` has no `specs`), and nothing was built, by instruction.
- Whether `demo.zrounba.fr/specs` (online docs and demo) is still live (no network).
- Whether the legacy `Makefile` build still works (SystemC path and C++17 mismatch with CMake). Not tested.
- Intent of the local branch `plan-repo-discovery-meson-migration-backlog-fixes` (is a Meson migration planned?).
- Copyright and ownership of 2025+ contributions (made from a cern.ch address) versus "Copyright INL" in the README. Whether the licence is "LGPL-3.0-only" or "-or-later". The paper PDF's licence.
- Whether the INL GitLab pre-history (the PCM matrix-multiplication simulator) is preserved anywhere.
- Whether the `octane_*`, `clements`, `mesh_col` and `pcm_device` models (OCTANE/OPTICALL² projects) are meant to stay public or are research leftovers.

---

## 10. Seeds

### Seeds for backlog

- [high] CMakeLists.txt:1-13 / install.sh:28 — No default `CMAKE_BUILD_TYPE`, so the documented install path gives a -O0 binary. Measured CROW N=3 at 2.3 s for 20 ns. Set Release/RelWithDebInfo by default.
- [high] src/optical_output_port.cpp:360-369 (+ src/main.cpp:297-301) — `-m sampled-time` is accepted, but `delayedWriteSampledTime` is empty, so every optical output is silently dropped. Reject the mode until it is implemented.
- [high] install.sh:9-29 vs CMakeLists.txt:15 — install.sh never builds or installs `thirdparty/args`, but CMake requires `thirdparty/args/build/install`. Configuring a fresh clone should fail (static reading; not run).
- [med] src/parser/parser.l:461 + src/parser/parser.y:82 — `.save` is lexed but has no grammar rule, so it is a parse error (verified). The paper's Fig. 1b netlist fails. Implement it (e.g. map to probes) or drop the token.
- [med] src/parser/parser.y:786-806 — `.dc` only accepts `/elem/ATTR`. The paper and cheatsheet `WL(cwsrc1)` form is a parse error (verified). Add the SPICE-style alias.
- [med] src/parser/parser.l:469 — `^\.IC { exit(1); }` exits with code 1 and no message. `ICDirective::create` (parse_directive.cpp:128-131) is unreachable.
- [med] src/parser/parse_directive.cpp:65-68 — `.nodeset` is lexed and parsed, then always exits: "NODESET is disabled pending bugfixes".
- [med] src/main.cpp:244-247,341-345 + src/tb/crow_tb.cpp:33 — `--nrings_crow` is a no-op; N is hard-coded to 3. The paper's benchmark testbench does not use the paper's parameters (κ, power, random lengths, 10 pulses).
- [med] doc/Doxyfile:832-835, doc/mainpage.md:3-12 — Reference the deleted QUICKSTART.md/quickstart.html; the mainpage is a TODO. The repo has no build/usage docs since e6da8de.
- [med] .gitlab-ci.yml — GitLab CI on a GitHub remote (dead). It used the Makefile + C++17 and ran only `./specs -t wg`. Add GitHub Actions (CMake Release + unit_tests + parse check of `circuits/*.cir`).
- [med] config.mk:6,20 — Makefile build uses SystemC path `sc_install` (install.sh uses `build/install`) and `-std=c++17` (CMake uses C++20). CHANGELOG claims the Makefile still works. Fix it or remove it.
- [low] src/optical_output_port.cpp:416-420 — The `FREQUENCY_DOMAIN` case has no `break` and falls into `default`. Harmless today.
- [low] src/optical_output_port.cpp:67-73 — reltol divides by `|last|` (inf or NaN when `last==0`) and relies on IEEE comparison semantics. Add a guard and a comment.
- [low] src/optical_output_port.cpp:167-170 — Values below 10·abstol are forced to zero ("TODO: check this !!"). This hidden accuracy behaviour is not documented anywhere.
- [low] src/specs.h:96 vs src/specs.cpp:26 — Default timescale is `ONE_FS` in the header and `HUNDRED_FS` in the constructor. The effective default is 100 fs.
- [low] src/parser/parse_element.cpp:314-322 — Copy-paste comments ("convert from cross-coupling field coef" on the `kp` and `t` branches). Document that `k` is field coupling and `kp` is power coupling.
- [low] src/devices/waveguide.cpp:6-279 — The same body is triplicated (Uni, Bi p0, Bi p1). λref 1.55e-6 is hard-coded (`:25,28,60,84-86`…).
- [low] src/optical_signal.h:15,120-123,147 — `using namespace std` in a core header; `operator<` is inverted; `operator=` returns by value.
- [low] src/main.cpp:65-141,187,189 — Dead code: `do_circuit`, `raphael_main`, a declared-but-never-defined `tests_module_registry_access`.
- [low] Makefile:279-280 — `make rc` references the missing `circuits/rc_mohab.cir`.
- [low] circuits/double_ring.cir:4, triple_ring.cir:4, paul_simple_ring.cir:4, subckt_test.cir:5, paul_3ring_puf.cir — Do not parse or run (legacy syntax or missing CSV; verified). Fix or delete them.
- [low] utils/draw_graph_from_json.py ≡ pyspecs/draw_graph_from_json.py — Byte-identical duplicates.
- [low] doc/resources/delayedwriter.svg, signals.svg; src/devices/waveguide.h (`@sa DelayedWriter`) — Describe removed classes.
- [low] doc/resources/cheatsheet.pdf — Shows `.dc WL()` syntax and the `pd` prefix; many elements, options and directives are missing. Regenerate it from the parser.
- [low] CHANGELOG.md — Misses every change since v0.1 except CMake; the tag is named `v0.1`, not `v0.1.0`.
- [low] LICENSE.txt — Ships LGPLv3 without the GPLv3 text; no SPDX headers; no SystemC NOTICE in distributions.
- [low] cmake/googletest.cmake:6-16 — Needs network. `FIND_PACKAGE_ARGS` requires CMake ≥3.24, but the minimum is 3.14. Unit tests are tied to `BUILD_TB` (confusing next to `BUILD_OLD_TB`).
- [low] cmake/coverage.cmake — Never included (`add_coverage_target` is commented out in src/test/CMakeLists.txt).
- [low] (repo) — No `.mailmap` (6 identities for 2 authors). Local `dev` is one commit ahead of `origin/dev` and unpushed (252a7f4). `master` lacks the SystemC 3.0.1 and gtest work.
- [low] pyspecs/ — No `requirements.txt`/`pyproject` (needs numpy, pandas, matplotlib, pyDigitalWaveTools); not installable as a package.

### Seeds for feature scout

- **Mixed-mode simulation** (paper future work #1): implement `SAMPLED_TIME` ports (`optical_output_port.cpp:360`) plus event↔sample conversion at boundaries. `spx_module::ModuleFlags {NON_LINEAR, TIME_VARIANT, FREQUENCY_DEPENDENT}` (`spx_module.h:22-28`) already hints at per-device dispatch.
- **Non-linear waveguide** (todo.txt; paper limitation): Kerr, TPA or free-carrier effects. Depends on mixed-mode.
- **Dispersion-induced pulse broadening** (todo.txt): today `D` only changes per-λ phase and group delay (`waveguide.cpp:84-98`); edges are never reshaped.
- **Measured S-matrix device** (todo.txt; paper claim): make `GenericTransmissionDevice` concrete, add a Touchstone `.sNp` / CSV loader and a netlist element. Fix `GenericWaveguide`.
- **Parallelism** (todo.txt): process-level parallel sweeps and Monte-Carlo (cheap, since the SystemC kernel is single-threaded); job manager in pyspecs ("paralel run" idea at `pyspecs/specs.py:8-15`).
- **Python component prototyping** (paper claim, missing): pybind11 bindings to `libspx`, or a Python-callback device.
- **Impulse-response analysis** (paper claim, missing): `.ir` analysis = Dirac VLSRC + FFT → frequency response, cross-checked against `.dc`.
- **Co-simulation API** (paper vision): put the parser and `build_circuit` into `libspx`, add a C++ embedding example, and a SystemC-AMS/TLM bridge to an electronic core.
- **Paper benchmark suite**: CROW netlist generator + seeded Monte-Carlo + timing harness (B3), accuracy goldens (B1/B2), optional Photontorch cross-check.
- **Netlist compatibility with the paper**: `.save`, `WL(src)` sweep syntax, `DC` coupler alias, a working `.ic`/`.nodeset`, nested or multiple `.dc` sweeps, sweeping any element parameter (not only CWSource).
- **Output formats**: VCD is huge (16–34 MB for 2 probes / 20 ns); add CSV/HDF5/Parquet probes ("hdf5" was on the old todo list).
- **Expose built-but-unreachable models** in the netlist: BitstreamSource, Ring, CROW, Clements/mesh_col, OCTANE cell/segment/matrix, TimeMonitor.
- **Old pre-ECOC todo list** (removed in ce91d2d):
  - EO modulator, LFSR and PWL sources.
  - OPTICALL² photodetector.
  - Energy monitors (source energy, energy left in the circuit).
  - Material/parameter table.
  - Multi-mode propagation.
  - Logging.
  - xschem GUI link.
  - Netlist reference docs.
