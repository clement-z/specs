# P3 report — devices and physics (branch `ws/devices`)

Sprint 2026-10b, phase P3, on top of `sprint/2026-10b` @ 51f0ca2 (P1 and P2 merged). Prompt:
`discover/orchestration/2026-10b/P3-devices.md`. All four items are **done**. One part of item 4 is **held back**:
the decided B-014 *error* for delays that round to 0 would change an existing case's exit status, so it waits for
sign-off on branch `P3-ready/b014-error`, ready to cherry-pick. Nothing was pushed.

How the work was split: four Opus implementers ran in parallel, one per item, each in its own
`/tmp/P3-wt/<name>` worktree (branches `P3-wip/*`). They committed code, tests and netlists only. Sonnet agents
wrote the CROSSING and PDET sections of `doc/netlist.md` and `doc/numerics.md`. The orchestrator did the rest:
reviewed every diff, cherry-picked onto `ws/devices` in the order 1→4, re-checked every golden change
independently (own VCD checker, line diffs), recorded only the predicted cases, and kept the bookkeeping
(CHANGELOG, issue status, `backlog.md`, progress, golden README).

## Items

| # | item(s) | outcome | commit | tests added | golden impact |
|---|---|---|---|---|---|
| 1 | B-032 + B-033 + B-159 crossing | done. **B-032:** passive S-matrix, x = 10^(-\|xtalk\|/20) (0 if not given), t = 10^(-att/20)·sqrt(1−x²), through t, cross j·x, reciprocal. **B-033:** per-λ memories and one thread per input, like the coupler. **B-159:** `CrossingBi` instantiated when any net is bidirectional | 04c7819 (status b8c6c5a) | gtest `CrossingModel.*` (7: coefficients, sign, NaN default for OCTANE, passivity for 16 relative phases, two λ, bidirectional reciprocity); golden `regress/b032-crossing-passive`, `b032-crossing-isolation-sign`, `b033-crossing-two-lambda`, `b033-crossing-two-lambda-tran`, `b159-crossing-backward` | only the 4 cases with a crossing: `regress/b147-crossing-out-{bidir,explicit-wg,explicit-splitter,explicit-splitter-first}`. In their OP listings, a, b and d are split into :0/:1 slots (the crossing is now bidirectional there). Every other OP line, the rest of stdout and stderr are identical, and the netlist headers are updated. `err/b148-crossing-args` is unchanged |
| 2 | B-049 + B-050 event-driven PDET | done. `p_readout` is written on input events and at the enable. Every Ts only while ≥ 2 non-zero λ beat, written only beyond max(abstol, reltol·\|last\|). Single λ uses the exact R·Σ\|E\|². `sc_pending_activity()` is removed. Enabled by default; `.op`/`.dc` disable detectors before their first `sc_start()`. `detector_tb` enabled, self-checks 3/3 | be96f26 (status 8e788e3) | gtest `Detector.*` (7, all fail on the old code); golden `regress/b049-pdet-drives-net`, `b050-pdet-two-open-tran` (open-ended `.tran`, 2 PDETs, ends: "Simulated 2 ns"), `b050-pdet-beat` | `tb/det`, `tb/detector`. Stdout: readouts printed with 3 decimals, 3 more readout events, the "Failure!" block gone, "Success rate: 3/3". VCD: the noisy readout signals are in `vcd_ignore_values` (random seeds); every other signal is unchanged. No other existing case runs an enabled PDET. **Shown:** `a3-pdet-readout` with `.tran 10e-9` goes from 6 to 1 readout records, max deviation 2.2e-16 at the old sample times, and out2 phase goes 0 → 1 rad. `a3-pdet-two-hang`: was killed by the timeout (207 MB VCD), now ends at 1 ns |
| 3 | B-048 probe sampling (IC-proof) | done. PROBE/MLPROBE threads also wake on the enable posedge and sample the current value of their net. `runTRANAnalysis()`: (a) OP (incl. `.nodeset`), (b) new `applyInitialConditions()` (`ic_orders`), (c) probe, ML-probe and detector enables. Undriven nets are skipped silently at the enable | 65129a7 (status 48e37ed) | gtest `Probe.*`, `MLambdaProbe.*` (6), `TranStart.ProbesSampleAfterOperatingPointAndInitialConditions` (real `runTRANAnalysis()`: each trace changes exactly once, at t = 0, to the post-OP/post-IC value; fails if IC is applied after the sample); golden `regress/b048-probe-cw-tran`, `b048-mlprobe-cw-tran`, `b048-undriven-tran` | the 10 existing `.tran` cases whose VCDs change: `regress/b031-mzi-bidir` (204 signals), `b144-two-sources-one-net` (9), `b144-uni-writer-first` (11), `b144-uni-writer-second` (11), `b145-upgrade-reader`, `-ref`, `-case2`, `-case2-ref` (41 each), `b147-crossing-out-bidir` (23), `b150-include-in-subckt` (59). Each changed signal was checked by two checkers: old value 0 on [0, t1), new value constant there and equal to the OP value, identical waveform from t1 on; stdout and stderr unchanged. `tb/*`, `cir/*` unchanged |
| 4 | B-021 + B-014 + B-124 numerics | done, except the B-014 error (held back, below). **B-021:** emission rule commented; \|last\| = 0 explicit; `.options snapfactor` (default 10, 0 = off, < 0 error). **B-014:** dead check removed; warning once per waveguide when a delay is rounded by > 10 % or to 0, only where delays are honoured. **B-124:** tight-reltol case; `doc/numerics.md` | c527d7d (status 66a626a) | gtest `OpticalOutputPort.EmissionGateWithZeroLast`, `.SnapFactorFromConfig`, `DelayRounding.*` (2), `DelayRoundingDeathTest.CheckDelayRounding`, `WaveguideDeathTest.DelayRoundingReportedOncePerDevice`; golden `regress/b021-snapfactor{,-default,-zero}`, `err/b021-snapfactor-negative`, `regress/b014-delay-rounding-warning`, `regress/b014-delay-rounds-to-zero`, `regress/b124-allpass-ring-{default,tight}-reltol` | stderr gains rounding warnings, and nothing else changes, in `cir/add_drop_pyspecs`, `opt/tracefile-o`, `regress/b006-empty-output` (2 lines: 2.27 ps ring arms at 1 ps, −11.7 %) and `regress/b031-mzi-bidir` (18 lines: MZI-internal arms of 43 fs → 0 and 88 fs → 100 fs; stderr declared unordered, since creation is pointer-ordered, B-016). Stdout, VCD and exit status are identical. Plus `regress/b033-crossing-two-lambda` (new in P3): `unordered_streams` stdout, see decision 9 |

