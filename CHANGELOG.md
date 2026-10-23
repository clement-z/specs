# Changelog

## [unreleased]

### New

* Accuracy suite (`tests/bench/`, F-06): SPECS's `.dc` and `.tran` results against closed forms, as the Meson
  suite `oracle`: the add-drop ring (Yariv, Bogaerts) on `circuits/add_drop.cir`, and CROWs with random ring
  lengths (Poon transfer matrices), complex through/drop responses within 1e-5; each check also proves that
  three injected physics errors (k read as a power ratio, no dispersion slope, a real cross coupling) would
  fail it. The CROW generator (`crow_gen.py`, from the feature scout) gains CW and `.dc` forms and a
  `crow_tb` preset that reproduces `specs -t ac_crow` (same event times, powers within 1e-12 W, checked by
  the suite)
* Benchmark (`tests/bench/bench.py`): the CROW case matrix with per-run user time, peak RSS, VCD size and
  `--stats` counters, appended to a local `tests/bench/history.jsonl` (not versioned); the Meson suite
  `bench` checks only ratios (event plateau from 30 to 300 rings, WDM linearity, and the user-time plateau on
  optimised builds without sanitizers)
* Activity map (`utils/activity_map.py`, F-31): the circuit of a run drawn with Graphviz's layout, each device
  coloured by its processed events (linear scale) over a background field that blends nearby devices over
  about a node's size (`--field-spread`) and is zero between devices that are not close, subcircuit
  instances as clusters, optional suppression-ratio discs (clear of the labels), a legend and
  the run totals; reads `--json` and `--stats=ports,devices`, writes DOT (standard library only), SVG and
  PNG (numpy, matplotlib, Graphviz; pixi's `py` environment now includes Graphviz). See
  `doc/activity_map.md`
* `SPAR`: an N-port device given by an S-parameter file (F-03, `doc/netlist.md` "SPAR", `doc/sparam.md`):
  Lumerical/SiEPIC `.sparam`, Touchstone 1.x/2.x, a CSV form and a Taylor-coefficient form. Phases are
  converted to SPECS's convention on import (Touchstone, written in the RF convention, is conjugated; a test
  shows that a waveguide written in each format gives the phase and delay of a `WG` of the same length).
  Magnitude and unwrapped phase are interpolated in frequency (`interp=linear|pchip`), real and imaginary
  parts of the entry with its delay (pulse-pair estimate) taken out near transmission nulls (B-190: an unbalanced
  MZI sampled 28 times per FSR is within 0.0016 between grid points instead of 0.023; with `pchip` the
  magnitude/phase slopes are taken per run between those intervals); in `.tran` the S-matrix of the carrier wavelength applies with a group delay
  per path (each path a delayed copy of its input; no dispersion within a pulse; negative delays become 0,
  reported per path). A grid too coarse for the device is reported when the file is loaded (leave-one-out
  check, real/imaginary parts where the device uses them on either side of the point). Unidirectional or bidirectional like `COUPLER`; a wavelength outside the data range is an error;
  `passivity=check|ignore|enforce`, `delay=auto|none|<s>`, `ports=`, `convention=`. Test data: the SiEPIC
  EBeam broadband directional coupler (MIT, `tests/sparam/data/siepic/NOTICE`)
* `utils/sparam_export.py`: writes the S-matrix of a subcircuit, from one `.dc` sweep per port, as
  `.sparam`, Touchstone or CSV (F-03)
