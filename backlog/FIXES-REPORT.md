# Fix phase (ws-fixes) — report

Sprint 2026-10, phase 2. Branch `ws/fixes`, from `sprint/2026-10` @ 3b94976. Instructions:
`discover/orchestration/ws-fixes.md`; working log: [`fixes-progress.md`](fixes-progress.md). Nothing was pushed.

## Outcome
All **70 `easy-unattended`** items are dealt with: **46 fixed** (one `fix(B-NNN)` commit each), **2 obsolete**
(Meson migration), **22 deferred** to the cleanup phase (pure `style/maintainability`, as the orchestration rules
ask). None was abandoned, reclassified or skipped: no fix changed the result of a valid netlist, and every golden
change matched the issue's predicted *Golden impact*. All 70 were `CONFIRMED`; there was no `PLAUSIBLE` candidate.
B-119, B-120 and B-121 (`needs-design` build items) were also marked obsolete, since the files they are about are
gone. `backlog.md` has a new `status` column, and each issue's header line has a `status:` field.

| | before | after |
|---|---|---|
| golden `specs` cases | 44 | 80 (29 `err/*`, 6 `regress/*`, `tb/ac_add_drop-fd`) |
| gtest tests | 3 (1 known failure) | 14, all PASS; `unit-known-failure` is gone |
| meson `tb` smoke tests | 8 | 9 (`mesh` no longer segfaults) |
| cases expected to crash/abort | `tb/mesh` (SIGSEGV) | none |

## How it was done
- 7 implementer subagents in isolated worktrees, by file area: Opus for test infrastructure, engine/CLI, parser
  validation, parser lexer/grammar/memory and devices (crash/UB); Sonnet for pyspecs/Python and docs/misc. Each
  reproduced the problem, added a regression test, made the minimal fix, and ran `meson test` and the golden check
  at `--atol 0`. The orchestrator reviewed every diff, cherry-picked it, and ran the full suite on `ws/fixes` after
  every batch. B-023 was done last, directly by the orchestrator (2 lines plus a label-only re-record).
- New regression netlists live in `tests/golden/netlists/` (not `circuits/`). Case ids: `err/bNNN-*` means the
  input must be rejected with a clean message, `regress/bNNN-*` means a fix made something work. Most `err/*`
  cases set `ignore_streams: [stdout]`, so later stdout-only cleanups (B-065, B-173) do not touch them.

## Items
Commits are on `ws/fixes`. Golden "none" means no existing case changed (new cases may have been added).