The gtest case was re-recorded with each item; it now has 45 tests, all PASS. Every recording appended a
`rerecords` entry to `manifest.json` citing its item.

### Hand-checked values of the new cases
All match the hand values in their netlist headers.

| case | expected = observed |
|---|---|
| b032-crossing-passive (2 × 1 W coherent, att 0, xtalk −20) | out1 = out2 = 1 W at 0.1002 rad (total 2 W). Was 1.21 W + 1.21 W |
| b032-crossing-isolation-sign (1 W, att 1, xtalk +10 positional / −10 keyword) | through 10^(-0.1)·0.9 = 0.714895 W at 0, cross 0.1 W at π/2, for both. xtalk=10 used to give 10 W |
| b033-crossing-two-lambda (.op) | out1 = 1 W @1550 nm, out2 = 1 W @1560 nm. Was out1 = 0 W @1560 |
| b033-crossing-two-lambda-tran (xtalk −10) | MLPROBE out1: 0.9 @1550 (1–5 ps), 0.1 @1560 at π/2 (2–6 ps); out2 mirrored; stderr empty (was 6 × "Attempted to sum") |
| b159-crossing-backward (1 mW backward into out1, 4 mW into in2, xtalk −10) | a:0 = 0.9 mW, b:0 = 0.1 mW at π/2, c:1 = 0.4 mW at π/2, d:0 = 3.6 mW |
| b049-pdet-drives-net | readout 1 mA from t = 0; PSHIFT out2 phase 1 rad (sensitivity 1000 rad/A) |
| b050-pdet-two-open-tran | 0.5 mA, then 2 mA from 1 ns, on both PDETs; the run ends |
| b050-pdet-beat (1550/1551 nm) | readout = 0.625 + 0.5·cos(2π·124.703 GHz·t) mA, recorded every 1 ps (1.125, 0.979212, 0.626866, ...), to ~1e-14 relative |
| b048-probe-cw-tran | PROBE1 and PROBE{ROOT/out} at t = 0: 1 mW, 0.0316228 √W, −0.4053668 rad, 1.55e-6 (= OP listing) |
| b048-mlprobe-cw-tran / b048-undriven-tran | power@1550 = 1 mW at phase 0.5 from t = 0 / undriven probes stay 0, nothing printed |
| b021-snapfactor{-default,,-zero} (abstol 1e-2) | out_a/out_b: 0/0 (default 10), 0/0.06 √W (4), 0.03/0.06 √W (0); out_c 0.2 √W always |
| b124-allpass-ring default / tight reltol | 0.995098 W / 1.000000 W (circulating 24.563207 / 24.626058 W, exact 24.626059) |
| b014-delay-rounding-warning / -rounds-to-zero | warning "+87.4 %" (53 fs → 100 fs) / warning "rounds to 0" (13 fs); simulation unchanged |

