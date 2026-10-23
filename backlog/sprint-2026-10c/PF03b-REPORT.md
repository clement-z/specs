# PF03b report: SPAR follow-ups (F-03, branch `ws/spar2`)

Prompt: `discover/orchestration/2026-10c/PF03b-spar-followups.md`, plus item 8 from the coordinator (2026-10-23):
re-export the ring file on a 0.05 nm grid. Decisions: `backlog/DECISIONS-2026-10-21.md`, rows "SPAR delays", "SPAR
licence and manifest", "SPAR follow-up phase"; `backlog/sprint-2026-10c/SPAR-discussion.md`. Base: 48e510f.
Predictions per item, written before each re-record: `PF03b-progress.md`.

## Items

| # | Item | Outcome | Commit | Golden impact |
|---|---|---|---|---|
| 1 | Per-path delayed copies (`.tran`) | done | 7e9e03b | `regress/spar-ring-tran` (out at +0), `cir/spar-touchstone-tran` (same values, same-time rows swapped) |
| 8 | Ring file re-exported on a 0.05 nm grid (own commit, after 1) | done | 5e1b14f | `cir/spar-add-drop-ring` (<= 4.3e-11), `regress/spar-ring-tran` (drop at +1.721 ps) |
| 4 | Mixed interpolation near nulls | done | 368bf61 | `out` at 1.55 um in the two ring cases (<= 4.2e-11); gtest |
| 5 | Data files in the run manifest (own commit) | done | 805dc8c | the 160 cases with a manifest: `format_version` and `data_files` only |
| 3 | Grid-adequacy warning | done | 964b75a | none; new case `regress/spar-grid-warning`; gtest |
| 2 | Negative delay warning per path | done | 55d7997 | stderr of `regress/spar-ring-tran`, `regress/spar-grid-warning` |
| 6 | SiEPIC BDC fixture + golden case | done | cfe7da8 | new case `cir/spar-siepic-bdc` |
| 7 | Tests and docs | done | 1c206c2 (docs; the tests landed with each item) | none |

Order: 1, 8, 4, 5, 3, 2, 6, 7 (item 4 before 3 so that the grid check uses the final interpolant; 2 after 3 for
its grid hint). Item 5 was written by an Opus subagent in its own worktree (`/tmp/PF03b-wt/manifest`, branch
`PF03b-wip/manifest`), reviewed and cherry-picked; its baseline had been recorded before item 8, so the two ring
cases' manifests were re-recorded inside the same commit for the new file's SHA-256 (still only manifest lines).

### 1. Per-path delayed copies
- **Implementation: one internal timed queue per device** (`SParamDeviceBase::m_arrivals`, a priority queue of
  (time, sequence, path, wavelength, field), one `sc_event` notified for the earliest arrival, handled by the
  existing `SC_METHOD`, which first takes the arrivals due, then the inputs with an event). Chosen over internal
  delayed ports per path: those would add an emission gate (the decision says none on the internal stage), extra
  signals and delta cycles, and port counters per path to `--stats`; the queue holds one 40-byte entry per value
  in flight and nothing else.
- On an input event: a path without delay copies the input and writes its output at once (as before: `.op`,
  `.dc`, `delay=none` and the operating point before a `.tran` write exactly as before); a delayed path queues an
  arrival at t + tau. On arrivals, each changed (output, wavelength) is written once, without delay. Coalescing
  and the emission gate stay at the output port. The dominant-path selection and the per-output delay are gone;
  `delay=<s>` applies per path.
- A bug found while testing: during the operating point before a `.tran` the ports are in `NO_DELAY` mode while
  their configuration still says event-driven; queueing on the configuration would have lost the delayed paths of
  CW sources at t = 0. The device tests the port's actual mode; a test covers it (it fails on the first version).
