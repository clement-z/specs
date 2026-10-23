# PD2b report: removals (decisions of 2026-10-23)

Branch `ws/cleanup2`, from `sprint/2026-10c` (2140dd4). Scope: "Decisions of 2026-10-23 (phase D)" in
`backlog/DECISIONS-2026-10-21.md`, prompt `discover/orchestration/2026-10c/PD2b-cleanup.md`. No `git push`, no merge.
Every item stayed within the predicted golden impact. No item was stopped.

## Items

| # | Item | Outcome | Commit | Tests | Golden impact |
|---|---|---|---|---|---|
| 1 | No-coalescing switch | done | 8c17275 | 2 gtests removed; golden `err/f15-options-coalesce-removed`, `err/f15-no-coalescing-removed` (new); `regress/f15-coalesce-options`, `regress/f15-no-coalescing-cli` removed | as predicted: `cli/help`, `cli/no-args`, 9 stats JSON (only the `coalesce` key), gtest list |
| 2 | Delta-write mode and generic devices | done | 54b15dc (backlog: 8d8f0f3) | none needed (no reachable behaviour) | none: bit-identical, counters identical |
| 3 | F-03 brief, FEATURES line, B-184 | done | b358373 | repro `backlog/repro/b184-mlprobe-tran-start-multilambda.cir` | none (docs, backlog) |
| 4 | Gate, this report | done | this commit | see "Final results" | — |

### 1. No-coalescing switch (8c17275)
- **Removed:**
  - `--no-coalescing`, `.options coalesce`;
  - `SPECSConfig::coalesce` and `coalesce_explicit`, `OpticalOutputPortConfig::m_coalesce`;
  - the port's FIFO queue (`m_fifo_queue`, its typedef and its branches in `on_data_ready()`,
    `delayedWriteEventDriven()`, `immediateWriteFrequencyDomain()`, `drop_queue()`, `isempty()`, `pending()`).
    These functions are back to their pre-D1 form; the stats counting added in D1 stays.
  - the `"coalesce"` key of the `options` object of `--stats`.
- **Kept:** the event cap (`--max-events`, `.options maxevents`) and its cases, and the `coalesced` counter.
- **`.options coalesce=0` is now an unknown key, handled by the existing path:**
  `Unknown keyword: coalesce (value: 0 (INTEGER))` on stderr, exit 1. New golden case
  `err/f15-options-coalesce-removed`. The removed flag also has a case, `err/f15-no-coalescing-removed`: the args
  parser prints `Flag could not be matched: no-coalescing` and the help, exit 1.
- **Docs:**
  - `doc/numerics.md`: the table row is gone and the CLI-priority note no longer cites the switch. The
    "Expert options (ablation)" section is now "Event cap (expert option)"; "no coalescing" is no longer listed
    as a use of the cap. The stats example has no `coalesce` key.
  - `doc/netlist.md`: the `.options` list and the CLI table.
  - `CHANGELOG.md` `[unreleased]`: the D1 entry now describes only the event cap.
  - `tests/golden/README.md`: case counts.
- **Golden:** 13 cases re-recorded, plus the 2 new ones. Two cases were removed: their baseline directories and
  their ids in `manifest.json` `cases`; `run_golden.py` reports such leftovers as STALE. The netlist
  `tests/golden/netlists/f15-coalesce-off.cir` was removed too. The `rerecords` entry has the full note.
- **Proof that the stats files lose only that key:** after removing `, "coalesce": true` from the 9 baseline
  files, they are identical to the new output (the case.json differences are the byte counts and sha256 of those
  files). `cli/help` and `cli/no-args`: exactly the 6 help lines of the flag are deleted, nothing else changes.

### 2. Delta writes and generic devices (54b15dc)
- **Removed:**
  - `OpticalOutputPort::m_use_deltas` and its three branches. The processed entry and a merge into a pending
    entry now always take the written value; with the flag always false, this is what already ran.
  - `src/devices/generic_transmission_device.{h,cpp}` (with `TransmissionMatrix`), `generic_2x2_coupler.h` and
    `generic_waveguide.h`;
  - the build-list entry and the include in `src/specs.cpp`;
  - the commented-out instantiation blocks in `src/tb/directional_coupler_tb.cpp` and `src/tb/wg_tb.cpp`.
- **Docs:**
  - `doc/numerics.md`: the `coalesced` row no longer mentions "devices that write deltas";
  - `CHANGELOG.md`: a "Removed" entry under `[unreleased]` (precedent: v0.2.0 also listed unused devices it
    removed).
- **Backlog:**
  - B-040 (the GenericWaveguide Taylor coefficients) is obsolete;
  - B-069: its GTD part is obsolete; OCTANE and BitstreamSource stay open;
  - B-061: a note that one of its locations is gone.