* KiCad symbols `SPAR_2PORT` and `SPAR_4PORT` (`utils/specs.kicad_sym`)
* Parameter sweeps and Monte-Carlo (F-02, `doc/netlist.md` "Functions" and "`.step` and `.mc`"):
  * functions in expressions: `sqrt exp log log10 sin cos tan asin acos atan abs floor ceil min max`, and the
    random `gauss(mu, sigma)`, `uniform(a, b)`, `lognormal(mu, sigma)`, `choice(...)`. Each draw takes its own
    stream of the master seed, named after where it is (instance path, element or `.param`, assigned name), so a
    random parameter in a subcircuit body is drawn per instance; `shared=1` shares it between the instances of
    the subcircuit, `shared="name"` between every use of the name
  * `.step param NAME start stop step` (stop included, as `.dc`) and `.step param NAME list ...`, nested, and
    `.mc N [seed=S]`: one run per point, each in its own process (`-j N` at a time). Monte-Carlo run k has a seed
    derived from the master seed and k, the same at every step point
  * one run table for all runs, `<output>.runs/`: `runs.csv` (status, exit code, seed, stepped and drawn values,
    message of each run) and appendable NumPy columns of every run's probe samples and random draws, readable
    without loading them (`np.load(..., mmap_mode="r")`, `pyspecs/runtable.py`). A failing or crashing run is
    recorded and the others go on; running the same command again resumes (completed runs are skipped);
    `--overwrite` starts over. `--run K` simulates one run alone with the outputs of a single run

### Changed

* Run manifest (`<trace>.manifest.json`, `--print-manifest`) and run table manifest (`table.manifest.json`):
  `format_version` 2, with a `data_files` list (path and SHA-256 of each `SPAR` S-parameter file and
  `VLSRC`/`EVLSRC` value file read), so a `.step`/`.mc` plan is not resumed against a changed data file (F-01)
* `.options seed` is applied when its line is read (so that the random functions after it use it); a seed that
  changes after a random function drew is an error (F-02)
* MLPROBE: the wavelengths a probe does not list are reported in one warning per probe on stderr at the end of
  the run, in name order and with every digit needed to tell them from the listed ones, instead of one
  "received unsupported signal" line on stdout per event (B-062)
* VLSRC and EVLSRC are quiet by default: the values read from a value file (and its skipped lines),
  "`<name> was enabled`" and the "`@<t>, <name> emitted: <value>`" line of every emission are only printed with
  `--vci`. The values whose time is already past when the source reaches them are reported in one warning per
  source on stderr, with their count, instead of one "invalid time for signal emission !" line on stdout per
  value (VLSRC; EVLSRC skipped them silently). The emitted values are unchanged (B-066)
* Wavelengths equal within a relative 1e-12 are one wavelength (compile-time constant, `doc/numerics.md`): in the
  wavelength registry (the first registered value is kept), so sources at "the same" wavelength written
  differently (`1550*1e-9`, `1.55e-6`, c/f) add coherently instead of as two wavelengths, and in MLPROBE's
  wavelength list, so a computed wavelength is traced (B-010, B-062). A `.dc` point that drifted to within 1e-12
  of a registered wavelength takes that value (e.g. 1.550000000000001e-06 -> 1.55e-06 in `circuits/add_drop.cir`,
  values at that point change by about 1e-13 relative)
* `.dc`: each sweep point is a fresh steady state: every output port writes each of its wavelengths at least
  once per point, also when its value did not change since the previous point, so the wavelength of a source
  that is not swept reaches the nets and probes after it at every point (before, at the first point only); a
  PROBE on such a net now has a sample at every point. More events per point in such circuits (`--stats`);
  `.op` and `.tran` are unchanged (B-185)

### Fixed

* `.dc`: a sweep point whose output falls below `abstol` (e.g. a null of an MZI) is written (0 after the snap)
  instead of keeping the previous point's value and wavelength (B-003)
* Parse errors inside a `.subckt` body (also a nested one) give the file that defines the subcircuit and the line
  in it, as for top-level lines, instead of an empty file name and an uninitialised line number (B-182)
