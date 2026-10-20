# Cleanup phase (ws-cleanup) — report

Sprint 2026-10, phase 3. Branch `ws/cleanup` from `sprint/2026-10` @ adab6b9. Brief:
`discover/orchestration/ws-cleanup.md`; plan: [`PLAN.md`](PLAN.md). Nothing was pushed.

## Outcome
All **22 items deferred by the fix phase are consumed** (status `fixed in …` in `backlog.md` and in each issue
header). Three items are only partly done, for reasons found during implementation: B-027, B-142 and B-073 (see
"Rejected or trimmed"). Two `#if 0` arms were also removed (X-1, X-2). Simulation results did not change. The only
golden change is the planned one: 88 debug-print lines deleted from 28 stored stdouts (C-08). Output files and exit
codes stayed byte-identical.

| | before (adab6b9) | after |
|---|---|---|
| GCC 11.5 warnings, unique (clean build, `-Dold_tb=true`) | 12 | **4** (the `specs.cpp` strict-aliasing ones, B-029) |
| clang 19 warnings, unique | 47 | **37** (36 are `SC_HAS_PROCESS`, B-030; plus generated `yynerrs`) |
| `src/` lines | | +140 / **−1307** (71 files) |
| `pyspecs/` + `utils/` lines | | +9 / −175 |
| golden baseline | | 28 `stdout.txt`: −88 lines, 0 added; `cases.json`: `unordered_streams` dropped for tb/det* |
| golden `check --atol 0 --strict` | 81/81, 7 reorder notes | 81/81, 8 reorder notes (GCC and clang) |

Unique warnings removed with GCC: `lambda0` (C-02), `ic_order` (C-01), GTD `delay` maybe-uninitialized (C-07),
and the 5 testbench unused locals (C-05). Clang also loses `parse_analysis.cpp` `i` set-but-unused and the
`netlist_content_t` typedef-for-linkage warning (C-04), plus the `SC_HAS_PROCESS` warning of the deleted `ring.h`.
With `-Db_ndebug=true`, the `has_reader` warning (B-112) is gone too (see Verification).

## Batches and commits (on `ws/cleanup`)
Each batch was implemented by a Sonnet subagent in `/tmp/ws-cleanup-wt/C-NN` (branch `cleanup-wip/C-NN`) with its
own build dir. Each one ran a clean build, compared warnings, ran `meson test` and the strict golden check at
`--atol 0`. At most two ran at a time. The orchestrator (Opus) reviewed every diff, cherry-picked it, and ran an
integration build plus a strict golden check after each wave.

| batch | commit | items | extra evidence |
|---|---|---|---|
| C-01 engine/CLI | c108018 | B-026 (all but the print), B-027, B-028 utils, B-073 specs.cpp, X-1, X-2 | −446 lines; `drop_all_events` bisected (below) |
| C-02 passive devices | 39e7734 | B-041, B-043, B-045 devices, B-067, B-111 header, B-073 cw_source.h | `--vci` netlist with SPLITTER 0.3 + PSHIFT: exactly the 2 intended lines differ |
| C-03 probes, Ring | bb043ec | B-068, B-073 devices | `-t lambda` and `add_drop_pyspecs`: stdout and VCD identical |
| C-04 parser | 4880c3d | B-110, B-112, B-174, B-173 comments/TRACEALL, B-045 parse_element | `-n --json` on 40 netlists: rc/stderr/JSON identical; 13 syntax edge cases identical; bison 199 → 198 states (T_NONE) |
| C-05 tb + gtest | 6bba225 | B-111 tb, B-141, B-142 | 14 gtests pass with `DOUBLE_EQ`; `-t ac_crow -m fd` identical except the now-unstuck `Total runtime` format |
| C-06 Python/scripts | a77cd9f | B-090, B-094, B-097 | 32 baseline VCDs parsed by old vs new `specsparse`: identical DataFrames and stdout; openwave repro fixed |
| C-07 helper | 537b2f0 | B-178, B-075, B-028 devices | `sc_object` name dump identical: `-t mesh` 1003 names, lambda 86, mzi 119, add_drop_pyspecs 152; OCTANE/GTD by call-site reading |
| C-08 debug prints | bbce9e2 | B-065, B-173 prints, B-026 reltol echo | proof below |
| C-08 baseline | 7866df4 | — | baseline −88 lines, 0 added |

Also: 665cc0f (plan), 607d6a0 (proof script `check_print_removal.py`).