| ID | outcome | commit | tests added | golden impact |
|---|---|---|---|---|
| B-006 | fixed | eee3be8 | `regress/b006-empty-output` | none |
| B-008 | fixed | ff391b2 | `err/b008-dc-bare` | none |
| B-017 | fixed | 28adebe | `err/b017-json-unwritable` | none |
| B-018 | fixed | b588268 | gtest `src/test/specs_config.cpp` (2 tests), `err/b018-resolution-below-fs` | gtest +2 PASS |
| B-020 | fixed | 2f69dc5 | gtest `src/test/optical_output_port.cpp` (3 tests) | gtest +3 PASS |
| B-023 | fixed | 5ea38e4 | the stdout of every case that prints a signal | stdout label `V.m⁻¹` → `√W` in 30 cases (the 25 predicted + 5 that print signals since this phase); label only, verified bit-exact |
| B-031 | fixed | 2d1e116 | `regress/b031-mzi-bidir` (Bi vs Uni MZI side by side, both directions) | none |
| B-034 | fixed | 2408b9f | 3 × `err/b034-*` | none |
| B-036 | fixed | a8600c3 | `regress/b036-vci-crow` | none |
| B-044 | fixed | f6ec1c5 | none feasible (class never instantiated) | none |
| B-047 | fixed | aa0e1c6 | `tb/mesh` now runs; `mesh` added to the meson `tb` suite | `tb/mesh`: SIGSEGV → exit 0 with `traces/mesh_tb.vcd` |
| B-054 | fixed | 2997ba2 | `tb/det`, `tb/detector` now compare the readout signals | t=0 VCD values of 3 detector signals: garbage → 0; `vcd_ignore_values` dropped |
| B-064 | fixed | 7caf561 | `err/b064-evlsrc-missing-file` | `cir/paul_3ring_puf` stderr names the missing file |
| B-072 | fixed | 9939a3e | `tb/ac_crow` stdout | `tb/ac_crow` stdout: CROW child names prefixed (`crow_wg_top_0`, …); VCD unchanged |
| B-076 | fixed | 286bd34 | scripted check (no pyspecs suite: B-096) | none |
| B-077 | fixed | 601c777 | scripted check on pandas 2.3 and 3.0 | none |
| B-082 | fixed | 907abee | templates checked by running their netlists | none |
| B-083 | fixed | bfba715 | doxygen warning count (78 → 74) | none |
| B-085 | fixed | a921bc0 | scripted check | none |
| B-086 | fixed | d22213b | scripted check | none |
| B-087 | fixed | ef0ea37 | scripted check | none |
| B-088 | fixed | 685edc0 | notebook cells executed in a scratch tree | none |
| B-089 | fixed | b63f02f | `pip install -r` in fresh venvs (py3.9/pandas 2, py3.12/pandas 3) | none |
| B-091 | fixed | aacfde1 | scripted check | none |
| B-092 | fixed | a20b67a | scripted check | none |
| B-099 | fixed | 5c29fbb | scripted check | none |
| B-100 | fixed | b3f1719 | doxygen output (links resolve) | none |
| B-102 | fixed | acc890d | doxygen warning count (74 → 47) | none |
| B-109 | fixed | c712d3f | build warning gone | none |
| B-114 | fixed | 427873a (+ abeaa4b, CLAUDE.md note) | `git status --ignored` before/after | none |
| B-123 | fixed upstream (b7d6fff); eb7ca0e re-records the baseline | `Waveguide.OperatingPoint` enforced in the `unit` suite | gtest `Waveguide.OperatingPoint` FAIL → PASS |
| B-125 | fixed | 60972ff | harness probes run locally (64 KiB output, ASSERT escape, non-std throw), not committed | none |
| B-131 | fixed | 449d63c | `regress/b131-nrings-crow`, `err/b131-nrings-crow-zero` | none |
| B-132 | fixed | 093f890 | `cir/subckt_test` | `cir/subckt_test`: parse error → exit 0 (no VCD) |
| B-136 | fixed | 84d087e | gtest `src/test/devices/models.cpp` (6 tests; coupler phase sign not pinned) | gtest +6 PASS |
| B-138 | fixed | 6fa8c9e | `tb/ac_add_drop-fd` | one new case |
| B-148 | fixed | 2310bdd | 9 × `err/b148-*` (one per converted assert) | none |
| B-149 | fixed | bc74a11 | `err/b149-x-noargs`, `err/b149-x-kwargs-only` | none |
| B-150 | fixed | 89db4c4 | `regress/b150-include-in-subckt` | none |
| B-153 | fixed | d90348d | 5 × `err/b153-*` | none |
| B-154 | fixed | 0dcfda3 | the `cir-json/*` cases | exactly the 10 predicted `writers` values in 3 `cir-json/*` cases |
| B-156 | fixed | 00556f2 | `regress/b156-many-includes` (110 includes under a 64-fd limit; new case key `rlimit_nofile`) | none |
| B-161 | fixed | c12ea44 | `err/b161-ic-unsupported` | none |
| B-165 | fixed | 91dc39f | 4 × `err/b165-*` | none |
| B-170 | fixed | 86f2707 | valgrind before/after (1,450 B → 0 definitely lost), not committed | none (heap-layout "reordered" notes) |
| B-177 | fixed | 2a4df67 | bison warnings gone; LALR tables identical | none |
| B-117, B-118 | obsolete | — | — | — (CMake removed in 57714e7) |
| B-026, B-027, B-028, B-041, B-043, B-045, B-065, B-067, B-068, B-073, B-075, B-090, B-094, B-097, B-110, B-111, B-112, B-141, B-142, B-173, B-174, B-178 | deferred to ws-cleanup | — | — | — |

## Decisions taken (unattended)
1. **Pure `style/maintainability` items deferred** to ws-cleanup (22), including the warning-only ones
   (B-075, B-110, B-111, B-112) and B-142, which SUMMARY.md lists in step 1. The ws-cleanup brief covers them.
   B-044, B-072 and B-170 (category bug/memory, listed under cleanup in SUMMARY.md) were fixed here.