## Held back, needs sign-off: the B-014 error for delays that round to 0
The decision says to "error when a non-zero delay rounds to 0". Implemented as decided, it makes the existing
`regress/b031-mzi-bidir` exit 1. Its MZI-internal arm waveguides (43.4 fs, created by `MZI::init()`) round to 0
at the default 100 fs. They are feed-forward, so the rounding is harmless there. The phase table predicted
"stderr warnings only, no value changes", so the rules require holding it back.

On `ws/devices` such delays get their own **warning** ("rounds to 0 ... the delayed events happen in the same time
step"), and the code has a `TODO(B-014)` where the error belongs. The error is ready on **`P3-ready/b014-error`**,
two commits on top of c527d7d, 134/134 golden `--atol 0` with its own baseline:
- cfe0eb0: the error (clean message, exit 1, before the simulation). The case becomes
  `err/b014-delay-rounds-to-zero`. b031 gets `.options timescale=-14`: arms 40 fs / 90 fs, no message, OP values
  identical; stdout gains the directive and timescale lines, and the VCD timescale is 10 fs.
- cc1614c: the re-recorded baseline.

Options:
- (a) take the branch as is (`git cherry-pick cfe0eb0 cc1614c`);
- (b) error only for netlist waveguides, warning for device-internal ones;
- (c) keep the warning.

This is written in `backlog/B-014-*.md` (status "warnings fixed; error held back, needs-human").

## Decisions taken
Choices the decision record does not cover. None changes results beyond the decisions.
1. **B-032, positive xtalk is read as an isolation.** The decided formula uses \|xtalk\|, so the sign is
   ignored: `xtalk=20` = `xtalk=-20` = 1 % on the cross port.
   - `att` is the excess loss on the through path only, exactly as decided, so \|t\|²+\|x\|² =
     10^(-att/10)(1−x²) + x².
   - `att` is not range-checked: a negative value is a gain, as before; it is not decided.
2. **B-159 changes the slot listing.** With `CrossingBi` instantiated, a crossing on a bidirectional net
   upgrades all four of its nets. That is why the b147 cases list a, b and d as :0/:1 slots. Their values
   follow the next-free-slot convention of P2 and the b147 headers explain them.
3. **PDET, when the readout is written.**
   - Input events always write. Only the periodic beat samples are gated by the tolerance, using the global
     `default_abstol`/`default_reltol` applied to the current (A).
   - The readout is also written at the enable posedge; no write at initialisation when enabled by default
     (that would write the random dark-field value in testbenches).
   - Periodic samples come Ts after the last wake, not on a fixed grid, as before.
   - "was enabled" is still printed (existing `.tran` stdout).
4. **PDET, enable flow.** `.op`/`.dc` move their detector-disable loop before the first `sc_start()`, so the
   analyses behave as before now that the constructor enables detectors. `.op`/`.dc` keep detectors disabled:
   the decision's "outside `.tran`" is read as outside the analysis flows (testbenches, OCTANE).
5. **`detector_tb`.**
   - The enable is made explicit.
   - The noisy readouts are printed with 3 decimals, 28σ above the noise, so stdout is reproducible. The
     checks still use the raw values.
   - The third self-check ("different noise seeds") used an absolute 1e-7 threshold on two ~1 A readouts with
     σ ≈ 1.8e-5 each, so it failed in about 0.3 % of runs. That would make the golden case flaky. The
     threshold is now 1e-12: identical readouts (noise off, same seed) still fail it.
   - The readout VCD signals of `tb/det`/`tb/detector` are in `vcd_ignore_values` until P4 (F-01) seeds the RNG.
6. **B-048, IC structure.**
   - `applyInitialConditions()` writes a new `SPECSConfig::ic_orders`, with the same type and mechanism as
     `nodeset_orders`, after the OP and before the probe enables. It is filled only from C++: `.ic` stays a
     parse error (B-161).
   - What `.ic` itself means (hold vs one-shot, UIC) is left to whoever implements `.ic`.
   - The order test drives the real `runTRANAnalysis()`.
7. **B-048, undriven nets.** A never-written net holds the default `OpticalSignal`, whose wavelength id 0
   aliases the first registered wavelength (B-012), so `isnan()` cannot detect it. At the enable, the probe
   therefore skips a (field 0, id 0) value silently. A `.nodeset` golden case is impossible because `.nodeset`
   never parses (B-162), so `b048-undriven-tran` was added instead; the gtest covers `.nodeset` through
   `nodeset_orders`.
8. **B-021 / B-014 implementation choices.**
   - The option is named `snapfactor`. It has no CLI flag and no `printConfig()` line, either of which would
     change `cli/help` and every testbench stdout.
   - Delays are checked once per device, in `start_of_simulation()`. The check reads the port *configuration*
     mode and `analysis_type`, because the OP overrides `m_mode` later. It compares against the port time
     step, i.e. tick × `.options resolution`, the grid events are snapped to.
   - Dispersive waveguides: only the nominal delay (1.55 µm) is checked.
   - Bidirectional waveguides: one message per device.
   - Threshold 10 % (`DELAY_ROUNDING_WARN_THRESHOLD`), chosen from the delays of every golden case, testbench
     and repro:
     - realistic parameters at 100 fs round by at most 5.95 % (default 100 µm waveguide);
     - 2.27 ps at 1 ps rounds by 11.7 %;
     - the B-014 case rounds by 87 %.
     - Rule of thumb: a delay of ≥ 5 time steps never warns.
9. **Line order of the new cases.** All new cases are recorded with strict line order. The one exception is
   `regress/b033-crossing-two-lambda`: its pointer-ordered OP listing (B-016) reordered when item 4 added a
   member to `OpticalOutputPort`, with values identical. It now declares `unordered_streams: ["stdout"]`, as
   the b147 cases do. The numerics cases with multi-net `.op` listings were declared unordered by their
   implementer. The final gcc, clang and gcc `-O0` builds give 134/134 with no reorder note.
10. **Docs.**
    - `doc/netlist.md` gained CROSSING, PDET, and "PROBE, MLPROBE and traced nets in `.tran`" sections.
    - New `doc/numerics.md`: it quotes the paper's "Temporal resolution" and "Absolute and Relative tolerance"
      paragraphs and gives the emission rule, the snap, the resonator error bound with the ring example and
      measured tables, how to choose reltol, the time resolution and the delay rounding. It is linked from
      `README.md`, `doc/mainpage.md` and the Doxyfile `INPUT`.
    - The cheatsheet PDF has no sources and is not regenerated (P8, B-080).
11. **B-124 error bound.**
    - The circulating-field error is between δ·r/(1−r) and δ/(1−r), with δ = max(reltol, abstol/\|E\|).
    - On resonance the output-power error is 2(1+t) times that.
    - For the example ring that is 45–49·δ; measured: 49·reltol.
    - At reltol 1e-9 the default abstol dominates (δ ≈ 2e-9).
    - All of this is documented.

## Found, not done
Crossing:
- **`upgrade_signal` assert.** P2 already noted that `assert(has_writer ^ has_reader)` aborts when a uni net
  already has a writer and a reader. It can now also fire for `CrossingBi`. No current case hits it; it matters
  for P4's name-ordered iteration.
- The OP listing shows one wavelength per net (B-011), so the b033 `.op` case uses an ideal crossing.
- A negative positional number is a parse error (`crossing1 a b c d 0 -20`). The keyword form works, or the
  positive form now that the sign is ignored.
- `kind()` prints "CROSSING (bidir)" for unidirectional instances too (pre-existing, element-level label).
PDET:
- **Last time step of an open-ended `.tran`.** SystemC does not dump it to the VCD. This affects every signal;
  it was hidden for PDET runs before, because the detector's ticks advanced time. `b050-pdet-two-open-tran`
  adds a later dummy event to show its 1 ns values.
- **Same-delta writes.** PDETs now really write, so two PDETs on one electrical net, or a PDET net with another
  writer, can hit an `SC_MANY_WRITERS` same-delta conflict that was hidden before.
- **Random draws.** Noise and dark-field values are still drawn at every computation, including gated beat
  samples. A detector seeing exactly zero field writes a random ~1e-40 A (B-056); this is P4 (F-01).
- **Permanent beat.** With one, an open-ended `.tran` never ends (physical; documented).
- **Elaboration-time writes.** Values written to a detector input before the simulation starts don't wake
  `on_port_in_changed` (pre-existing).
- **For P4:** once the RNG is seeded, drop `vcd_ignore_values` from `tb/det`/`tb/detector`.
Probes:
- **B-012.** The `isnan()` wavelength checks in the probes are dead code, because the default signal has id 0.
  Edge case: a real zero-field value at id 0 that a probe sees only at the enable is skipped, so its wavelength
  trace stays 0.
- **B-162.** `.nodeset` cannot be used from a netlist (syntax error).
- The B-048 issue file's "Golden impact: None" is stale; 10 cases changed.
- **MLPROBE print.** MLPROBE prints "received unsupported signal" in its per-event path (pre-existing). Since
  B-048 it also prints this once at TRAN start when the OP value's wavelength is not in its list. No golden case
  does this.
Numerics:
- `--abstol`/`--reltol`/`-R` have no effect on `-t` testbenches; they only become a `.options` footer for `-f`.
- A negative waveguide length gives a negative delay and an abort, rc 134 (B-035). The rounding check does not
  catch it.
- `.options reltol=1e400` (inf) passes the parser's `v > 0` check. With reltol = inf the old gate never
  emitted; the explicit \|last\| = 0 branch now emits the first value.
- The rounding check rounds the delay to tick × multiplier in one step. The real path rounds to the tick, then
  snaps the arrival time to the grid.
- GTD delays are not checked; GTD is unreachable from netlists (B-069). With `resolution` > 1, zero-delay events
  are also moved onto the grid (pre-existing; documented).

## Final results
HEAD before this report: 66a626a. Fresh builds with the prebuilt deps (`specs-deps-next`):
- `/tmp/P3-fgcc`: gcc 11.5, Meson defaults, asserts on;
- `/tmp/P3-fclang`: clang 19.1.7 through ccache;
- `/tmp/P3-fdebug`: gcc `--buildtype=debug`, `-O0`.

Results:
- **`meson test`:** 12/12 OK on all three builds (unit, 9 tb, golden-harness, golden).
- **Golden check** (`python3 -I tests/golden/run_golden.py check --specs <b>/specs --unit-tests
  <b>/src/test/unit_tests --baseline tests/golden/baseline -j 8 --atol 0`, strict order and coverage by default):
  **134 passed (0 with notes), 0 failed, 0 new, 0 missing, 0 skipped** on gcc, clang and gcc `-O0`. That is 133
  `specs` cases plus the gtest case with 45 tests, all PASS. Every recording also passed
  `record --repeat 3` (identical across runs).
- **Compiler warnings:** none in changed code. The remaining ones are pre-existing: `src/specs.cpp`
  strict-aliasing (`(int&)` casts in `sc_trace`), `value_list_source.cpp:40` unused variable, and the generated
  parser/scanner.
- **Golden baseline:** 114 → 133 `specs` cases. The 19 new ones: 5 crossing, 3 PDET, 3 probe, 8 numerics
  (7 regress, 1 err). Existing cases re-recorded, all predicted:
  - item 1: the 4 `b147-crossing-out-*`;
  - item 2: `tb/det`, `tb/detector`;
  - item 3: the 10 `.tran` regress cases listed above;
  - item 4: `cir/add_drop_pyspecs`, `opt/tracefile-o`, `regress/b006-empty-output`, `regress/b031-mzi-bidir`
    (stderr only);
  - gtest with every item.

## Branches and worktrees
- `ws/devices` holds the phase.
- `P3-ready/b014-error`: keep until B-014's error is decided.
- Implementer branches, all integrated by cherry-pick and safe to delete: `P3-wip/crossing` (496818c),
  `P3-wip/pdet` (81ccd4c), `P3-wip/probes` (8521e6b), `P3-wip/numerics` (7678d33; its 2b1902d is superseded by
  `P3-ready/b014-error`).
- The `/tmp/P3-wt/*` worktrees were removed.