### C-08 proof (the single re-record exception)
`backlog/cleanup/C-08-proof.txt`: (1) a full recording with the binary before C-08 against one with the binary after
it, and (2) the stored baseline before against after. Both pass `check_print_removal.py`: in 28 cases every lost line
matches one of the deleted messages, no line is added, and exit codes and output files are byte-identical in all
80 cases. The fix phase predicted 11 cases. The other 17 are `err/*` and `regress/*` cases whose netlists contain a
PROBE (`created ROOT/PROBEn`) or a PSHIFT/MZI (copied DC warning). Their stdout is stored even where
`ignore_streams` hides it.
**Decision:** the subagent's `record --only` also rewrote heap-ordered lines and VCD declaration order (B-016) in 4
cases. The orchestrator replaced it with an in-place edit: old baseline minus exactly the removed lines
(`apply_removals.py`). The baseline diff is therefore deletions only, and `manifest.json` → `rerecords` records the
step with its note. **Decision:** B-026's `reltol` echo is the same kind of leftover debug print as B-065/B-173, so
it went into this commit under the same proof.

## Rejected or trimmed
- **B-027 `SPECSConfig::drop_all_events`**: all its readers and writers are gone, but the member stays (with a
  comment). Removing it fails `regress/b031-mzi-bidir`, because the printed OpticalSignal IDs (`[0235]` vs
  `[0199]`) follow heap layout (B-016). Delete it together with B-016.
- **B-142 `sc_main` stub**: wrong premise under Meson. `src/test/meson.build` links gtest without `gtest_main`, so
  libsystemc's `main()` → `sc_main` runs `RUN_ALL_TESTS`. With the stub, no test ran (golden: tests missing). Kept.
- **B-073**: the `isnan` guard and the live `m_lambda_signals.insert` stay, as its second check asked. So does the
  commented `p_readout->write` (B-049). `<random>` stays in `detector.h` (used). `<fstream>` in `probe.h` was not
  listed and was left.
- **B-141**: `IN3`/`IN4` (printed IDs), both phase-shifter checks, the `ptrough` name and `mesh_tb` are kept, as
  its corrections say. The `pc` leak in `crow_tb` is left.
- **B-045**: the "Constructor for Waveguide" comments had already been fixed upstream, so there was nothing to do.
- **B-094**: the wildcard import stays (public API). Removing unused imports drops `specs.plt`/`specs.json` from
  the re-exported names; nothing in the repo uses them (grep).
- **Not attempted (out of scope, already logged):** Uni/Bi duplication (B-046, needs-design), VL/EVL duplication
  (B-074), `SC_HAS_PROCESS` (B-030), `using namespace std` in headers (B-024), the strict-aliasing warnings (B-029),
  PhaseShifter "nan wavelength" lines (B-042), OCTANE/GTD (B-069), `" points)"` (changes 5 stdouts), `"subcircuits":"TODO"`.

## Notes
- Heap-layout reorder notes: 7 before, 8 after (`regress/b031-mzi-bidir` was added by C-07's `spx_child_name`).
  They are accepted by the default check (FIXES-REPORT decision 3). Any allocation change moves them. B-016 is the
  real fix, and it would also let `drop_all_events` go.
- `src/test/meson.build` comment reworded: trace files are per-process now. `is_parallel: false` is kept.
- `crow_tb` FD mode resets `cout`'s `fixed`/precision after the sweep (B-141). Only `-t ac_crow -m fd` is
  affected, and it is not a golden case.
- Worktrees left in `/tmp/ws-cleanup-wt/`: `C-01` … `C-08` (branches `cleanup-wip/C-01` … `C-08`; everything in
  them is cherry-picked, except C-08's second commit, which was reworked as 7866df4), `base-src` (detached at
  adab6b9, used for the clang baseline). Build dirs `base-build`, `base-clang`, `final-{gcc,clang,ndebug}`,
  recordings `pre-c08-keep`, `c08-new`, `old-baseline`. Remove them with `git worktree remove` / `rm -rf`.

## Verification of `ws/cleanup` HEAD
Clean build dirs, `-Dold_tb=true`. Golden: `check --unit-tests … --atol 0 --strict`.

| build | warnings (unique) | `meson test` | golden |
|---|---|---|---|
| GCC 11.5, default (asserts on) | 4 | 11/11 OK | 81/81 pass, 8 reorder notes |
| clang 19.1.7 | 37 | 11/11 OK | 81/81 pass, 8 reorder notes |
| GCC, `-Db_ndebug=true` | 4 (`has_reader` gone) | 11/11 OK | 81/81 pass, 8 reorder notes |

`tb/det*` also pass `--strict-order` with GCC and clang, now that `unordered_streams` is gone.
