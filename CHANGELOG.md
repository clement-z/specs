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
* `specs --version` prints the version from the git tags (upstream 0865a47, ported from CMake to
  Meson: `utils/git-version.sh`, `src/utils/meson.build`)
* SystemC 3.0.2 (built without the copyright banner) and taywee/args 6.4.7
* `doc/netlist.md`: nets, positional arguments and keywords of COUPLER, CWSRC, VLSRC, EVLSRC and PCMCELL
  (complements the cheatsheet)

### Fixed

Details and repros in `backlog/` (one commit per item, see `backlog/FIXES-REPORT.md`).

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

### Removed

* `--mon` and the `TimeMonitor` device: it shifted simulated time, stopped after initialisation in netlist runs
  and called `exit(1)` on its first report (B-058; progress reporting is planned separately, scout F-15)
* Dead code (see `backlog/cleanup/REPORT.md`): `do_circuit`/`raphael_main`, the unused `Ring`,
  `PowerProbe` and `PhaseProbe` devices, dead output-port and parser code, the duplicate
  `utils/draw_graph_from_json.py`, leftover debug prints on stdout

## v0.1.0

* Initial release