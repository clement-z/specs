# P4 report — F-01 reproducible runs (branch `ws/repro`)

Sprint 2026-10b, phase P4, on top of `sprint/2026-10b` @ 3fc4f94 (P1–P3 merged). Prompt:
`discover/orchestration/2026-10b/P4-repro.md`. Everything decided for F-01 is **done**: name-ordered iteration
(B-016) with netlist-ordered element creation (B-176), one master seed with per-device streams (B-055), the
dark-field injection kept as a documented option (B-056), and the run manifest. One extra fix the decision does
not cover (the lexer reads integer literals beyond the int range as 2147483647) is **held back** on
`P4-ready/lexer-int-overflow` for sign-off. Nothing was pushed.

**Default seed: 0.** Without `--seed` or `.options seed`, every run uses seed 0, so runs are reproducible by
default (`SPECSConfig::DEFAULT_SEED`, documented in `doc/reproducibility.md`, `doc/numerics.md`, `--help`).

How the work was split: the orchestrator did the ordering items (1–3) and all golden recording. Two Opus
implementers ran in parallel in `/tmp/P4-wt/rng` (`P4-wip/rng`) and `/tmp/P4-wt/manifest` (`P4-wip/manifest`) and
committed code, tests and netlists only. The orchestrator reviewed both diffs, cherry-picked them (dropping the
manifest's seed placeholder, holding back the lexer commit, adding a guard), resolved the conflicts, re-checked
every golden change with its own scripts, recorded the baseline, and wrote the docs and bookkeeping.

## Items

| # | item | outcome | commit | tests | golden impact |
|---|---|---|---|---|---|
| 1 | B-176 element creation order | done: the parser backlog is a `set<size_t>` of indices into the flattened element list, so elements are created in netlist order, not heap-address order | 7c12a61 (status 1eb823a) | the strict line order of the re-recorded cases; the clang ASan run (item 6) | 9 cases, stdout only: the "Creating/Done creating" lines move (sorted lines identical) in `cir/add_drop`, `cir-json/add_drop`, `opt/mode-fd`, `opt/tol-resolution-overrides`, `opt/multi-file`, `regress/b150-include-in-subckt`, `regress/b004-mode-td-dc`, `err/b017-json-unwritable`. `cir/paul_3ring_puf` (expected error: missing VLSRC file) reaches VLSRC1 two elements earlier: 4 fewer stdout lines, stderr and exit identical. glibc's pointer order was *not* netlist order: subcircuit elements are allocated during `flatten()`, in memory freed by the parser |
| 2 | B-016 name-ordered iteration | done: `sc_get_all_{module,object}(_by_type)` return `std::vector<T *>`, hierarchy walked in creation order then `stable_sort` by `name()` (`strcmp`); this orders the `init()` calls, the OP listing, the `--vci` dump, auto-probes and VCD declarations, trace calls and every enable/disable loop of the analyses | 5c6d742 | gtest `SysCUtils.CollectionsAreNameOrdered`; `backlog/repro/p4-ordering-proof.py` | 56 cases re-recorded, proven ordering-only (below) |
| 3 | drop the `unordered_streams` exceptions | done: none left (P1's 8 + 6 added later); notes cleaned; golden README | 54c5d7d | strict check on 4 builds | none (comparison rules only) |
| 4 | F-01 seeds (B-055, B-056) | done: `SPECSConfig::seed` (default 0) + `seed_explicit`; `--seed N` (uint64, set directly, so it also applies to `-t`, wins over `.options seed`); `.options seed=N` (integer 0..2^53); `spx::Rng`/`spx::rng_for(seed, name)` (`src/utils/rng.*`: `mt19937_64` through `seed_seq` from an injective (seed, name) encoding, own uniform and polar normal); Detector seeded once in `start_of_simulation()` from its hierarchical name, `random_device` and both `init()` calls gone, same draws per readout; `m_dark_field_injection` (default **on**) + PDET keyword `darkfield`; F-02 hook documented in `rng.h` | 083c8be (status 16b7678) | gtest `Rng.*` (known answers, encoding, distributions) and `Detector.*` streams (opposite creation order gives the same streams, seeds, darkfield on/off); 17 golden cases `regress/f01-*`, `err/f01-*` | `tb/det`, `tb/detector`: `vcd_ignore_values` removed, readouts now seeded and compared; rest of their VCDs reordered only (proof script, noise signals declared); stdout unchanged. `cli/help`, `cli/no-args`: the `--seed` lines. gtest |
| 5 | F-01 run manifest | done: `<trace>.manifest.json` written by `prepareSimulation()` right after it opens the default VCD (netlists, `-o`, testbenches): format, `specs_version` (`spx::version::full`), `systemc_version`, seed, `command_line`, `netlist_files` (every `-f` and `.include`d file, also in subckt bodies, with SHA-256), `trace_file`; `--print-manifest` prints it (also with `-n`); portable SHA-256 in `src/utils/sha256.*`; harness masks `specs_version` | 4e3e87d | gtest `Sha256.*` (NIST vectors), `RunManifest.*`; harness tests; golden `regress/f01-manifest`, `regress/f01-print-manifest` | 84 cases gain only `traces/*.manifest.json` (every other output byte-identical, checked file by file); all 85 manifests checked (seed 0, `["specs"] + args`, hashes equal to the repo files); `cli/help`, `cli/no-args`: the `--print-manifest` lines; gtest |
| 6 | verification | done: see "Final results" | — | — | — |
| 7 | docs | done: new `doc/reproducibility.md` (seed, streams, output order, manifest), linked from README, `doc/mainpage.md`, Doxyfile; `doc/netlist.md` (PDET `darkfield`, reproducible noise), `doc/numerics.md` (`.options seed`), golden README, CHANGELOG `[unreleased]`; backlog B-016, B-055, B-056, B-176 status + rows | e05090d, 1eb606c | — | — |

### The ordering re-record is ordering only (B-016, item 2)
`python3 -I backlog/repro/p4-ordering-proof.py <old baseline> tests/golden/baseline` classifies each output of
each case as identical / reordered / DIFFERENT:
- text (stdout, stderr): same multiset of lines with the printed signal IDs masked, and the same ID structure
  (`id_profiles`), exactly like the harness (B-129 ranks follow line order, so a reorder renumbers them);
- VCD: same timescale and the same set of (hierarchical name, type, width, exact waveform) tuples, the waveform
  being the exact list of (time, value string) records; signals matching `--noisy` (the detector readouts) only
  need the same declaration;
- JSON: same parsed value; exit status and the set of output files must match.

Old baseline (1eb823a) vs new: **55 reordered, 77 identical, 1 explained**:
`regress/b144-three-writers` (expected E115, stdout ignored) enables its three CWSRCs in name order now, so they
emit in name order and the E115 text on stdout names CWSRC3/CWSRC2 as the first/second driver (was
CWSRC1/CWSRC3); stderr and exit are identical. That is a process-order effect in an ignored stream, not a value
change. `tb/det`/`tb/detector` differed only in their (then random) noise signals and were left for item 4.

The upgrade_signal assert P2 warned about (`has_writer ^ has_reader`) did **not** fire: with B-176 the creation
order is the netlist order, which is the order the B-145 cases were written for, so the `fix(B-145)` commit was
not needed. `regress/b145-upgrade-reader-case2` and `b145-upgrade-roles` pass on all four builds.

## Held back, needs sign-off: integer literals beyond the int range
The lexer reads every digit string with `stringstream >> int`, which clamps a value beyond the int range to
2147483647 without an error: `.options seed=3000000000` and `seed=99999999999999999999` would silently be the
same seed (also `.assign n=3000000000` is 2147483647, and a net named 3000000000 is `N2147483647`). The fix (lex
such literals as doubles) changes how every out-of-range literal parses, which the F-01 decision does not cover,
so it is not on `ws/repro`:
- on `ws/repro`, `.options seed` rejects the integer literal 2147483647 with a hint ("write the seed in exponent
  form (e.g. 3e9) or use specs --seed"); cases `err/f01-options-seed-huge-integer`, `err/f01-options-seed-int-max`;
  `regress/f01-options-seed-large` uses `seed=3e9` (identical VCD to `--seed 3000000000`);
- **`P4-ready/lexer-int-overflow` @ 7c49a98** (one commit on top of 2353e42, based on the implementer's d6e81d4):
  the lexer fix, the guard removed, `err/f01-options-seed-int-max` replaced by
  `regress/f01-options-seed-int-literal` (`seed=3000000000`, VCD identical to `--seed 3000000000`), the 2^53
  message for the huge literal, docs; 153/153 `--atol 0` on its own baseline, no other case changes. A numeric
  net name beyond the int range becomes a syntax error there. To land: `git cherry-pick 7c49a98`.

## Decisions taken (not covered by the decision record; none changes results beyond the decisions)
1. **Default seed 0** (the decision asks for "a fixed documented seed").
2. **Creation order = netlist order, not name order** (B-176). The backlog is a work queue whose order sets the
   slot convention of bidirectional nets (B-144); netlist order is what B-176 asks for, it keeps every value of
   every case (only "Creating" lines move), while name order would have tied slot assignment to element names.
   Everything that is *collected and iterated or printed* (`sc_get_all_*`) is name-ordered, as decided.
3. **Name order = `strcmp` byte order** of the hierarchical names (upper case before lower case, `wg10` before
   `wg2`); equal names (SystemC makes them unique) would keep the hierarchy order (`stable_sort`).
4. **Dark-field injection on by default** (today's behaviour, now seeded): "kept as a documented option" does not
   say the default, and turning it off would change dark readouts (1e-40 A → exactly 0). Off: `darkfield=0`.
5. **`.options seed` in addition to `--seed`** (the scout design's serialisation of the seed), with `--seed`
   taking priority (like B-004's explicit `-m`). `--seed` is set directly, not through the `.options` footer,
   so it keeps the full 64-bit range and also applies to testbenches.
6. **Seeding point**: `Detector::start_of_simulation()` (once per simulation, after the CLI and `.options`;
   covers netlists, testbenches and C++ users that never call `prepareSimulation()`).
7. **Portable streams**: `std::mt19937_64` + `std::seed_seq` (specified by the standard) and SPECS's own
   uniform/normal (`<random>` distributions are implementation-defined); `std::fma` in the polar method so that
   `-ffp-contract`/`-march` cannot change the result. Cross-checked by the implementer against an independent
   Python implementation of `seed_seq` and `mt19937_64` (including the standard's 9981545732273789042 check
   value). libc++ is not installed here: untested.
8. **No stdout line and no VCD variable for the seed** (they would change every case); the manifest records it.
9. **Manifest**: written whenever SPECS opens the default trace, also when SystemC never creates the VCD
   (24 cases: `.op` only, or an error at `sc_start`); `trace_file` is then the configured name. Unwritable
   manifest → `Error: cannot open run manifest file`, exit 1 (like `--json`, B-017); a missing `traces/` now
   fails there (same exit status) instead of with SystemC E701. `systemc_version` (`sc_release()`) added next to
   `specs_version`. Data files read by VLSRC/EVLSRC are not listed (the decision says netlist files).
   `--print-manifest` was chosen for "printed on request"; `--version` is unchanged.
10. **One commit per item for the manifest**: the implementer's feature and harness commits were squashed with
    the re-record, since the stored manifests need both (writer and `specs_version` masking).
11. **`tb/det`/`tb/detector` re-recorded in item 4, not 2**: their readouts were still random during item 2.

## Found, not done
- **Lexer clamp** of integer literals beyond the int range (above; ready branch).
- `drop_all_events` in `SPECSConfig` (B-027) says "delete it together with B-016": it no longer affects any output
  (B-129 masks IDs, B-016 is fixed) but deleting it is not in this phase's scope.
- The harness's reorder hint still says "pointer-ordered listing? see README 'Known nondeterminism'"; the README
  section is "Determinism findings".
- Manifest: non-UTF-8 bytes in argv or paths are copied as is (invalid UTF-8 JSON); C++ users of `libspx.so` get a
  manifest with an empty `command_line` unless they set it; the version string has no dirty marker for an
  uncommitted tree (`utils/git-version.sh`); VLSRC/EVLSRC data files are not hashed; pyspecs does not read the
  manifest yet (scout §4).
- Unit tests that call `prepareSimulation()` leave `specs_test_<pid>.manifest.json` in `$TMPDIR`, next to the
  `specs_test_<pid>.vcd` files they already leave (1425 of them in `$TMPDIR` here).
- A netlist with no optical source aborts in the OP listing with "Wavelength not found in global vector" when a
  net is undriven (the darkfield case uses a 0 W CW source for that reason).
- `normal()` is bit-portable only as far as `std::log` is (glibc for libstdc++ and libc++ on Linux).
- `.options seed` error messages print the value with 6 significant digits, like the neighbouring options.
- `discover/03-parser-netlist-pyspecs.md` lists the PDET keywords without `DARKFIELD`.
- Noise is still drawn at every readout computation, also for gated beat samples (P3 note), so adding a beat
  sample changes later draws of that detector (deterministically).
- The VCD `$version` block is still SystemC's; the scout's idea of putting the SPECS version there was not decided.

## Final results
HEAD before this report: 1eb606c. Builds with the prebuilt deps (`specs-deps-next`):
- `/tmp/P4-fgcc`: fresh gcc 11.5, Meson defaults (release, asserts on); `libspx.so` links;
- `/tmp/P4-clang`: clang 19.1.7 release;
- `/tmp/P4-debug`: gcc `--buildtype=debug` (`-O0`);
- `/tmp/P4-asan`: clang 19.1.7 `-Db_sanitize=address,undefined -Db_lundef=false --buildtype=debug`, run with
  `ASAN_OPTIONS=detect_leaks=0`.

Results (same code as 1eb606c; the commits after 4e3e87d change docs only):
- **`meson test`**: 12/12 OK on all four builds (unit, 9 tb, golden-harness, golden); under ASan with
  `ASAN_OPTIONS=detect_leaks=0`.
- **Golden check** (`python3 -I tests/golden/run_golden.py check --specs <b>/specs --unit-tests
  <b>/src/test/unit_tests --baseline tests/golden/baseline --atol 0 --strict`): **153 passed (0 with notes), 0
  failed, 0 new, 0 missing, 0 skipped** on gcc, clang, gcc `-O0` and clang ASan+UBSan, i.e. 152 `specs` cases
  plus the gtest case (68 tests, all PASS). **No case declares `unordered_streams`, and no `--lenient-order`**:
  the order no longer depends on the allocator (before P4, ASan reordered 7 cases and changed where
  `cir/paul_3ring_puf` stops). No sanitizer report in any output.
- **`check --strict` twice in a row** (fresh gcc build, `--strict --strict-order`): 153/153 both times.
- **Reproducibility**: `record --repeat 3` (fresh gcc build, into a scratch dir) is identical across its 3 runs,
  and its output is **byte-identical** (`diff -r`) to the committed baseline for every case and the gtest
  results, including the noisy `tb/det`/`tb/detector` readouts, the f01 noise cases, the VCDs and the manifests.
- **Compiler warnings**: none in changed code (remaining: `src/specs.cpp` strict-aliasing in `sc_trace`, SystemC
  `SC_HAS_PROCESS` deprecations under clang, generated parser/scanner).
- **Golden baseline**: 133 → 152 `specs` cases (+17 seed/darkfield cases, +2 manifest cases). Existing cases
  re-recorded, all predicted (ordering, seeded noise, manifest, help text):
  - item 1 (B-176): 9 cases, creation-line order;
  - item 2 (B-016): 56 cases, ordering only (proof above);
  - item 4 (seeds): `tb/det`, `tb/detector` (seeded readouts), `cli/help`, `cli/no-args`;
  - item 5 (manifest): 84 cases gain `traces/*.manifest.json`, `cli/help`, `cli/no-args`;
  - gtest with items 2, 4, 5.
  Every recording appended a `rerecords` entry to `manifest.json` citing its item.

## Branches and worktrees
- `ws/repro` holds the phase.
- `P4-ready/lexer-int-overflow` @ 7c49a98: keep until the lexer fix is decided.
- Implementer branches, integrated (cherry-picked, reviewed) and safe to delete: `P4-wip/rng` (22feaa5 taken,
  d6e81d4 superseded by the ready branch), `P4-wip/manifest` (2815930 + 72eedeb taken, ff19964 dropped).
- Worktrees `/tmp/P4-wt/{rng,manifest,lexer}` are removed after this report.
