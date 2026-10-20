# F-01: Reproducible runs (seed, version, deterministic order, run manifest)

value 5 | effort S | risk low (one golden re-record for VCD declaration order) | prerequisites: none | horizon 1

## Problem / user story
*As a PUF or Monte-Carlo user I need two runs of the same netlist to give the same bytes, and a run with noise to
be repeatable from a printed seed.* Today:
- `Detector::init()` seeds its generator from `std::random_device` (`src/devices/detector.cpp:132-136`) and is
  called twice (from `prepareSimulation`, `src/specs.cpp:359-361`, and at thread start, `detector.cpp:15`);
  a 1e-20 random field is injected whenever the total field is zero (`detector.cpp:74-75`), so even
  `noise_bypass=1` runs are not bit-reproducible.
- Every "find all objects" query returns `std::set<T*>` ordered by pointer (`src/utils/sysc_utils.h:73-102`);
  that order drives `init()` calls, auto-probe creation (`src/specs.cpp:282-298`), the OP printout
  (`src/specs.cpp:348-353`) and VCD variable order. The golden harness had to add an "unordered" comparison
  mode because Debug and clang builds reorder output (`tests/golden/README.md`, "Known nondeterminism").
- There is no `--version`; the only provenance in outputs is the traced config block (`src/specs.cpp:380-391`).
- The paper's CROW benchmark draws waveguide lengths from a normal distribution; without seeds the whiskers in
  Fig. 3c cannot be regenerated. Cardoso's 1e5-run GEMM study and Jimenez's 20-instance PUF statistics
  (SOURCES B) are the same need at larger scale; F-02 and F-14 build on this.

## Prior art
- Xyce `-randseed <n>` overrides the netlist seed and **always prints the seed actually used** (Xyce Reference
  Guide 7.9); ngspice `set rndseed=<n>`; gem5 and SystemC itself expose `sc_version()`.
- Research-software expectations: `CITATION.cff`, Zenodo DOI per release, a `--version` that identifies the
  build (SOURCES C, hygiene).

## Proposed design
1. **Seed plumbing.** `--seed N` (CLI, `src/main.cpp:194-285`) is serialised like `--abstol` into the footer
   `.options seed=N` (`src/main.cpp:383-388`, `src/parser/parse_directive.cpp:18-48`) and stored in
   `SPECSConfig` (`src/specs.h:95-117`). If absent, draw one from `random_device`, print `seed used: N` and
   trace it. A `spx::rng_for(name)` helper returns a `std::mt19937_64` seeded with `hash(seed, instance name)`
   so each noisy device gets an independent stream that does not shift when other devices are added.
   `Detector::init()` uses it; the dark-field injection is removed (it exists only to avoid `norm()==0`,
   which `photocurrent = 0` handles) — flag as `needs-human` since it changes values at the 1e-40 W level.
2. **Deterministic iteration.** `sc_get_all_{module,object}_by_type` return a `std::vector<T*>` sorted by
   `name()` (`src/utils/sysc_utils.h:61-66, 73-102`). All callers iterate; none relies on set semantics.
3. **`--version`.** Prints SPECS version (Meson `vcs_tag`), git hash, `sc_version()`, compiler and flags; the
   same string goes into the VCD `$version` block (set before `sc_create_vcd_trace_file`, `src/specs.cpp:367`)
   and the stdout banner.
4. **Run manifest.** Extend the JSON export (`src/parser/parse_tree.cpp:578-660`) or write
   `<trace>.manifest.json` with: version string, command line, resolved options (abstol, reltol, timescale,
   mode, analysis), seed, netlist file hashes, wall time. pyspecs reads it instead of sniffing the VCD
   (`pyspecs/specsparse.py:258-266` compares `analysis_type` to `'b01'`, which SPECS never writes).

## Touch points
`src/main.cpp:194-285, 383-388` (flags, footer) · `src/parser/parse_directive.cpp:18-48` (`seed` option) ·
`src/specs.h:95-117`, `src/specs.cpp:21-39` (config) · `src/devices/detector.cpp:74-75, 132-136` ·
`src/utils/sysc_utils.h:61-102` · `src/specs.cpp:367, 380-391` (VCD header, traced config) ·
`src/parser/parse_tree.cpp:578-660` (JSON) · `pyspecs/specsparse.py:258-266` · `tests/golden/` (re-record,
drop the "reordered" allowance).

## Validation plan
- `run_golden.py record --repeat 3` is bit-identical for all cases including `tb/det` with noise enabled
  (today 4 detector signals are excluded as nondeterministic).
- Two seeds give different noise; the same seed with devices listed in a different netlist order gives the
  same per-detector streams (unit test with two `PDET` elements).
- `check --strict-order` passes across gcc/clang/Debug builds (the ordering workaround can be retired).
- Re-record the baseline once (VCD declaration order changes) with `--note "F-01 name-ordered iteration"`.

## Effort breakdown
Seed plumbing and helper 1 d · iteration order 0.5 d · `--version` + VCD header 0.5 d · manifest + pyspecs 1 d ·
tests and golden re-record 1 d. Total about 4 days.

## Open questions for the maintainer
- Per-instance stream derived from the hierarchical name (robust to netlist edits) or from creation order?
- Keep the 1e-20 dark-field injection as an option, or delete it (numerics, needs-human)?
- Should `--seed` also seed `.param gauss()` draws (F-02)? Proposed: yes, one master seed.