- **Cost** (ring SPAR, both inputs lit, 2 x 200000 events 0.25 ps apart, gcc release, 7 runs each): user time
  1.02 -> 1.63 s, peak RSS unchanged (~27 MB); output-port writes the same (1199194); emitted events 799596 ->
  1198315 (paths with different delays now change the outputs at different times: more, correct, events);
  pending entries per port max 6 -> 1 (the delays now live in the device's queue). Per input event: one port
  write per zero-delay path, one queue entry per delayed path, then one port write per changed (output,
  wavelength) and arrival time. The golden ring case's port counters are identical (7 writes).
- Side effect: a delayed output is written one delta cycle later than before (arrival, then a write without
  delay): in `cir/spar-touchstone-tran` the CSV rows of the WG probe and the SPAR probe at the same time swap
  (values bit-identical). I found this in the first strict run, not before (noted in the progress file).

### 8. Ring file on a 0.05 nm grid
- Exported with `utils/sparam_export.py` and the committed file's settings (tool defaults `abstol=1e-12
  reltol=1e-9`), 1.535-1.565 um by 0.05 nm, 601 points; the command is in `tests/sparam/README.md` ("Ring data";
  the test referred to a command the README did not have). The 1 nm export still reproduces the old file byte for
  byte. Netlists of both golden cases unchanged.
- **Not bit-identical at the 31 sweep points**: SPECS's `.dc` sweep accumulates its wavelengths, so only the
  first point (1.535 um) is bit-identical to a point of the new grid; the other 30 are 2e-14 to 4e-14 (relative)
  off, and the device interpolates there. Fields change by <= 4.3e-11 (predicted <= ~1e-10). My prediction also
  said the default tolerances would pass: they do not at one point (`out` at 1.55 um, |E| = 0.002, 2e-8 relative).
- `regress/spar-ring-tran`: drop at 2.721 / 21.721 ps (in->drop 1.721 ps, exact 1.72 ps; 1.324 ps on the 1 nm
  grid), out unchanged in time (in->out -37.3 ps on this grid, clamped).
- `test_sparam.py`: re-export check on the new grid (difference 0); same-grid round trip on the new grid in the
  three formats, bound 1e-6 kept (7.23e-7); between-points reads the committed file.
- **The grid warning does not fire on the new file** (coordinator's expectation was that it would at the 4 pm
  null): with item 4's interpolation, the leave-one-out error is 0.0039 (threshold 0.02), and the field between
  grid points is within 1e-3 of the circuit at the null. What the grid does not resolve there is the phase
  feature, i.e. the group delay (-37 ps vs -621 ps exact), which the S-based check does not test. With
  magnitude/phase interpolation the null would give 0.067 and warn. Documented in `tests/sparam/README.md` and
  `doc/sparam.md` ("Grid check"); the old 1 nm file warns (0.71), shown by the new case `regress/spar-grid-warning`.

### 4. Mixed interpolation
- Per interval between grid points: real and imaginary parts when |S| at either end is below **0.07 x the
  entry's largest |S|**, magnitude and unwrapped phase otherwise; PCHIP uses its own slopes for re and im
  (Fritsch-Carlson on each). Grid-point values unchanged bit for bit (magnitude and phase there), so S is
  continuous. The group delay is still from the phase table.
- Measurements (stdlib prototype of the device's interpolants, cross-checked against the C++ results; field error
  at the midpoints of the grid against exact exports of the circuit, all used entries):

  | device, grid | interp | magnitude/phase | 0.02 | 0.05 | **0.07** | 0.1 | 0.3 | re/im everywhere |
  |---|---|---|---|---|---|---|---|---|
  | ring k=0.85 (the test ring), 0.05 nm: all / where \|S\| >= 0.1 | linear | 0.031 / 4.0e-4 | 0.028 / 4.0e-4 | 9.7e-4 / 4.0e-4 | 9.7e-4 / 4.0e-4 | 9.7e-4 / 9.5e-4 | 9.7e-4 / 9.5e-4 | 9.8e-4 |
  | same | pchip | 0.026 / 5.7e-4 | 0.024 / 5.7e-4 | 2.0e-3 / 5.7e-4 | 6.6e-4 / 5.7e-4 | 6.6e-4 / 2.7e-4 | 6.6e-4 / 2.7e-4 | 6.7e-4 |
  | unbalanced MZI (FSR 5.6 nm), 0.2 nm | linear | 0.070 | 0.064 | 0.023 | 0.023 | 0.025 | 0.032 | 0.079 |
  | same | pchip | 0.062 | 0.055 | 0.016 | 0.016 | 0.016 | 0.028 | 0.053 |
  | ring k=0.4 (narrow), 0.05 nm (under-resolved) | linear | 0.23 | 0.23 | 0.23 | 0.21 | 0.21 | 0.043 | 0.043 |
  | ring k=0.85 on 1 nm (under-resolved) | linear | 0.54 | 0.54 | 0.54 | 0.54 | 0.54 | 0.50 | 0.27 |

  0.06-0.08 is where both resolved devices are at their best with both interpolants (lower leaves the nulls to
  magnitude/phase; from 0.1 the MZI's rotating paths are cut by chords); 0.07 is its middle. Under-resolved data
  stay bad with any threshold: that is the grid warning's job.
- The PF03 metric (ring 0.05 nm, 30 points 0.025 nm off the grid, `test_sparam.py` between-points): at the null
  0.0202 -> **9.35e-4** (linear), 0.0145 -> **5.19e-4** (pchip); where |E| >= 0.1 unchanged (3.84e-4, 9.56e-5).
  The bound "anywhere" is tightened from 0.03 to 2e-3. Phase-convention tests keep their bounds (3.62e-13 `.dc`
  between points, 2.8e-15 `.tran`).
- `interp=pchip`: the same threshold and per-interval rule; PCHIP on magnitude/phase or on re/im.

### 5. Data files in the run manifest
- `data_files` (path as opened + SHA-256, as `netlist_files`), right after `netlist_files`, **always present**
  (`[]` when empty), registered where each file is opened (`SPECSConfig::record_data_file()`: SPAR after
  `SParamData::load`, VLSRC/EVLSRC `setValues(file)`). `BitstreamSource` reads a file but no element or testbench
  creates it: not covered.
- **`format_version` 1 -> 2** in the run manifest and in the run table manifest: a reader must be able to tell
  "no data files" from "not recorded". So every recorded manifest changes, and only in those two places (160
  cases, checked mechanically against HEAD by a script outside the repo: re-inserting version 1 and removing the
  key gives the old bytes; the case.json hashes match; two `rerecords` entries appended, the second for item 8's
  file).
- Run table (F-02): the parent process of a plan does not build the circuit, so it takes the data files from the
  flattened elements (`ParseElement::data_files()`; SPAR resolves its path with the function `create()` uses).
  A resume against a changed data file is refused (unit test). A data file name cannot depend on a swept
  parameter (string values only; `.step`/`.mc` give numbers).
- Docs: `doc/reproducibility.md`, `doc/netlist.md` (run table), CHANGELOG.

### 3. Grid-adequacy warning
- At load time, per tabulated entry, the leave-one-out error of each interior grid point with the device's own
  interpolant (mixed; PCHIP slopes from the points around it, as the grid without that point would give them);
  one warning per device for the worst path **the device uses** (unidirectional: input to output), naming the
  ports, the wavelength and the error. **Threshold 0.02** (field): leaving a point out doubles the spacing, so the
  error is ~4x the one between the actual grid points (measured ratio on the ring: 0.0039 vs 9.7e-4); 0.02 stands
  for ~0.005 between points. Measured: delay line 4e-13, SiEPIC BDC 0.0026, ring 0.05 nm 0.0039 (pass); ring
  1 nm 0.71, MZI 0.2 nm 0.11, ring k=0.4 0.42 (warn). Not applicable to the Taylor form.
- Tests: gtest (an all-pass ring sampled 10 times per FSR warns at its nulls; a delay line turning 1 rad per point
  gives < 1e-12), golden `regress/spar-grid-warning` (11-point 1 nm export of the ring).

### 2. Negative delay warning
- Still clamped to 0, per path. Reported in `.tran` once per path **when the path is first used** (an input event
  on its input), if at least half a time step: path (port names), wavelength, value; "probably an artefact of the
  grid" when the delay depends on a grid point flagged by item 3 (the 4 points of the finite differences around
  the interval); "acts as a connection without delay, also inside a feedback loop". Decision: reporting at first
  use rather than when a wavelength's delays are computed, so that the paths of an unlit input (the ring's `add`)
  are not reported. The B-014 rounding warning stays once per device.

### 6. SiEPIC BDC
- `tests/sparam/data/siepic/EBeam_1550_TE_BDC.sparam` copied unchanged (SHA-256 646d224f..., verified), next to
  `NOTICE` (URL, path, commit 363fbb52, copyright, the MIT text of `LICENSE.md` verbatim). Golden case
  `cir/spar-siepic-bdc`: `.dc` of both outputs over 1.5-1.6 um by 2 nm (51 points); `test_sparam.py`
  `siepic-bdc/split`: within 0.02 of 0.5 over 1.54-1.56 um (0.502 / 0.490 at 1.55 um), P1 + P2 <= 0.9964, no
  warning. Only these two files were copied from outside the repository.

### 7. Docs
`doc/sparam.md` (Interpolation, Group delay, Grid check, Behaviour in the simulation: per-path delays, negative
delays, process), `doc/netlist.md` (SPAR `delay`, `interp`, load-time warnings, run table), `CHANGELOG.md`
`[unreleased]` (SPAR entry, manifest entry), F-03 brief status block, `tests/sparam/README.md`,
`tests/golden/README.md` (case counts).

## Decisions I had to take (not covered by the decisions file)
1. Item 1: one timed queue per device (not internal ports); zero-delay paths write at once; arrivals at one time
   write each changed output once.
2. Item 4: threshold 0.07 relative to the entry's largest |S| (relative, so a uniformly weak path, e.g. a
   -30 dB crosstalk with a delay, keeps magnitude/phase); the same threshold for PCHIP; grid-point values kept as
   magnitude/phase (bit-identical to before).
3. Item 3: threshold 0.02; only the paths the device uses; one warning at element creation (per device).
4. Item 2: report a path when first used, not when computed.
5. Item 5: key always present; `format_version` 2 for the run manifest **and** the run table manifest.
6. Item 8: the between-points check reads the committed file instead of exporting the same grid again.
7. The coordinator's expectation that the grid warning fires on the 0.05 nm file is not met (above); I kept the
   decided S-based criterion and documented it rather than adding a delay criterion.

## Found, not done
- **Grid check vs group delay**: the check tests S only; the ring's through null passes on 0.05 nm while its group
  delay is wrong (-37 ps vs -621 ps). A delay criterion (e.g. leave-one-out on the finite-difference delay, or the
  phase step relative to |S|) would need a maintainer decision.
- **Rotating paths with nulls** (an unbalanced MZI, 28 points per FSR): neither interpolant is good (0.023); a
  delay-removed interpolation (interpolate S exp(-j omega tau) with the path's mean delay taken out) would be
  better. Numerics choice, needs-human.
- Netlist headers of `regress/spar-ring-tran` ("delay=auto per output, the group delay of the largest path") and
  `cir/spar-add-drop-ring` ("negative group delays ... are reported", which `.dc` never did) are stale; kept
  unchanged as the coordinator asked (case notes in `cases.json` updated instead).
- The SPAR discussion says three golden cases use the ring file: two do.
- flex: a VLSRC with a long inline `values=[...]` list (20000 entries) fails with "input buffer overflow, can't
  enlarge buffer because scanner uses REJECT" (pre-existing; value files work).
- `tools - specs:run-table` is skipped (exit 77) on this machine: `tests/tools/test_run_table.py` needs numpy and
  pandas; Meson's `python3` (pixi default environment) has no numpy, the system one no pandas, pixi's `py`
  environment is not installed in the worktree.
- VLSRC value files are opened relative to the working directory, SPAR files next to the netlist first
  (inconsistent; documented as is).
- `doc/reproducibility.md`'s example hashes are illustrative (made-up) values.
- Pre-existing compiler warnings: `-Wstrict-aliasing` in `src/specs.cpp` (sc_trace lines), `-Wdangling-else` in
  `src/test/run_plan.cpp` (EXPECT_ inside if).

## Final results (HEAD of `ws/spar2`)
- `pixi run test gcc`: 16 OK, 0 fail, 1 skipped (`tools - specs:run-table`, exit 77: numpy/pandas, see above).
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: **275 passed** (273 + 2 new cases), 0 failed.
- `pixi run test asan-clang`: all OK but the same skip, no sanitizer message; `lsan.supp` unchanged. Run at every
  item (7e9e03b, 5e1b14f, 368bf61, 805dc8c, final c4afca8).
- Strict `compare`, before (df47f0c) vs final, gcc, clang and asan-gcc: the **same set** in all three: 160 changed
  (the cases with a run manifest; among them the 3 whose outputs changed, `regress/spar-ring-tran`,
  `cir/spar-add-drop-ring`, `cir/spar-touchstone-tran`) and 2 new (`regress/spar-grid-warning`,
  `cir/spar-siepic-bdc`). Per item, clang and asan-gcc showed the same cases as gcc (items 1, 8, 4: 2, 3, 2
  cases; item 5: 160).
- `stats-compare` (gcc) before vs final: 271 same, 2 new, 1 differs: `regress/spar-ring-tran`, the last emission
  times of `out` (21.324 -> 20 ps, item 1) and `drop` (21.324 -> 21.721 ps, item 8), as predicted.
- Worktree of the subagent: `/tmp/PF03b-wt/manifest` (branch `PF03b-wip/manifest`, its commit 1aa6140 is
  805dc8c here with the ring manifests corrected).
