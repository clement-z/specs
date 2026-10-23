# Golden-output regression harness

`run_golden.py` runs a fixed set of `specs` invocations (netlists, CLI
variants, built-in testbenches) plus, optionally, the GoogleTest binary. It
normalises their outputs and stores them as a baseline, or compares a new build
against the stored baseline. It doesn't depend on the build system: it only
needs the path of a built `specs` (and `unit_tests`) binary. Use it to check that
a build-system migration, a bug fix or a cleanup didn't change what the
simulator produces.

Requirements: Python >= 3.9, standard library only. Always run it isolated:
`python3 -I tests/golden/run_golden.py ...`.

## Usage

```sh
# Compare a build against the committed baseline (exit 0 = pass, 1 = fail,
# 2 = --only selects nothing). Give --unit-tests, or --no-unit-tests to skip
# the gtest case explicitly (without either, it fails as MISSING).
python3 -I tests/golden/run_golden.py check \
    --specs <builddir>/specs --unit-tests <builddir>/src/test/unit_tests \
    --baseline tests/golden/baseline [-j 4] [--keep /tmp/candidate] [--report report.json]

# (Re-)record a baseline; --repeat N also re-runs everything N-1 times and
# reports any case whose output is not reproducible
python3 -I tests/golden/run_golden.py record \
    --specs <builddir>/specs [--unit-tests ...] --out tests/golden/baseline --repeat 3 --note "why"

# Compare two recorded directories (e.g. two `record` runs, or `check --keep` output)
python3 -I tests/golden/run_golden.py compare --baseline DIR_A --candidate DIR_B

# Show the case list
python3 -I tests/golden/run_golden.py list

# Event counters of every case (F-15): record them for two builds, then compare
python3 -I tests/golden/run_golden.py stats-record --specs <before>/specs --out /tmp/stats-before -j 8
python3 -I tests/golden/run_golden.py stats-record --specs <after>/specs --out /tmp/stats-after -j 8
python3 -I tests/golden/run_golden.py stats-compare --baseline /tmp/stats-before --candidate /tmp/stats-after
```

### Event counters (`stats-record`, `stats-compare`)

`stats-record` runs every case (or `--only`) with `--stats=ports,devices --stats-file
golden-case.stats.json` appended to its arguments and stores, per case, the exit status and the run
statistics (`specs --stats`, `doc/numerics.md`) without `wall_time_s`, in `<out>/cases/<id>.json`. Cases
whose run writes no statistics (parse errors, `-n`, `--help`, ...) store `null`; the `-t` testbenches
write theirs. Outputs are not compared: the appended options only add the file. `stats-compare` compares two
such directories **exactly** (integers, the doubles bit for bit, the exit status); the rows of `ports` and
`devices` are matched by port name and device type, so a difference reads
`ports.ROOT/WG1.out_delayed_writer.processed: 12 -> 14`. Exit 1 if a case differs or is missing on one
side. `--ignore REGEX` (repeatable) skips the counters whose key path matches (`--ignore pending_`),
`--report` writes the result as JSON. Two records of the same build compare equal (checked 2026-10-22 on all
226 cases, 134 of them with statistics).

Use: the F-15 engine refactors must keep the counters of every case identical (D2) or explain each change
(D3, decision Q1): record with the build before the change and with the build after it (same compiler and
options), then `stats-compare`.

With Meson, `check` (default tolerances, `-j 4`, `--unit-tests` when the unit
tests are built, `--no-unit-tests` otherwise) is also registered as a test
(`tests/golden/meson.build`), next to `golden-harness`, the unit tests of
`run_golden.py` itself (`test_run_golden.py`):

```sh
meson test -C <builddir> --suite golden --print-errorlogs
```

Useful options (all sub-commands that compare):