- **Checks:** golden 228/228 strict with no re-record. `stats-record` before (2140dd4) and after: identical on
  every pre-existing case.

### 3. Roadmap and backlog (b358373)
- **F-03:**
  - `discover/scout/F-03-sparameter-device.md`: an update note (generic device removed on 2026-10-23, 54b15dc;
    F-03 to be written from scratch as a state-holding, per-wavelength device; former code at cc1e52e);
  - the problem paragraph is in the past tense; design point 3 describes a new `SParamDevice`, not a refactor;
  - the prerequisite, touch points, effort line and the Taylor-form open question are updated.
  - `discover/scout/FEATURES.md`: the prerequisite of the F-03 line is updated.
- **B-184 (new): MLPROBE at the start of a `.tran` samples only the wavelength last written to its net.** Med,
  bug, needs-design, open.
  - Verified with a minimal repro: two 1 W CW sources at 1.55 and 1.56 µm into a merger, an MLPROBE on its
    output listing both wavelengths. 1550 nm reads 0.5 W from t = 0. 1560 nm reads 0 for the whole run (VCD),
    and the CSV has no row for it. Same with the clang build at 2140dd4 and the gcc build at 54b15dc.
  - Cause: the B-048 sampling at enable reads the net. A net holds one `OpticalSignal` (the last write, of any
    wavelength), and the CW sources never write again.
  - The issue file gives two fix options; both change traces. Not fixed.

## Decisions I had to take
- **`--stats` `format_version` stays 1** although a key of `options` is removed: the format was added in D1 and
  never released.
- **A golden case for the removed CLI flag too** (`err/f15-no-coalescing-removed`), next to the `.options` case
  the prompt asks for. It pins the CLI behaviour change.
- **`m_desired_fields` removed with `m_use_deltas`.** Only the delta mode read it; otherwise it was only
  cleared. The commented-out debug line that printed it now prints `desired`.
- **`OpticalSignal::operator+=`/`+` are kept.** The delta merge was one user; the crossings still use them
  (B-027).
- **Backlog statuses:** B-040 is marked obsolete without a fix. B-069 stays open (partly obsolete), because
  OCTANE and BitstreamSource are not in this decision.
- **`tests/golden/README.md` counts:** the total was already stale before this phase (226, while there were 227
  `specs` cases since D2 added `regress/f15-dispersive-waveguide`). Corrected to 227 (err 82, regress 100).
- **The `rerecords` history in `manifest.json` still lists the two removed case ids** in older `only` regexes.
  That is history and is left as is.
- **Bench not run:** item 4 of the prompt does not ask for it, and the removed branches are one well-predicted
  test per write or entry.

## Found, not done
- The 2026-10 audit documents (`discover/01-architecture-engine.md`, `02-devices-physics.md`,
  `05-paper-history-conventions.md`) still describe `GenericTransmissionDevice` and `m_use_deltas` with their
  2026-10 line references. Like PD2, I left these audits as they are.
- The PD2 report's remark that `doc/numerics.md` claimed "MLPROBEs and per-wavelength values do not change"
  without coalescing is moot: the paragraph is gone.
- `run_golden.py` has no command to remove a case; it only reports STALE ones. Removing a case means editing
  `manifest.json` `cases` and deleting the directory by hand.
- `src/test/optical_output_port.cpp:576`: a `-Wdangling-else` warning from an `EXPECT_EQ` under an unbraced
  `if`. It was there before this phase.

## Final results (tip b358373 + progress/report commits)
- `pixi run test gcc`: 12/12 OK. `pixi run test asan-clang`: 12/12 OK, no sanitizer message in the test log,
  `lsan.supp` unchanged.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: **228 passed**, 0 failed, 0 new, 0 missing. That
  is 227 `specs` cases (225 pre-existing, minus 2 removed, plus 2 new) and gtest.
- clang release, asan-gcc, asan-clang: `run_golden.py record` with the build at 2140dd4 and at the tip, then
  `compare --rtol 0 --atol 0 --print-slack 0`:
  - the same 11 cases differ in all three: `cli/help`, `cli/no-args`, and the 9 stats cases;
  - after removing `, "coalesce": true` from the "before" recordings, only `cli/help` and `cli/no-args`
    differ (the 6 deleted help lines);
  - 223 of 223 other pre-existing cases are bit-identical.
- Event counters (`stats-record`, gcc, 2140dd4 vs tip, `stats-compare --ignore '^options\.coalesce$'`): 225 of
  225 cases identical, 133 of them with statistics. Without `--ignore`, the only difference is
  `options.coalesce: true -> (absent)`.
