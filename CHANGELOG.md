# Changelog

## [unreleased]

### New

* Switched to the **Meson** build system; it replaces both CMake and the `Makefile`
  * `meson setup build && meson compile -C build` (release build by default; CMake built without
    optimisation unless a build type was given). Options: `native`, `old_tb`, `spx`, `tests`, `golden`, `docs`
  * SystemC is found via pkg-config, its CMake package (`-Dcmake_prefix_path=...`), or built from the
    `thirdparty/systemc` submodule; googletest comes from a WrapDB wrap (no more configure-time git clone)
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
* CI: GitHub Actions and GitLab CI on Ubuntu 24.04 with Meson (gcc and clang, golden check);
  `install.sh` rewritten for Meson
* The unit tests compile again (`src/test/utils.h` placeholder `static_assert` removed)
* Golden-output regression harness (`tests/golden/`, `meson test --suite golden`): 114 `specs` cases
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
* SystemC 3.0.2 (built without the copyright banner) and taywee/args 6.4.7
* `doc/netlist.md`: nets, positional arguments and keywords of COUPLER, CROSSING, CWSRC, VLSRC, EVLSRC, PCMCELL
  and PDET, and how probes trace in `.tran` (complements the cheatsheet)
* `doc/numerics.md`: tolerances (the emission rule max(abstol, reltol·|last|), the snap to 0), the error of
  resonators versus reltol and how to choose it, time resolution and delay rounding (B-021, B-124, B-014)
* `.options snapfactor=<x>`: emitted fields with |E| < snapfactor·abstol are emitted as 0 (default 10, as
  before; 0 disables the snap) (B-021)
* Reproducible runs (F-01, `doc/reproducibility.md`):
  * `--seed N` (0 to 2^64 − 1, also for `-t` testbenches) and `.options seed=N` (0 to 2^53; `--seed` wins):
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
  * **Changed results:** `.dc` includes the stop value when (stop - start)/step is an integer (SPICE):
    `circuits/add_drop.cir` sweeps 31 points, 1535 to 1565 nm, instead of 30; the other points keep their
    values (still accumulated, B-009), and the announced point count is the swept one
  * `--param name=value` (repeatable) overrides the netlist's `.param`/`.assign` of that name; the value is
    parsed like a `.param` value
  * `.include` resolves a relative path against the directory of the including file first (for a `.subckt`
    body: the file that defines it), then the working directory as before; a warning when both exist and
    differ (B-143)
  * The pre-release netlists `double_ring.cir`, `triple_ring.cir`, `paul_simple_ring.cir` moved to
    `circuits/legacy/` (they do not parse; kept for reference) (B-133)
* Outputs and tracing (F-05, `doc/netlist.md`):
  * **Changed outputs:** a netlist with probes (PROBE, MLPROBE, also inside a subcircuit) traces only them: the
    automatic `PROBE{<net>}` probes of every optical net are added only when the netlist has no probe, or with
    an explicit `.options traceall=1` (`.options traceall=0` still disables them). Simulation results and the
    probed signals are unchanged; the VCD loses the automatic probes and `- trace all optical nets:` prints 0
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
* `--nrings_crow` sets the number of CROW rings (B-131); CROW children are prefixed with the instance name (B-072)
* Detector readouts start at 0 instead of uninitialised memory (B-054); BitstreamSource saturation message (B-044)
* JSON export writes the writers count, not the readers count (B-154)
* The optical field is labelled √W, not V/m (B-023)
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
* Positional element arguments: CWSRC `wl power phi` (were shifted by one: `cwsrc1 a 1.55e-6 1e-3` gave 0 W
  at 1 mm), VLSRC/EVLSRC `file` (was ignored), PCMCELL `emelt n tc ta` (Debug builds aborted); too many
  positional arguments is a clean error (B-052)
* PCMCELL requires `tc` and `ta` (a cell without them was silently opaque: Tc = Ta = 0); the C++ PCM
  constructor defaults match the header (Tc = 0, Ta = 1) and the OCTANE classes take Tc/Ta as required
  parameters (B-051)
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
  could never fire (B-014; an error for delays that round to 0 is pending, see `backlog/B-014-*.md`)

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