| option | default | meaning |
|---|---|---|
| `--rtol`, `--atol` | `1e-9`, `1e-12` | float tolerance: `|a-b| <= atol + rtol*max(|a|,|b|)` |
| `--print-slack` | `1` | extra tolerance for floats in **text** (stdout, .txt) with 5 or more significant digits: N units of the last printed digit (`0.707107` may become `0.707106` or `0.707108`). `0` disables it. Shorter floats get no slack (B-128) |
| `--lenient-order` | off | accept stdout/stderr lines that differ only in **order** in every case (reported as a note). By default only the streams a case lists in `unordered_streams` may be reordered (see "Determinism findings"). `--strict-order` is accepted as a no-op |
| `--allow-new` | off | NEW cases (in `cases.json`, not in the baseline) are reported but don't fail, e.g. while adding a case before recording it. MISSING cases (in the baseline, no result) always fail. `--strict` is accepted as a no-op |
| `--no-unit-tests` | off | (`check`, `compare`) skip the gtest case explicitly (reported as SKIPPED). Without it, a selected gtest case with no result fails as MISSING, also when the baseline has none |
| `--only REGEX` | all | restrict to matching case ids (`gtest/unit_tests` included); exit 2 when it matches nothing |
| `--max-file-bytes` | `1e6` | larger outputs are stored gzipped (deterministic, lossless). If still too large, only a lossy summary is kept: per-signal min/max/last for VCDs, line count for text |