* pixi: `pixi run test` (and the other build tasks) failed with `gtest/gtest.h: No such file or directory` on a
  host with googletest installed (e.g. Arch Linux's `gtest`): the environment's pkg-config found the host's
  `gtest.pc`. The pixi builds now use the googletest wrap; remove a `build-pixi/<toolchain>` directory configured
  before this fix
* `CWSRC`: a negative power is an error when the element is created (`Error (...): power must be >= 0 and finite`,
  exit 1); it aborted in `std::polar` (builds with `_GLIBCXX_ASSERTIONS`, e.g. pixi) or gave a NaN field, and a
  `.step`/`.mc` run was recorded `crashed` instead of `failed` with the message (B-189)
* The run-table test (`tests/tools/test_run_table.py`, `pyspecs/runtable.py`) was always skipped in the pixi builds
  and in CI: numpy and pandas are now in pixi's default environment and installed in the CI jobs that run
  `meson test`; it still skips (exit 77) where they are missing (B-193)
* A netlist line longer than flex's buffer (16 KB, e.g. a `VLSRC` with 700 or more inline values) failed with
  `input buffer overflow, can't enlarge buffer because scanner uses REJECT` (exit 2). The scanner no longer uses
  flex's REJECT, so lines of any length parse; the tokens and error messages are otherwise unchanged (B-191)

## [0.3.0] - 2026-10-23

### New

* Optional pixi development environment (`pixi.toml`, `pixi.lock`): gcc 15 and clang 21 toolchains with `setup`,
  `build`, `test` and `golden` tasks in `build-pixi/<toolchain>` (also ASan+UBSan), a `py` and a `docs`
  environment. CI: pixi job on Ubuntu (blocking) and macOS (non-blocking), next to the apt jobs
* Agent instructions moved from `CLAUDE.md` to `AGENTS.md` (read by all coding agents); `CLAUDE.md` imports it
* Run statistics (F-15): `specs --stats` writes `<trace>.stats.json` with the counters of the optical output
  ports (values scheduled, coalesced, processed, emitted, suppressed by abstol/reltol/snap, pending entries,
  last emission) and the wall time; `--stats=ports,devices` adds one row per output port and per device type,
  `--stats-file` and `--print-stats` choose where it goes (stdout without a trace file); also for the `-t`
  testbenches. Off by default, no change to the outputs (`doc/numerics.md`)
* Golden harness: `run_golden.py stats-record` / `stats-compare` record the event counters of every case for
  two builds and compare them exactly (gate of the F-15 engine refactors)
* `specs --progress[=seconds]`: a periodic progress line on stderr (wall time, simulated time, delta cycles),
  without effect on the simulation
* Expert option (F-15, `doc/numerics.md`): an event cap (`.options maxevents=N`, `--max-events N`: a `.tran`
  ends there with a warning, an `.op`/`.dc` stops with an error). Default unchanged (no cap)
* A plain `PROBE` (also the automatic `PROBE{<net>}` probes and the `.save` targets) traces one wavelength of a
  net that carries several: it latches the smallest one per analysis point (each `.op`/`.dc` point; in a `.dc`
  that sweeps a source's wavelength or frequency, the swept wavelength when the net carries it; in a `.tran`,
  the smallest at the start of the transient, or of its first time step that brings any) and ignores the others,
  with one warning on stderr per probe (one line for all the automatic probes). Before, it traced every event, so
  it showed one of the wavelengths of each time step, in an unspecified order. For a given
  wavelength, use MLPROBE. Single-wavelength nets, `.dc` wavelength sweeps and the `-t` testbenches are
  unchanged (`doc/netlist.md`)

### Changed

* `-Dnative` is off by default: builds are portable (no `-march=native`) and their results no longer depend on the
  build machine's CPU. `-Dnative=true` (pixi: `pixi run build-native`, toolchains `gcc-native`/`clang-native`) gives
  host-tuned binaries, whose results differ in the last bits (B-183). The golden baseline is re-recorded without
  `-march=native`: 28 cases change in the last one or two digits of floats (within 1e-12 relative)
* Signal listings (`.op`/`.dc` net listings, source and testbench prints) no longer start with the `[NNNN]`
  debug id of the signal: the id is a build option, `-Dsignal_ids=true`, off by default (F-15). Values unchanged
* Faster engine (F-15): emission gate on squared norms, waveguide transmissions and delays computed once per
  wavelength, the output ports' event notifications without an allocation per event, and the output ports and
  stateless device reactions (waveguide, coupler, splitter, merger, crossing, phase shifter) run as SystemC
  methods instead of threads. CROW benchmark (30 rings, no tracing): about 2.5x less CPU time than v0.2.0.
  Results are the same bit for bit, except that on a net that carries several wavelengths, a plain `PROBE` (the
  last write of each time step) could show another of the wavelengths written in a step; the per-wavelength
  values (`MLPROBE`) were unchanged in every circuit checked. The PROBE wavelength latch (New) removes this
  dependency on the kernel order
* A `PROBE` always traces its wavelength (the `wavelength` signal says which wavelength the latch chose, and is
  the x-axis of a transmission sweep), and so do the automatic `PROBE{<net>}` probes. A 4th positional argument and
  `wl=`/`wavelength=` are still accepted: a true value changes nothing, a false one (`wl=0`, `probe1 out 0 1 0 0`)
  is ignored, with one warning on stderr for all such probes. `.save QTY(net)` traces exactly the quantity asked
  for (`WL(net)` adds the wavelength). C++: the `Probe` constructor keeps its `trace_wavelength` parameter
  (`doc/netlist.md`)

### Fixed

* B-184: at the start of a `.tran`, an MLPROBE on a net that carries several wavelengths traces the
  operating-point value of every listed wavelength. Before, only the wavelength written last to the net during
  the operating point; the others read 0 until written again (never, with CW sources)

### Removed

* Unused code (decision 2026-10-23): the delta-write mode of the optical output ports and the single-wavelength
  generic transmission devices (`GenericTransmissionDevice`, `Generic2x2Coupler`, `GenericWaveguide`), which no
  netlist or testbench could reach. A generic N-port device stays planned (scout F-03), written from scratch

## [0.2.0] - 2026-10-21

### New

* CI: GitHub Actions and GitLab CI on Ubuntu 24.04 with Meson (gcc and clang, golden check);
  `install.sh` rewritten for Meson (plain setup, compile, test)
* The unit tests compile again (`src/test/utils.h` placeholder `static_assert` removed)
* Golden-output regression harness (`tests/golden/`, `meson test --suite golden`): 211 `specs` cases
  (example netlists, CLI options, every testbench, bad-input errors, regressions) plus the gtest suite
  * Stricter by default (sprint 2026-10b, B-126..B-129): line order is checked (except per-case
    `unordered_streams`; `--lenient-order`), NEW/MISSING cases and an unrequested gtest skip fail
    (`--allow-new`, `--no-unit-tests`), an `--only` that matches nothing exits 2, floats printed with
    fewer than 5 significant digits compare exactly, and printed OpticalSignal IDs are normalised to
    first-appearance ranks (`[#k]`). `--strict`/`--strict-order` are accepted as no-ops
* Paper reference (ECOC 2023, hal-04302200) in the README and `doc/papers/`
* CI: the ASan+UBSan job is blocking; LeakSanitizer suppressions (`tests/sanitizers/lsan.supp`) cover only
  parse-error exits and SystemC thread stacks, and are applied by `meson test` in sanitizer builds
* `specs --version` prints the version from the git tags (upstream 0865a47, ported from CMake to
  Meson: `utils/git-version.sh`, `src/utils/meson.build`)
* `doc/netlist.md`: nets, positional arguments and keywords of COUPLER, CROSSING, CWSRC, VLSRC, EVLSRC, PCMCELL
  and PDET, and how probes trace in `.tran` (complements the cheatsheet)
* `doc/numerics.md`: tolerances (the emission rule max(abstol, reltol·|last|), the snap to 0), the error of
  resonators versus reltol and how to choose it, time resolution and delay rounding (B-021, B-124, B-014)
* `.options snapfactor=<x>`: emitted fields with |E| < snapfactor·abstol are emitted as 0 (default 10, as
  before; 0 disables the snap) (B-021)
* Zero tolerances: `abstol` and `reltol` may be 0 (`.options`, `--abstol`, `--reltol`; negative values stay
  errors), e.g. to compare with a fixed-step discrete model; with both at 0 nothing is gated or snapped and
  one warning says that feedback circuits may not quiesce and `.op`/`.dc` may stop at `maxiter`
  (`doc/numerics.md`) (B-018, B-153)
* Reproducible runs (F-01, `doc/reproducibility.md`):
  * `--seed N` (0 to 2^64 − 1, also for `-t` testbenches) and `.options seed=N` (an integer literal 0 to
    2^63 − 1, exponent form 0 to 2^53, B-180; `--seed` wins):
    one master seed for every random draw, **default 0**, so runs are reproducible by default. Each device
    draws from its own stream, derived from the seed and its hierarchical name (adding or reordering other
    devices does not change it); the streams are the same with every standard library (`std::mt19937_64`,
    `std::seed_seq`, SPECS's own uniform/normal). Future `gauss()`/Monte-Carlo draws will use the same seed
  * PDET `darkfield` keyword: the 1e-20 √W dark-field injection on a detector whose total field is exactly 0
    stays on by default and is now seeded; `darkfield=0` turns it off (the readout is then exactly the dark
    current) (B-056)
  * Run manifest: every run that writes the default trace also writes `<trace>.manifest.json` (SPECS and
    SystemC versions, seed, command line, SHA-256 of every netlist and `.include` file, trace file);
    `--print-manifest` prints it on stdout
* `.op`/`.dc` convergence (F-28, `doc/numerics.md`): the delta-cycle fixed point of an operating point or a
  sweep point stops after `maxiter` iterations (delta cycles; default 1000000, `.options maxiter=N`,
  `--maxiter N`, 0: no limit) with an error naming the analysis (and the sweep point), a residual report (the
  output ports still changing, their last change and threshold) and exit status 1, instead of running for
  hours (weakly coupled or lossless loops). Converging circuits are not affected
* Netlist pack (F-12, SPICE semantics; syntax reference at the top of `doc/netlist.md`):
  * SI scale suffixes on numbers: `f p n u m k meg g t` (`1.55u`, `10p`, `2meg`), also in expressions,
    `.param`, `.options` and analyses; case-insensitive, so `M` is milli and `MEG` mega. `1.55u` is exactly
    `1.55e-6`. Trailing unit letters (`1.55um`) are not accepted (B-160)
  * `.ends NAME`, checked against the `.subckt` name (case-insensitive) (B-169)
  * `.dc WL(cwsrc1) start stop step` (also `P(...)`, `F(...)` and the long forms), the same sweep as
    `.dc /cwsrc1/WL ...`, as written in the paper and the cheatsheet
  * `--param name=value` (repeatable) overrides the netlist's `.param`/`.assign` of that name; the value is
    parsed like a `.param` value
  * `.include` resolves a relative path against the directory of the including file first (for a `.subckt`
    body: the file that defines it), then the working directory as before; a warning when both exist and
    differ (B-143)
  * The pre-release netlists `double_ring.cir`, `triple_ring.cir`, `paul_simple_ring.cir` moved to
    `circuits/legacy/` (they do not parse; kept for reference) (B-133)
* Outputs and tracing (F-05, `doc/netlist.md`):
  * `.save net QTY(net) ...` traces the listed optical nets (the paper's `.save P(out) P(drop)` now parses): a net
    alone traces power, modulus, phase and wavelength, `P(...)`, `E(...)`, `PHI(...)`, `WL(...)` one quantity
    each; the probes are named `PROBE{<net>}` like the automatic ones. A netlist with `.save` traces only its
    targets and probes
  * `--no-trace`: no trace file and no run manifest file (as `-o ""`), for runs that only need stdout or for timing
  * `--format vcd|csv|npz` (repeatable; default `vcd`): tables of the probe samples, `<output>.csv` and NumPy
    `<output>.npz` (dependency-free writer), one row per time step, probe and wavelength, with time, probe,
    wavelength, power and complex field, exact to the last bit (the VCD prints 16 digits). Column layout in
    `doc/netlist.md` ("Tracing and output files"). The golden harness decodes `.npz` outputs to text
  * An MLPROBE without a VCD (`-o ""`, `--no-trace`, `--format csv`) no longer reports every sample of its listed
    wavelengths as an unsupported signal

### Changed

Changes that may need edits to build scripts, netlists or post-processing. Bug fixes that change results are
under Fixed, and `--mon` under Removed.

* Switched to the **Meson** build system; it replaces both CMake and the `Makefile`
  * `meson setup build && meson compile -C build` (release build by default; CMake built without
    optimisation unless a build type was given). Options: `native`, `old_tb`, `spx`, `tests`, `golden`, `docs`
  * SystemC 3.0.2, taywee/args 6.4.7 and googletest come from wraps in `subprojects/` (release tarballs
    with sha256; the git submodules and `thirdparty/` are gone, so no `--recursive` clone): `meson setup`
    downloads them and builds SystemC as C++20 with its CMake build (offline: `meson subprojects download`
    first). An installed SystemC is used only on request: `-Dforce_fallback_for=` (falls back to the wrap)
    or `--wrap-mode=nofallback` (errors), found via pkg-config or its CMake package (`-Dcmake_prefix_path`),
    and ignored with a warning if built with another C++ standard. `meson install` also installs the
    subproject's `libsystemc.so` into `<prefix>/<libdir>`, where the installed `specs` finds it (rpath)
  * `meson test` runs the unit tests, the testbench smoke tests (former `make test`) and the golden-output
    regression check
  * `make lib` → `meson compile -C build spx` (`libspx.so`, as built by CMake; the Makefile's `libspecs.so`
    is gone); `make doc` → `ninja -C build doc` (output in `build/doc/html`); `make compiledb` → automatic
    `build/compile_commands.json`; `make waves/newwaves/cleantraces/cleanoldtraces/todos` → run targets
    of the same names; coverage → `-Db_coverage=true` and `ninja coverage-html`
  * Removed: `make format` (most files don't follow `.clang-format`; don't run `ninja clang-format`
    either), `make rc` (its netlist didn't exist), `config.mk`, `.config.gitlab-ci.mk`
  * `meson install` installs `specs` (and `libspx.so` with `-Dspx=true`)
  * Asserts stay enabled by default (`b_ndebug=false`); `-Db_ndebug=true` matches the old CMake Release
* SystemC 3.0.2 (built without the copyright banner) and taywee/args 6.4.7
* `.dc` includes the stop value when (stop - start)/step is an integer (SPICE):
  `circuits/add_drop.cir` sweeps 31 points, 1535 to 1565 nm, instead of 30; the other points keep their
  values (still accumulated, B-009), and the announced point count is the swept one (F-12)
* A netlist with probes (PROBE, MLPROBE, also inside a subcircuit) traces only them: the automatic `PROBE{<net>}`
  probes of every optical net are added only when the netlist has no probe, or with an explicit
  `.options traceall=1` (`.options traceall=0` still disables them). Simulation results and the probed signals are
  unchanged; the VCD loses the automatic probes and `- trace all optical nets:` prints 0 (F-05)
* The optical field is labelled √W, not V/m, in the signals printed on stdout (B-023)
* PCMCELL requires `tc` and `ta` (a cell without them was silently opaque: Tc = Ta = 0); the C++ PCM
  constructor defaults match the header (Tc = 0, Ta = 1) and the OCTANE classes take Tc/Ta as required
  parameters (B-051)
* Positional element arguments: CWSRC `wl power phi` (were shifted by one: `cwsrc1 a 1.55e-6 1e-3` gave 0 W
  at 1 mm), VLSRC/EVLSRC `file` (was ignored), PCMCELL `emelt n tc ta` (Debug builds aborted); too many
  positional arguments is a clean error (B-052)

### Fixed

Details and repros in `backlog/` (one commit per item, see `backlog/FIXES-REPORT.md`).

* Undefined behaviour on every netlist parse: `Variable` copied uninitialised members (found by gcc's UBSan; B-181)
* Crashes: bidirectional active MZI elaboration (B-031), `specs -t mesh` segfault (B-047), `-o ""` (B-006),
  `X1` without a subcircuit name (B-149), `--vci` on CROW (B-036), output-port error paths (B-020)
* Bad input now gives a clean error instead of an assert/core dump: positional-argument and config checks
  (B-148, B-018), bare `.dc` (B-008), bad `.options` values (B-153), out-of-range coupler `k`/`kp`/`t` and
  splitter ratio (B-034), stray `.op`/`.dc` arguments (B-165), `.ic` (B-161), unwritable `--json` path (B-017);
  file-open errors name the file (B-064)
* Parser: `.include` inside `.subckt` (B-150), files opened by `.include` are closed (B-156), memory leaks (B-170)
* Integer literals are 64-bit (0 to 9223372036854775807) and a larger one is a parse error with its file and line;
  they were clamped to 2147483647 silently (`.options seed=3000000000` was seed 2147483647, net `3000000000` was
  `N2147483647`). `.options seed`/`maxiter` take integer literals exactly; PCMCELL `n`/`k` beyond the int range
  are an error (B-180)
* `--nrings_crow` sets the number of CROW rings (B-131); CROW children are prefixed with the instance name (B-072)
* Detector readouts start at 0 instead of uninitialised memory (B-054); BitstreamSource saturation message (B-044)
* JSON export writes the writers count, not the readers count (B-154)
* `circuits/subckt_test.cir` parses again (B-132)
* pyspecs: works with pandas 3 and detects `.dc` sweeps (B-076, B-077); default output name, directory joins,
  error reporting, dependency manifest, notebook binary path, value shortening, bitstream helpers
  (B-085..B-089, B-091, B-092, B-099)
* Docs: Doxygen config/mainpage and stale `@param`/`@sa` references (B-083, B-100, B-102); KiCad symbol
  templates (B-082); `.gitignore` no longer hides new `circuits/*.cir` (B-114); flex/bison warnings (B-109, B-177)
* gtest fork harness: pipe deadlock above 64 KiB, escaping assertions/exceptions (B-125); angle check (B-123)
* Multi-wavelength `.op`/`.dc` (and the OP phase of `.tran`): a stale partial value of one wavelength could be
  emitted as the converged result when another wavelength shared the output port (B-001)
* Bidirectional (`&`) nets: one slot convention (every writer takes the next free slot), so light no longer
  depends on the netlist line order: a source or other unidirectional writer listed before the bidirectional
  device no longer loses its light, two sources on one net run (was E115), a net upgraded to bidirectional keeps
  its reader on the device's output, and crossing outputs on bidirectional nets propagate (B-144, B-145, B-147)
* Subcircuit instances that share a port net now behave like the flat netlist (was E115 or last write wins, and
  no net type check) (B-146)
* PCMCELL rejects `n` <= 0 and an initial state `k` outside [0, n] (B-060; the PCM model itself is unchanged)
* COUPLER: `k` is documented as the field cross-coupling (P_cross = k²), `kp` the power ratio and `t` the through
  field coefficient (`doc/netlist.md`; the paper's κ = 0.15 is a power ratio, i.e. `kp`); the misleading
  per-instance "Warning: coupling power for DC currently set to field." is gone (B-037)
* `-m sampled-time` is rejected with an error (it was not implemented and silently dropped every optical write);
  an explicit `-m` now takes priority over the default mode of the netlist's `.op`/`.dc`/`.tran` (it was
  silently overridden), e.g. `-m fd` runs a `.tran` in frequency domain (B-002, B-004)
* CROSSING is passive: the crosstalk is in quadrature (j·x, as in the coupler), `att` is the excess loss on top
  of the crosstalk (t = 10^(-att/20)·sqrt(1 − x²)), and the sign of `xtalk` is ignored (`xtalk=20` is a 20 dB
  isolation, it was a 100× gain). Two coherent inputs gave more power out than in (2 W → 2.42 W) (B-032)
* CROSSING keeps one memory per wavelength and input, so two wavelengths on its two inputs both come out (one was
  dropped, with "Attempted to sum signals of different wavelengths" in `.tran`) (B-033), and it is
  bidirectional when one of its nets is: light entering an output comes back out of the inputs (B-159).
  Documented in `doc/netlist.md`
* PDET drives its electrical output net (the write was commented out, so a PSHIFT or MZI driven by a photodetector
  saw 0) (B-049), and its readout is event-driven: it is written on input events and when the detector is
  enabled (in `.tran`, right after the operating point); it is sampled every `ts` only while two or more
  wavelengths beat, and those samples are written only when they change by more than max(abstol, reltol·|last|).
  An open-ended `.tran` with two or more detectors now ends (it never did), and a constant input gives one VCD
  record instead of one per sample (B-050). Detectors are enabled by default for C++ and testbench users;
  `detector_tb` enables them and its self-checks pass. Documented in `doc/netlist.md` (PDET)
* `.tran`: PROBE, MLPROBE and the `traceall` automatic probes trace their net's operating-point value from t = 0
  (they read 0 until the net's next event, so a CW path stayed at 0 in the VCD for the whole transient). Each
  probe samples its net when it is enabled at the start of the transient, after the operating point and any
  initial conditions (B-048; `doc/netlist.md`)
* Delays that the time resolution rounds by more than 10 % (e.g. a 53 fs ring round trip run at 100 fs: +87 %), or
  to 0 (the loop then collapses into one time step), are reported on stderr once per waveguide in `.tran` and
  testbenches, with the `.options timescale` to use; they were rounded silently and the check meant to catch it
  could never fire (B-014)
* A waveguide written in the netlist (also inside a `.subckt`) whose non-zero delay rounds to 0 at the time
  resolution is an error (exit 1, before the simulation, with the `.options timescale` to use); waveguides that
  devices create internally (MZI arms, ...) keep the warning (B-014)
* Output order no longer depends on heap addresses: the operating-point listing, the `--vci` dump, the
  automatic probes and VCD declarations, `init()` calls and the enable order of the analyses follow
  hierarchical names (they changed with the compiler, build type or a sanitizer), and elements are created in
  netlist order (B-016, B-176)
* Photodetector noise (`nb=0`) and the dark-field value were seeded from `std::random_device` (twice), so they
  differed between runs; they now come from the seeded per-device streams (B-055, B-056)

### Removed

* `--mon` and the `TimeMonitor` device: it shifted simulated time, stopped after initialisation in netlist runs
  and called `exit(1)` on its first report (B-058; progress reporting is planned separately, scout F-15)
* Dead code (see `backlog/cleanup/REPORT.md`): `do_circuit`/`raphael_main`, the unused `Ring`,
  `PowerProbe` and `PhaseProbe` devices, dead output-port and parser code, the duplicate
  `utils/draw_graph_from_json.py`, leftover debug prints on stdout

## v0.1.0

* Initial release