2. **Golden harness** (b1f333f): `record --only --note` used to drop the note; it is now appended to
   `manifest.json` → `rerecords`, so every partial re-record says which B-ID caused it. B-156 added a case key
   `rlimit_nofile`. It runs specs via `sh -c 'ulimit -S -n N; exec …'`, not a `preexec_fn`, because cases run
   in threads.
3. **"Reordered" notes are not re-recorded.** Stdout line order follows heap addresses (B-016/B-176), so every
   fix that moves allocations reorders the OP printout. These differences are accepted by default, and 7 cases
   show them now (`cir/add_drop_pyspecs`, `cir/subckt_tests`, `opt/{tracefile-o,mode-sampled-time,multi-file,vci}`,
   `regress/b006-empty-output`). Fixing B-016 is the real cure.
4. **Cross-item interactions** were re-recorded inside the later commit, and each one was checked to differ only by
   the expected text: `regress/b131-nrings-crow` and `regress/b036-vci-crow` were recorded before B-072 renamed the
   CROW children.
5. **B-023 last, label substituted in place.** A plain `record` of the 30 cases would also have rewritten their
   VCDs (scope declaration order follows the heap layout; values are identical), a 38k-line diff. Instead the label
   was replaced in the stored stdout (`case.json` only hashes output files). The new build matches that baseline
   at `--rtol 0 --atol 0 --print-slack 0`, and every changed line differs only by the label.
6. **Error-message pattern** for converted asserts (B-148): `Error (<ELEM>): …` for elements, `Error: .<directive>: …`
   for analyses/directives. B-052 (needs-human) is told to follow "the pattern of B-148".
7. **B-153 timescale:** the truncated value is range-checked as a double before the int cast, so values that ran
   before (e.g. `-15.5` → -15) still run.
8. **B-018 vs B-153:** both stay. B-153 rejects bad `.options` at parse time; B-018 is the config-level check that
   also covers the CLI and `prepareSimulation()` callers.
9. **Commit trailers:** the docs/misc batch (Sonnet) carries `Co-Authored-By: Claude Sonnet 5.5`, which is accurate,
   and was kept.

## Notes for the next phases
- **ws-cleanup:** shared lines already handled. `crow.cpp:96` was removed by B-072 (B-027 skips it). The
  `ParserState` leak in `main.cpp` was fixed by B-170 (B-026 and B-174 skip it). B-178 is unblocked (B-047 landed),
  and so is B-094 (the other pyspecs fixes landed). B-142: B-123/B-125 rewrote the `utils.h` macros, so recheck what
  remains. B-065/B-173 now also change the stdout of the new `regress/*` cases that print CROW/PROBE debug lines
  (`regress/b131-nrings-crow`, `regress/b036-vci-crow`, …); re-record those with the same note.
- **Human decisions:** nothing new was escalated. `backlog/repro/a2-mzi-bidir.cir`'s header comment still quotes
  "0.5 W" for the bind-only variant of B-031 (backlog files were not edited by implementers).
- Unit tests leave small `specs_test_<pid>.vcd` files in `$TMPDIR` (per-process paths from B-136; B-142 tracks the
  fixed `/tmp` path of `waveguide.cpp`).
- The implementer worktrees (`.claude/worktrees/agent-*` in the main checkout, with their `worktree-agent-*`
  branches) were left in place. Everything in them is cherry-picked; `git worktree remove` cleans them up.

## Verification of `ws/fixes` HEAD
All builds use `-Dold_tb=true` (needed by the `tb/*` cases). Golden checks ran with `--unit-tests --atol 0 --strict`.

| build | `meson test` | golden (80 specs cases + gtest) |
|---|---|---|
| clean GCC 11.5, default options (asserts on) | 11/11 OK | 81/81 pass; also bit-exact (`--rtol 0 --atol 0 --print-slack 0`) |
| clean GCC, `-Db_ndebug=true` | 11/11 OK | 81/81 pass |
| clean clang | 11/11 OK | 81/81 pass |
| incremental `build/` (after every batch) | 11/11 OK | 81/81 pass |

"Notes" are only the accepted heap-layout reorders: 7 cases with GCC, 9 with clang.

Compiler warnings, clean GCC build: 14 unique before, 9 after. The 5 removed are the flex/bison warnings fixed by
B-109 and B-177, and none is new. The rest (B-075, B-111, B-112, B-113, `ic_order`, strict aliasing) belong to the
cleanup phase or to other items.