Re-record (and commit) the baseline when a change *intentionally* alters the
output, e.g. a bug fix. Put the reason in `--note`; it is stored in
`baseline/manifest.json` (for a partial re-record with `--only`, it is appended to
the manifest's `rerecords` list together with the regex and the date). The
gtest case is only (re-)recorded when the regex matches `gtest/unit_tests`.
To add a case: add it to `cases.json`, run `check --allow-new` (the new case is
reported as NEW, everything else must pass), then `record --only '^<id>$'`.

## What a case is

`cases.json` lists the cases. Each one runs `specs <args>` (argv[0] is always
`specs`) in a **fresh temporary working directory**. Before the run, `circuits/`
is copied in (netlists use `.include circuits/...`: not found next to the including
file, it resolves relative to the cwd, F-12) and an empty `traces/` is created. Testbenches and the default trace
file write to `traces/<name>.vcd` relative to the cwd, with the run manifest
(F-01: version, seed, command line, netlist file hashes) next to it as
`traces/<name>.manifest.json`. If `traces/` is missing, specs fails (exit 1) with
`Error: cannot open run manifest file` (before F-01: SystemC `E701 cannot open
trace file`).

Netlists that only exist to test a fix (error paths, regressions) live in
`tests/golden/netlists/`, not in `circuits/` (the examples). Their cases override
`setup` with `{"copy": ["circuits", "tests/golden/netlists"], "mkdir": ["traces"]}`
and run `-f tests/golden/netlists/<name>.cir`.

For every case the harness records:

* exit status: an integer, `signal:SIGSEGV`, or `timeout` (default 120 s per case);
* stdout and stderr, normalised:
  * the SystemC banner is removed (also suppressed via `SC_COPYRIGHT_MESSAGE=DISABLE`);
  * `Total runtime: X ms` becomes `<DURATION>`;
  * absolute paths become `<CWD>`, `<BUILD>`, `<REPO>`, `<HOME>`;
  * SystemC `In file: /…/x.cpp:123` becomes `In file: x.cpp`;
  * `-nan` becomes `nan`, and trailing blanks are stripped;
  * the `"specs_version": "..."` line printed by `specs --print-manifest` becomes
    `"specs_version": "<VERSION>"` (the version changes with every commit);
  * printed OpticalSignal IDs (`[0042]  @ 1550 nm ...`, a global counter that every
    copy also bumps) become `[#k]`, k = rank of the ID's first appearance in that
    stream or file (B-129). Prints of the same signal keep the same rank; adding or
    removing an `OpticalSignal` copy no longer changes the output. Ranks follow line
    order, so streams compared as a multiset of lines (`unordered_streams`,
    `--lenient-order`) compare the lines with their IDs masked, plus the ID structure:
    for each ID, the lines it appears in (IDs and numbers masked);
* every file created or modified in the working dir (`circuits/` copies excluded):
  * VCD: `$date` and `$version` blocks are stripped;
  * JSON (the `--json` export): stored verbatim and compared structurally;
  * run manifests (`*.manifest.json`, F-01): JSON as well, but the value of
    `"specs_version"` becomes `"<VERSION>"` and absolute paths are mapped as in
    stdout (`argv[0]` is absolute in `rlimit_nofile` cases); the rest is verbatim, so
    the SHA-256 of the netlist files is compared: editing a netlist, even a comment,
    changes the manifest of every case that reads it;
  * NumPy `.npz` (`specs` writes them): the ZIP is read entry by entry, in archive
    order, and each `.npy` array is decoded (`<f8`, `<i4`, `<i8`, `<U<n>`) into text, one
    value per line under a `npz <entry>: descr=... shape=...` line (floats with `repr`).
    An undecodable file or entry becomes a `npz...: cannot decode: <reason>` line. The
    baseline stores this text, not the binary; the SHA-256 and size of the raw bytes are
    kept in `case.json` but never compared (the last bits of floats differ between compilers);
  * anything else: compared as text, or as bytes if binary.

The environment is inherited, with `LC_ALL=C LANG=C TZ=UTC SC_COPYRIGHT_MESSAGE=DISABLE`.

### Comparison rules

* **Text** (stdout/stderr/.txt) is compared line by line. Numbers embedded in
  text (not glued to identifiers such as `PS1`, `uut2`) use the float tolerance
  plus the print slack. Integers must match exactly. Floats with fewer than 5
  significant digits on both sides (`PRINT_SLACK_MIN_DIGITS`: tolerances and
  config echoes such as `1e-08` or `0.0001`, parameters, short fixed-point
  values such as `0.000317`) get neither the print slack nor `--atol`, only
  `--rtol`: with the default rtol they compare exactly, so `abstol: 1e-12` ->
  `2e-12` fails (B-128). The threshold keeps the slack for the long computed
  tokens where another compiler may flip the last digit, at most 1e-4 relative;
  gcc, clang, gcc `-O0` and clang ASan builds need no slack at all (2026-10-21).
* **VCD** files are parsed. Signals are matched by hierarchical name
  (`SystemC.PROBE{ROOT/out}.power`), not by id code or declaration order.
  Every signal is treated as a piecewise-constant function of time. Its value
  is compared, with tolerance, at every timestamp where either file has a
  record. This matters because SystemC only writes a record when a value
  changes bit-exactly, so ulp-level differences (`-O0` vs `-O3`, GCC vs clang)
  add or drop redundant records without changing the waveform. Timescale and
  the set of declared variables (with type and width) must match.
* **JSON** is compared structurally, with numbers using the tolerance. In run statistics
  (`specs --stats`, files `*.stats.json` and `--print-stats` on stdout) the value of `wall_time_s` is
  masked (`"<WALL_TIME>"`); name a `--stats-file` of a case `*.stats.json` so that it is masked too.
* **NPZ** is compared as its decoded text, line by line, as **text** is (float
  tolerance and print slack apply; a 1-ulp difference passes).
* **gtest** results are compared per test name (`Suite.Test`: PASS / FAIL /
  SKIP / NOT_RUN); timings are ignored:
  * PASS -> FAIL and missing tests fail;
  * FAIL -> PASS and new tests are reported as notes ("improvement;
    re-record");
  * the gtest case is selected when `--only` is not given or matches
    `gtest/unit_tests`; it then needs `--unit-tests`, otherwise it fails as
    MISSING, unless `--no-unit-tests` skips it explicitly (SKIPPED).
* Per-case keys in `cases.json`:
  * `vcd_ignore_values`: regexes on signal names excluded from the VCD comparison;
  * `unordered_streams`: streams compared as a multiset of lines (a reorder is
    reported as a note); every other stream must keep its line order;
  * `ignore_streams`, `ignore_files` (globs), `timeout`;
  * `rlimit_nofile`: soft open-files limit of the `specs` process (set with `ulimit -S -n`);
  * `expect` and `note` are documentation only.

## Case list (244 `specs` cases + 1 gtest case)

| group | cases | what it covers |
|---|---|---|
| `cir/*` (9) | every `circuits/*.cir` and `circuits/legacy/*.cir`, run with `-f` | parser + elaboration + OP/DC/TRAN analysis, default trace `traces/delete_me.vcd` |
| `cir-json/*` (4) | `-n --json flat.json -f` on the 4 netlists that parse | flattened-netlist JSON export, dry run |
| `opt/*` (6) | `-o`, `--abstol/--reltol/-R`, `-m sampled-time` (rejected, B-002), `-m fd`, two `-f` files, `--vci` | CLI option handling (options become a `.options` footer) |
| `tb/*` (20) | every name in `tb_map` (`src/tb/alltestbenches.cpp`, `specs -p`): 13 testbench functions, aliases included; plus `tb/ac_add_drop-fd` (`-m fd`) | legacy device testbenches (`BUILD_OLD_TB`) |
| `cli/*` (6) | `-h`, no args, `-p`, unknown testbench, missing netlist, bad `-m` | CLI and error paths |
| `err/bNNN-*`, `err/fNN-*` (86) | bad inputs from the 2026-10, 2026-10b and 2026-10c fix phases and features (`tests/golden/netlists/` or CLI options), and the `.op`/`.dc` fixed points that do not converge (F-28) | each must be rejected with a clean message and exit 1 (was a crash, abort, hang or silent acceptance); the backlog or feature ID is in the name |
| `regress/bNNN-*`, `regress/fNN-*` (110) | regression cases of the 2026-10, 2026-10b and 2026-10c fix phases and features | behaviour that a fix made work (bidirectional MZI and nets, `--vci` on CROW, many includes, multi-wavelength `.op`, positional arguments, `-m` with an analysis, passive and bidirectional crossings, event-driven photodetectors, probes tracing the operating point from t = 0 in `.tran`, `snapfactor`, delay-rounding warnings, resonator error vs reltol, seeds and per-device random streams, the PDET dark field, the run manifest, the `.op`/`.dc` iteration cap, SI suffixes, `.ends NAME`, `.dc WL(src)`, `--param`, `.include` relative to the including file, tracing only the probes of a netlist that has some, `.save`, `--no-trace`, CSV and `.npz` tables, subcircuit instances sharing a net, 64-bit integer literals, zero tolerances, run statistics, the event cap `maxevents`, value-list source prints only with `--vci`, per-wavelength values that must not depend on the order of evaluation within a time step, ...); hand-computed expected values are in the netlist headers |
| `gtest/unit_tests` | `unit_tests --gtest_output=json:…` | GoogleTest suite (only with `--unit-tests`) |

Cases that are **expected to fail**. They are kept so that any behaviour change
is visible; re-record when it is fixed on purpose.

* `cir/double_ring`, `cir/triple_ring`, `cir/paul_simple_ring`: parse error
  (pre-release netlist syntax, `one 0` at line 4), exit 1. The netlists are kept for reference in
  `circuits/legacy/` (moved there by F-12; the `cir/*` case ids are unchanged).
* `cir/paul_3ring_puf`: the VLSRC values file
  `paul_puf_bitstream_158b_3wl.csv` is not in the repo, exit 1.
* `cli/unknown-testbench`, `cli/missing-netlist`, `cli/bad-mode`: exit 1 by design.
* `opt/mode-sampled-time`: `-m sampled-time` is not implemented and rejected (B-002), exit 1.
* `regress/b144-three-writers`: three writers on one optical net still give SystemC E115 (known limitation, kept so a change is visible), exit 1.
* `err/*`: exit 1 by design. Most of them set `ignore_streams: [stdout]`, so only
  the exit status and the error message are compared.

`record` warns when a `circuits/*.cir` or a `specs -p` testbench has no case,
so new netlists and testbenches get noticed. Add them to `cases.json`, then
record again.

## Baseline contents (`baseline/`, about 3 MB, all files stored in full)

```
baseline/manifest.json              format, date, note (build used, gtest provenance), case ids, partial re-records
baseline/cases/<group>/<name>/case.json    args, exit status, list of output files (+sha256, size)
baseline/cases/<group>/<name>/stdout.txt   normalised
baseline/cases/<group>/<name>/stderr.txt   normalised
baseline/cases/<group>/<name>/files/...    normalised output files (VCD, JSON; an .npz as decoded text in <name>.npz.txt)
baseline/gtest/unit_tests.json      per-test PASS/FAIL
```

The current baseline was recorded on 2026-10-22 (B-183) with the Meson release build and the project
defaults, **without `-march=native`** (`-Dnative=false` is the default since then), using pixi's gcc 15
(`pixi run build gcc`), SystemC 3.0.2 and args 6.4.7 from the wraps. Before that full re-record the baseline
came from a `-march=native` gcc build, whose FMA contractions made it bit-exact only for gcc native builds;
the previous baseline and its partial re-records are in the git history of `baseline/`. Cases re-recorded or
added since then are listed in the `rerecords` list of `manifest.json`. The largest file is about 400 kB, so
nothing is gzipped or summarised.

**gtest provenance:** the gtest baseline is recorded from the in-tree
`unit_tests` (Meson: `<builddir>/src/test/unit_tests`) and re-recorded on its
own with `record --only '^gtest' --unit-tests ...` (the `rerecords` list of
`manifest.json` says when and why). Every test PASSes, `Waveguide.OperatingPoint`
included since the angle check in `src/test/utils.h` was finished upstream
(b7d6fff; B-123). The first
baseline came from an out-of-tree patched copy, because `unit_tests` did not
compile then; it had `Waveguide.OperatingPoint` FAIL.

`tests/golden/.gitignore` re-includes `*.vcd`/`*.json`/`baseline/**`, because
the root `.gitignore` ignores `*.vcd` (and `*.cir`, `specs`, ...) repo-wide.

## Determinism findings

* `record --repeat 3`, and two independent `record` runs compared with
  `--rtol 0 --atol 0 --print-slack 0`, are **bit-identical** for every case
  (except the `tb/det` and `tb/detector` readouts, see below).
* `tb/det`, `tb/detector`: until B-054, `Detector::m_cur_readout` and
  `m_cur_readout_no_interf` (`src/devices/detector.h`) were never
  initialised and were dumped at t=0, for example `r6.92e-310` (stale pointer
  bits). Since B-049/B-050 the testbench's detectors are enabled and write
  their readout, with noise ON. Until F-01 (sprint 2026-10b P4) the noise came
  from `random_device` seeds and the readout values (`READOUT`,
  `uut*.readout*`) were excluded through `vcd_ignore_values`; now each detector
  draws from a stream derived from the master seed (default 0) and its name,
  so the values are reproducible and compared. The testbench still prints
  them with 3 decimals.
* **Name-ordered iteration (F-01, sprint 2026-10b P4).** Until then,
  `sc_get_all_object_by_type<T>()` / `sc_get_all_module_by_type<T>()`
  (`src/utils/sysc_utils.h`) returned `std::set<T*>` and the parser created
  backlog elements from a `std::set<const ParseElement*>`, so the OP listing,
  the `--vci` dump, `init()` calls, auto-probes and VCD declarations, and the
  "Creating ..." lines followed heap addresses and changed between builds
  (B-016, B-176; 14 cases had to declare `unordered_streams`). Now the
  collections are sorted by hierarchical name (`strcmp`) and elements are
  created in netlist order, so every case checks its line order and no case
  declares `unordered_streams`. The one-time re-record was proven to change
  ordering only with `backlog/repro/p4-ordering-proof.py` (same lines with IDs
  masked, same set of (signal, waveform) pairs). `unordered_streams` and
  `--lenient-order` stay available for new nondeterminism.
* **Cross-build check:** the same baseline passes for
  * CMake Release (GCC, `-O3`),
  * CMake Debug (`-O0`),
  * the legacy Makefile build (`-std=c++20 -O2 -g`, bit-identical to Release),
  * a clang Release build;
  * the Meson builds (2026-10, `discover/meson/VALIDATION.md`): GCC default
    and `-Db_ndebug=true` are bit-identical (`--rtol 0 --atol 0
    --print-slack 0`); clang and `-Dnative=false` pass with `--atol 0`.
  * since the 2026-10-22 re-record (B-183), with `--rtol 0 --atol 0 --print-slack 0`: gcc release builds
    (system gcc 11.5, pixi gcc 15.3) are bit-identical; clang (19.1, 21.1) and debug ASan+UBSan builds (gcc
    and clang) differ in 6 cases at one sweep point (at most 1e-14 relative); `-Dnative=true` builds differ
    in 28 cases (at most 1e-12). All pass at the default tolerances and with `--atol 0`.

  The largest numeric deviation was 1.1e-13 relative. All of them also pass
  with `--atol 0`.
* **Tolerance caveat:** with the default `atol=1e-12`, values below about 1e-3
  are effectively compared with an absolute tolerance (VCD, JSON, and text
  floats with 5 or more significant digits). For example, a
  photocurrent of 1e-10 A may move by 1%. Use `--atol 0` (passes for every
  build above) for a tighter check.
* A negative test (a wrapper that perturbs outputs) is caught: a VCD value
  scaled by (1+1e-6), one stdout digit, the exit status, a deleted trace and an
  extra stdout line all make `check` exit 1. A (1+1e-12) scaling passes.
