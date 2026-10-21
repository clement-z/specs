# PA report — decided follow-ups for v0.2.0 (branch `ws/followups`)

Sprint 2026-10c, phase A, on top of `sprint/2026-10c` @ 25453cd, with `sprint/2026-10c` @ 8af20cc merged in
(ff4d2e2, at the coordinator's request: B-181 `Variable` initialisers, before item 3). Prompt:
`discover/orchestration/2026-10c/PA-followups.md`. All four items are **done**. Nothing was pushed.

How the work was split: the orchestrator did items 1 and 2, all golden recording and the bookkeeping. Two Opus
implementers ran in parallel for items 3 and 4, in `/tmp/PA-wt/int64` (`PA-wip/int64`) and `/tmp/PA-wt/zerotol`
(`PA-wip/zerotol`). They committed code, tests, netlists and docs, and proposed their golden case entries outside
the repository. The orchestrator reviewed both diffs, cherry-picked them, applied the case entries, checked every
golden change, and recorded only the predicted cases. Mid-phase the maintainer asked that new subagents get a model
chosen per task (haiku/sonnet/opus) and an explicit effort of medium or high; the two implementers already running
were not restarted, and no subagent was started after that.

## Items

| # | item | outcome | commit | tests added | golden impact |
|---|---|---|---|---|---|
| 1 | B-146 subckt port nets via `nets_flat` | done: `P2-ready/b146` (8129020) cherry-picked, code unchanged; `cases.json`/`manifest.json` conflicts resolved as a union (both sides' cases and `rerecords` entries) | 5912b6a (status 7b9ada5) | golden `regress/b146-x-shared-{net,delayed}(-flat)`, `err/b146-x-net-type-mismatch` | `cir-json/subckt_tests` only: `flat.json` `writers` of `ROOT/out` and `ROOT/X2/N00001` 0 → 1 (as predicted); the 5 new cases. Hierarchical and flat cases give identical OP listings and identical VCD value sections |
| 2 | B-014 error, option (b) | done: `WaveguideBase::m_from_netlist` (default `false`) is set by the parser in `WGElement::create()` (netlist lines, also inside a `.subckt`); `checkDelayRounding(name, delay, zero_is_error)`: a non-zero delay that rounds to 0 is an error (clean message, exit 1, in `start_of_simulation()`, before the simulation) for netlist waveguides, the existing warning for every other waveguide (MZI arms, OCTANE, CROW, meshes, testbenches). No timescale change to b031 | a23e4f9 (status b5f7046) | gtest `DelayRoundingDeathTest.CheckDelayRounding` (both origins), new `WaveguideDeathTest.DelayRoundingToZeroIsAnErrorOnlyForNetlistWaveguides`; golden `err/b014-delay-rounds-to-zero`, `err/b014-subckt-wg-rounds-to-zero`, `regress/b014-device-internal-rounds-to-zero` | none on the other existing cases; `regress/b031-mzi-bidir` unchanged at the default timescale (its 18 MZI-arm warnings stay). `regress/b014-delay-rounds-to-zero` (the netlist-waveguide case itself) became `err/b014-delay-rounds-to-zero`, see decision 3 |
| 3 | 64-bit integer literals (B-180) | done: digit-only literals are lexed with `std::from_chars` into `int64_t`; beyond 9223372036854775807: `Parsing error (<file>:<line>): integer literal ... is out of range (the limit is 9223372036854775807)` plus the line, exit 1. `%union i_val`, `Variable::inum` (B-181 initialiser kept), `Variable(int64_t)`, `as_integer()` are 64-bit (a DOUBLE outside the 64-bit range is a clean error, no UB cast). Narrow consumers: PCMCELL `n` and `k` (stored as `int`) reject values beyond the int range with `Error (<name>): ...`; numeric net names keep the full range (`N3000000000`); `.options timescale`/`resolution` were already range-checked. `.options seed` and `maxiter` take an integer literal exactly (0 to 2^63 − 1), exponent form keeps 0 to 2^53; P4's 2147483647 guard removed. New backlog entry B-180 (77a3c36) | bd2465c (implementer 0e1fede; status 24e9fa7) | gtest `IntegerLiterals.*` and `IntegerLiteralsDeathTest.*` (6: `Variable` storage and JSON, `as_integer()` of doubles and its errors, lexer boundary 2^63 − 1 / 2^63 / 20 digits with file:line, `name_from_id`); golden `regress/f01-options-seed-int-literal`, `regress/f01-options-seed-int64-max`, `err/b180-int-literal-{kwarg,positional,net-name}`, `regress/b180-numeric-net-names`, `err/b180-pcmcell-{n,k}-beyond-int` | as predicted: `err/f01-options-seed-huge-integer` (new parse-error message); `err/f01-options-seed-int-max` → `regress/f01-options-seed-int-max` (runs, VCD identical to `--seed 2147483647`); the new cases; nothing else |
| 4 | Zero tolerances allowed | done: `abstol >= 0`, `reltol >= 0` in `.options`, `--abstol`/`--reltol` and `verifyConfig()`; negative values and NaN stay errors. When both are 0, `verifyConfig()` prints one warning per run (member flag; it runs in `runAnalysis()` and `prepareSimulation()`). With `reltol = 0` the reltol test passes for any change (also when \|dE\|/\|last\| underflows); the F-28 residual reports `inf x` for a zero threshold instead of dividing by 0. Non-zero tolerances take exactly the old paths | 26327cb (implementer a2cc395; status 09cccfe) | gtest `SPECSConfig.VerifyConfigAcceptsValidValues`, `SPECSConfigDeathTest.VerifyConfigRejectsInvalidValues` (updated), new `OpticalOutputPort.EmissionGateWithZeroTolerances`, `SPECSConfig.ZeroTolerancesWarnOnce`; golden `regress/b153-zerotol-{open-loop,cli,lossy-ring-tran,ring-op}`, `err/b153-zerotol-lossless-ring-op`, `err/b153-zerotol-cli-{abstol,reltol}-negative` | as predicted: `err/b153-reltol-zero` → `regress/b153-reltol-zero` (runs, stderr empty since abstol is not 0); the new cases. **Plus `err/b153-abstol-negative`**: its message now states the new bound, `must be >= 0 (got -1)` instead of `must be > 0`; exit 1 unchanged. Not in the phase table, see decision 9 |
| — | docs | done: `doc/numerics.md` (delay-rounding error rule, "Zero tolerances" section, seed/maxiter ranges), `doc/netlist.md` (integer literals), `doc/reproducibility.md` (seed range), CHANGELOG `[unreleased]` (B-146, B-014, B-180, zero tolerances, the F-01 seed range), `tests/golden/README.md` (case counts); backlog status of B-146, B-014, B-018, B-153, B-180 and their `backlog.md` rows | 3eead8f and the status commits | — | — |

The gtest case was re-recorded with items 2, 3 and 4 (gtest results: 86 → 95 tests, all PASS: +1 B-014, +6 B-180, +2 zero tolerances). Every recording
appended a `rerecords` entry to `manifest.json` citing its item, and every recording passed `record --repeat 3`.
Stale baseline directories of the three renamed cases were removed, with their ids, from `manifest.json`.

### Hand-checked values of the new cases
| case | expected = observed |
|---|---|
| b146-x-shared-net / -flat | mid:0 = mid:1 = 1 mW at −2.1890 rad; hierarchical and flat OP listings and VCD value sections identical |
| b146-x-shared-delayed / -flat | mid:0 = 1 mW at −2.1890, mid:1 = 2 mW at −1.1756 rad; identical to flat |
| b014-device-internal-rounds-to-zero | MZI arms 43.36 fs → 0 (2 warnings) and 88.46 fs → 100 fs (+13 %); netlist WG (755 fs) silent; out1 = sin²(0.8918)·0.99977 mW = 0.6055 mW, out2 = 0.3942 mW (listing 0.000605 / 0.000394 W); exit 0 |
| b014-delay-rounds-to-zero, b014-subckt-wg-rounds-to-zero | `Error (ROOT/WGR)` / `Error (ROOT/X1/WGR)`: delay 1.33426e-14 s rounds to 0 at 1e-13 s, suggests `timescale=-15`; exit 1 |
| f01-options-seed-int-max / -int-literal / -int64-max | VCD identical to `--seed 2147483647` / to the baseline of `regress/f01-seed-large` (`--seed 3000000000`) / to `--seed 9223372036854775807` |
| b180-numeric-net-names | nets N00007, N3000000000, N9223372036854775807 all 1 mW (`.op`) |
| b153-zerotol-open-loop / -cli | out1/out2: 2.5e-05/7.5e-05 W at 10 ps, 2.5000025e-05/7.5000075e-05 at 20 ps, 2.5e-22/7.5e-22 at 30 ps, 0 at 40 ps; "Simulated 50 ps"; one warning line (also with `--abstol 0 --reltol 0`) |
| b153-zerotol-lossy-ring-tran | out pulses \|E\| = 0.00158114·0.0707107^(n−1): 0.00158114, 0.000111803, 7.90569e-06, ...; last 1.97626e-323 at 2790 ps (n = 279); "Simulated 2801 ps"; 32 kB baseline |
| b153-zerotol-ring-op | exact: out = 1.000000 W, ring = 24.626059 W (default tolerances: 0.995098 W, 24.563207 W) |
| b153-zerotol-lossless-ring-op | warning, then the F-28 error at maxiter = 1000; residual of 3 ports, \|dE\| 9.99999e-09, 9.99999e-05, 9.99999e-05, each `> 0 (inf x)`; exit 1 |

## Decisions taken (not covered by the decision record; none changes simulation results beyond the decisions)
1. **Merge of `sprint/2026-10c`** (ff4d2e2) into `ws/followups` before item 3, as the coordinator asked (B-181
   widens the same `Variable` lines). The branch merges cleanly with the current `sprint/2026-10c` (417eb09,
   board-only commits since).
2. **B-146 cases**: the 5 new cases lost their `unordered_streams: [stdout]` (added for the pointer-ordered listing
   that P4 fixed; no case declares it any more) and were recorded with the current build (their branch baselines
   predated P3's B-048 sampling and P4's ordering and manifests).
3. **B-014, the existing rounds-to-zero case**: `regress/b014-delay-rounds-to-zero` is exactly a netlist waveguide
   whose delay rounds to 0, so the decision turns it into an error; it became `err/b014-delay-rounds-to-zero`, as on
   `P3-ready/b014-error`. This is the "netlist waveguide rounding to 0 → error case" of the table.
4. **B-014, where the waveguide comes from**: a member set by the parser, defaulting to "device-internal", so
   testbenches and C++ users keep the warning. A waveguide inside a `.subckt` is written in the netlist: error (new
   case `err/b014-subckt-wg-rounds-to-zero`). Only a rounding to 0 depends on the origin; the > 10 % warning does not.
5. **B-180, `f01-options-seed-int-max`**: its netlist wrote `seed=3000000000`; it now writes `seed=2147483647`
   (the old clamp value, matching its name and the table's "VCD identical to `--seed 2147483647`"), and
   `seed=3000000000` moved to the new `regress/f01-options-seed-int-literal`.
6. **B-180, `.options maxiter`** gets the same exact-integer treatment as `seed` (an integer literal above 2^53 would
   otherwise lose precision through `as_double()`). Its DOUBLE rule is unchanged.
7. **B-180, narrow consumers**: PCMCELL `n` and `k` reject only values beyond the int range here; values inside it
   keep their B-060 checks and messages (so no existing case changes). The `k` message says "between 0 and
   2147483647" since `n` may not be parsed yet. The lexer error exits 1 (the common error status; other lexer errors
   exit 3, `yylerror()`'s value). `unit_tests` now links `parser_dep` to drive the lexer from gtest (`libspx.so` has
   no parser, so nothing is linked twice).
8. **Zero tolerances, warning wording**: "may never quiesce" and "may stop at maxiter" instead of the decision's
   "never quiesce" and "end at maxiter", because the decision's own lossy-ring case terminates and the B-124 ring
   `.op` converges exactly at zero tolerance (both are golden cases now). Exact text:
   `Warning: abstol = 0 and reltol = 0: the emission gate is off, every change of a value is emitted; a circuit with
   feedback may never quiesce in .tran (it stops only when its values repeat exactly), and .op/.dc may stop at
   .options maxiter with a non-convergence error`. A dry run (`-n`) does not warn; `-t` testbenches ignore the CLI
   tolerances (pre-existing), so they do not warn either. `-0` is accepted and acts as 0.
9. **Zero tolerances, `err/b153-abstol-negative`**: the one existing-case change outside the phase table. Keeping
   "must be > 0" would state a bound that is no longer true; the new message states the decided bound. Only that
   stderr line changes (`> 0` → `>= 0`); the case still fails with exit 1, as "negative values stay errors" requires.
   Re-recorded in the item's commit and named in its `rerecords` note.
10. **Zero tolerances, the gate at `reltol = 0`**: a `m_reltol == 0` branch makes "gating is off" literally true
    (without it, a change whose ratio to \|last\| underflows to 0 would be dropped). Non-zero tolerances are
    bit-identical (all 212 cases at `--atol 0`).
11. **Extra cases** beyond the table's minimum (all new, none changes an existing case): `regress/b180-numeric-net-names`,
    `regress/f01-options-seed-int64-max`, `err/b180-pcmcell-k-beyond-int` (the double path, which was UB),
    `regress/b153-zerotol-ring-op` (an `.op` that converges exactly at zero tolerance), and the two CLI negative cases.

## Found, not done
- **Parse errors inside a `.subckt` body** print an empty file name and a garbage line number
  (`Parsing error (:1634561391)`, `:-1094795586` = 0xBEBEBEBE under ASan): the `yy_scan_string` buffer of
  `ParseTree(name, subcircuit, kwargs)` (`src/parser/parse_tree.cpp` ~339) never gets `yyset_lineno` or a file
  name, so an uninitialised value is read. Older than this phase; it affects every parse error in a subcircuit body,
  including the new integer-overflow one. Worth a backlog entry (B-182?).
- `.options resolution`: an integer literal above 2^53 still goes through `as_double()` (no UB, range-checked, but
  not exact like `seed`/`maxiter`).
- `as_integer()` errors for huge doubles (`n=1e20`) are generic, without the element name; `PCMCELL n=2.5` is still
  truncated to 2 silently (old `as_integer()` behaviour).
- A negative integer such as `-5` is a unary minus on a literal, so it becomes a DOUBLE: a negative INTEGER cannot be
  written.
- Zero tolerances: the traced phase of a field of exactly 0 can read π (the waveguide product gives (−0, +0); the
  snap normally writes +0). A C++ per-port `OpticalOutputPortConfig` with zero tolerances gets no warning (only the
  global defaults are checked). `.options reltol=1e400` (inf) is still accepted (P3 note). The CLI message says
  "abs_tol" for `--abstol` (pre-existing).
- At zero tolerance the number of decay steps before a value underflows could in principle depend on floating-point
  contraction (`-march=native`, FMA); gcc native and clang non-native give the same records here, but
  `regress/b153-zerotol-lossy-ring-tran` is the case to look at if another compiler or CPU disagrees.
- `kind()` prints "MZI MODULATOR (bidir)" for unidirectional MZIs (like the CROSSING label noted by P3).
- Procedure: a direct `run_golden.py check` on an ASan build reports the accepted flex-buffer leak of parse-error
  exits unless `LSAN_OPTIONS=suppressions=tests/sanitizers/lsan.supp` is set, as `meson test` does; 4 parse-error
  cases then "fail" on stderr. With it: 212/212.
- Procedure: a background `meson compile` that overlaps a cherry-pick can keep a stale object (the compile read the
  old source and finished after the checkout, so ninja saw a newer output). Seen once on the ASan build; fixed by
  touching the file. Don't build while changing the tree.
- `P2-ready/b146`, `P3-ready/b014-error` and `P4-ready/lexer-int-overflow` are superseded by this phase (the last one
  was not used, as decided) and can be deleted by the coordinator.

## Final results
HEAD before this report: 3eead8f (code identical to 09cccfe; the last commit changes `tests/golden/README.md` only).
Fresh builds with the prebuilt deps (`specs-deps-next`):
- `/tmp/PA-fgcc`: gcc 11.5, Meson defaults (release, asserts on, `-march=native`);
- `/tmp/PA-fclang`: clang 19.1.7 through ccache, release;
- `/tmp/PA-fasan`: clang 19.1.7 `-Dbuildtype=debug -Db_sanitize=address,undefined -Db_lundef=false -Dnative=false`.

Results:
- **`meson test`**: 12/12 OK on all three builds (unit, 9 tb, golden-harness, golden); on the ASan build with
  leak detection on (`tests/sanitizers/lsan.supp`, unchanged), no sanitizer report in the test log.
- **Golden check** (`python3 -I tests/golden/run_golden.py check --specs <b>/specs --unit-tests <b>/src/test/unit_tests
  --baseline tests/golden/baseline -j 2 --atol 0`, strict order and coverage by default): **212 passed (0 with
  notes), 0 failed, 0 new, 0 missing, 0 skipped** on gcc, clang and clang ASan+UBSan (the last with
  `LSAN_OPTIONS=suppressions=tests/sanitizers/lsan.supp`), i.e. 211 `specs` cases plus the gtest case (95 tests, all
  PASS). The gcc build used for recording (`/tmp/PA-gcc`) and the incremental ASan build gave the same after each item.
- **Compiler warnings**: none in changed code (remaining: generated parser/scanner, `src/specs.cpp` strict-aliasing
  in `sc_trace`, SystemC `SC_HAS_PROCESS` deprecations under clang).
- **Golden baseline**: 189 → 211 `specs` cases (+5 B-146, +2 B-014, +8 B-180, +7 zero tolerances; three cases
  renamed between `err/` and `regress/`). Existing cases re-recorded, each mapped to its item:
  `cir-json/subckt_tests` (B-146), `err/f01-options-seed-huge-integer` (B-180), `err/b153-abstol-negative` (zero
  tolerances, decision 9); renamed and re-recorded: `err/b014-delay-rounds-to-zero`,
  `regress/f01-options-seed-int-max`, `regress/b153-reltol-zero`; gtest with items 2, 3, 4.

## Branches and worktrees
- `ws/followups` holds the phase.
- Implementer branches, cherry-picked after review: `PA-wip/int64` (0e1fede), `PA-wip/zerotol` (a2cc395). Their
  worktrees `/tmp/PA-wt/{int64,zerotol}` are removed after this report; the branches can be deleted.
- Heartbeat `2852972a` (PA-heartbeat) deleted after this report is committed.
