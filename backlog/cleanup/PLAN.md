# Cleanup phase (ws-cleanup) — plan

Sprint 2026-10, phase 3. Branch `ws/cleanup` from `sprint/2026-10` @ 8928f8f. Brief:
`discover/orchestration/ws-cleanup.md`. Report: [`REPORT.md`](REPORT.md).

## Scope
The 22 items the fix phase deferred (`status: deferred to ws-cleanup`) plus two `#if 0` arms found by grep
(X-1, X-2). Each item's backlog file has the exact change, with the second checker's corrections; this plan only
adds what changed since (line drift, overlaps between items) and assigns every file to exactly one batch, so
cherry-picks cannot conflict.

Out of scope (already logged, not touched): the Uni/Bi duplication (B-046, `needs-design`), VLSRC/EVLSRC
duplication (B-074), `SC_HAS_PROCESS` (B-030), `using namespace std` in headers (B-024), the strict-aliasing
warnings at `specs.cpp:392-395` (`sc_trace` of enums, B-029), the PhaseShifter "nan wavelength" lines (B-042,
shifts printed IDs), the cross-λ branches of `OpticalSignal::operator+=/-=` (B-033), the `(30points)` message
(changes 5 golden stdouts, not a debug print), the `ptrough` probe name (VCD scope name), the wildcard import in
`pyspecs/specs.py` (public API), the OCTANE/GTD family (B-069, `needs-human`).

## Baseline (ws/cleanup @ 8928f8f, clean GCC 11.5 build, `-Dold_tb=true`)
- Compiler warnings: 18 lines, **12 unique**: `generic_2x2_coupler.h:35 lambda0`, `generic_transmission_device.cpp:221
  delay maybe-uninitialized`, `specs.cpp:209 ic_order`, `specs.cpp:392-395` strict-aliasing ×4,
  `freqsweep_tb.cpp:60-62` ×3, `lambda_tb.cpp:21-22` ×2.
- Golden `check --atol 0 --strict`: 81/81 pass, 7 with "reordered" notes (the heap-layout notes listed in
  FIXES-REPORT.md decision 3).

## Rules for every batch
- Worktree `/tmp/ws-cleanup-wt/C-NN` on branch `cleanup-wip/C-NN` from the current `ws/cleanup`; own build dir
  `meson setup <wt>/build -Dcmake_prefix_path=$HOME/.local/opt/specs-deps -Dold_tb=true`; `meson compile -j 2`
  (two batches share 4 cores).
- Only the planned change, content-anchored (line numbers in the backlog files drifted), local style, no
  reformatting of untouched lines. New `.cpp` removals also come out of the explicit `meson.build` lists.
- Evidence: unique warning list before/after (clean build), `meson test -C <wt>/build` (all suites),
  `python3 -I tests/golden/run_golden.py check --specs … --unit-tests … --baseline tests/golden/baseline --atol 0
  --strict`. **No re-recording** except in C-08. A heap-layout "line order differs (reordered)" note is accepted
  (it is a PASS; FIXES-REPORT decision 3) but must be listed; any FAIL means the change is reverted or trimmed.

## Batches
Waves (≤ 2 subagents at a time): W1 = C-01 + C-02; W2 = C-03 + C-04; W3 = C-05 + C-07 (C-07 after C-03);
W4 = C-06 + C-08 (C-08 after C-01, C-04, C-07, so its files are final).

### C-01 — engine and CLI dead code (B-026 without the print, B-027, B-028 utils part, B-073 specs.cpp part, X-1, X-2)
Files: `src/main.cpp`, `src/optical_output_port.{h,cpp}`, `src/specs.{h,cpp}`, `src/optical_signal.{h,cpp}`,
`src/devices/time_monitor.cpp`, `src/utils/sysc_utils.{h,cpp}`, `src/utils/general_utils.cpp`, `src/meson.build`
(and any other build list naming `general_utils.cpp`).
- B-026: delete `do_circuit`, `raphael_main`, the `tests_module_registry_access` declaration, includes `<ctime>`,
  `<iomanip>`, `<deque>` (only if nothing else in main.cpp needs them: compile proves it), the forward declaration
  `class OpticalOutputPort;` and `using namespace literals;` (if nothing else needs them). **Keep** the
  `cout << reltol_val` print (C-08) and the ParserState handling (B-170 already fixed it).
- B-027: the corrected list in its backlog file. `crow.cpp:96` is already gone (B-072). Keep `m_event_queue`,
  the constructor's closing brace, `m_mode = m_config->m_mode;`, `m_skip_next_convergence_check`,
  `nodeset_orders` and its apply loop, and the cross-λ operator branches. In-class `engine_timescale` initialiser →
  `HUNDRED_FS` only after checking the constructor assigns it unconditionally. Add the missing `break;` after
  `immediateWriteFrequencyDomain(value);` in `delayedWrite`. Drop the `#if 1/#else/#endif` wrapper in
  `immediateWriteFrequencyDomain` (keep the `#if 1` body) and the `#if 1/#elif 0/#else` in specs.cpp (keep
  `oop->reset();`).
- X-1: `optical_signal.h` `operator==`: delete the dead `#if 0` arm and the `#else/#endif` lines; keep the body.
- X-2: `time_monitor.cpp`: delete the `#if 0 … #endif` block before the wall-clock report (it calls
  `current_err_fd`, which B-027 deletes), together with B-027's `if (false)` block.
- B-028 (utils part): delete `sc_object_is_module` (h + cpp) and the empty TU `general_utils.cpp` (and its
  `meson.build` entries).
- B-073 (specs.cpp part): comment "Enable photodetectors" → "Disable photodetectors"; delete the commented MLPROBE
  block. **Do not** touch the detector/power-meter name prints in `applyDefaultTraceFileToAllSignals` (C-08).
- Neutral because: every removed symbol has no caller (grep), the preprocessor arms are dead (`#if 0`/`#elif 0`),
  `drop_all_events` is only ever set inside `if (false)`, the fall-through only reached `break`, and the in-class
  initialiser is overwritten by the constructor. Expected: `ic_order` warning gone; possibly reorder notes in
  `cir/subckt_tests`/`opt/vci` (predicted by B-027).

### C-02 — passive devices (B-041, B-043, B-045 device part, B-067, B-111 header part, B-073 cw_source.h)
Files: `src/devices/{directional_coupler.cpp,merger.cpp,splitter.{h,cpp},phaseshifter.cpp,mzi.h,mzi_active.{h,cpp},
crossing.h,waveguide.cpp,generic_2x2_coupler.h,generic_waveguide.h,cw_source.{h,cpp}}`.
- B-041: delete the 8 `// initializing for nan wavelength` lines in DC and Merger (not PhaseShifter: B-042).
- B-043: splitter prints the actual `m_split_ratio`; PS `" rad @1.55)"` → `" rad"` (both sites).
- B-045, device files only: comment fixes (mzi.h:14, mzi_active.h ×3 "Constructor for Waveguide", ×3
  "y-junctions" → directional couplers, mzi_active.cpp "cast as unidirectional" in the Bi variant, crossing.h ×3
  in1 → in2), delete `WG3` from mzi.h, delete the orphan doc comment + `// static double m_delay_ns;` in
  splitter.h, the commented `cout`s in waveguide.cpp, generic_2x2_coupler.h and generic_waveguide.h, and the
  unused `lambda0` in generic_2x2_coupler.h (= B-111 header part). The `mzi.cpp` `__modname` deletion moves to C-07
  (B-178 owns that file); the parse_element.cpp comments move to C-04.
- B-067: drop `first_run` and simplify the condition to `if (!reset.read().to_bool())`.
- B-073: drop the unused `<fstream>` include from `cw_source.h` (only if it compiles: some TU may rely on it
  transitively; then add the include where it is used, or keep it).
- Neutral because: the map lines re-store the default of `operator[]` on maps that are never iterated; the two
  messages are only printed with `--vci`, which no golden case combines with SPLITTER/PSHIFT (verified by B-043's
  checker); `!first_run` is constant `false`; the rest is comments and unused declarations. Expected: `lambda0`
  warning gone.

### C-03 — probes, detector, power meter, value-list sources, Ring (B-068, B-073 device part)
Files: `src/devices/{ring.h,ring.cpp,alldevices.h,probe.{h,cpp},detector.{h,cpp},power_meter.{h,cpp},
value_list_source.cpp,electrical_value_list_source.cpp}`, `src/devices/meson.build` (and any list naming
`ring.cpp`).
- B-068 first: delete `ring.{h,cpp}`, its include in alldevices.h, the meson entry, and `PowerProbe`/`PhaseProbe`.
- B-073 with its corrections: delete `MLambdaProbe::m_trace_sig_*`, the undefined `prepare()` declaration,
  commented-out code in probe.cpp (keep the live `m_lambda_signals.insert(...)` line and the `isnan` guard),
  `wavelength_dependent_responsivity()` and the commented line in detector.cpp, the unused includes, the
  `"spx_module.h"` → `"devices/spx_module.h"` include, and the `if (false)` blocks in the VL/EVL runners. **Not**
  the `__modname` refactor (C-07) and not the `created <name>` print (C-08).
- Neutral because: nothing references or can construct the deleted classes; the removed `sc_signal` members of
  MLambdaProbe are never traced/read (auto-named, so they shift no user-visible name: the checker found no output
  change); the rest is compile-time only.

### C-04 — parser (B-110, B-112, B-174, B-173 non-print part, B-045 parse_element.cpp comments)
Files: `src/parser/{parser.l,parser.y,parse_tree.{h,cpp},parse_element.{h,cpp},parse_directive.{h,cpp},
parse_analysis.cpp}`.
- B-110: `struct netlist_content_t { … };`.
- B-112: `[[maybe_unused]]` on `has_reader`; delete `int i = 0;`/`++i;` in parse_analysis.cpp.
- B-174 with its corrections (ICDirective::create range includes its closing brace; also the commented
  `T_NONE`/`T_BOOL` lexer block; X-element TODO: remove the empty `if`, keep the design comment). Skip main.cpp.
  Do not touch `T_DIRECTIVE_SAVE`/`.nodeset` handling beyond the NODESET `#if 0` body. Keep the
  `"subcircuits":"TODO"` JSON block (design note) — it is not in B-174's list.
- B-173, non-print part: delete the duplicate `TRACEALL` branch; reword the `kp`/`t` comments (not :304/:315);
  delete the copied "convert from cross-coupling field coef" comments on MZI and CWSRC. **Keep** all prints
  (PSHIFT/MZI DC warning, `created`, `-----`, bidir `\t<sig0>,<sig1>`): C-08. Keep `" points)"`.
- B-045 parse_element.cpp part: DCElement "for waveguide" → "for coupler".
- Neutral because: no callers (grep), unreachable grammar checks (the grammar rejects the input first; B-174's
  checker tested it), the removed token is never lexed (one LALR state fewer, no accepted-language change),
  comments only. Extra check: `specs -n --json` on every `circuits/*.cir` and `tests/golden/netlists/*.cir` before
  and after give identical stdout/stderr/rc and JSON. Expected: possibly reorder notes (B-174 checker saw 2).

### C-05 — legacy testbenches and gtest sources (B-111 tb part, B-141, B-142)
Files: `src/tb/*`, `src/test/*`.
- B-111: the 5 unused locals in freqsweep_tb.cpp and lambda_tb.cpp.
- B-141 with its corrections: keep `IN3`/`IN4`, keep both phase-shifter checks (fix only the message strings),
  do not rename `ptrough`, do not touch mesh_tb.
- B-142 as corrected: stub `sc_main` (do not delete: libsystemc needs it), drop the `#if 0` block, the unused
  `<sys/wait.h>` in waveguide.cpp (check: utils.h may still need its own), the debug `cout << wl`, check `b_out`
  in the second assertion, per-process trace path like models.cpp. The macros: switch `EXPECT/ASSERT_CPLX_EQ` to
  `DOUBLE_EQ` inside `do { } while (0)` with parenthesised arguments **only if every test still passes**;
  otherwise keep `FLOAT_EQ` and report it. Keep test `A.B` (the baseline has it).
- Neutral because: testbench edits remove unused variables, dead code and overwritten assignments; message strings
  are only printed on failure; `crow_tb`'s stream-flag reset only acts after an FD sweep. gtest result set and
  names unchanged. Expected: 5 tb warnings gone.

### C-06 — Python and scripts (B-090, B-094, B-097)
Files: `pyspecs/*.py`, `utils/openwave.sh`, `utils/draw_graph_from_json.py` (unchanged), any doc that names the
deleted copy.
- B-090: `git rm pyspecs/draw_graph_from_json.py` (check first that both copies are still byte-identical).
- B-094 with its corrections (keep the wildcard import and the user-visible summary print).
- B-097: shebang and `${#configs[@]}`.
- Neutral because: no importer of the deleted copy; unused imports and dead branches; the script fix only changes
  an out-of-range path. Check: `py_compile`, `pyflakes` before/after, the 27+ baseline VCDs parsed into identical
  DataFrames before/after; `bash -n utils/openwave.sh` and `backlog/repro/a6-openwave-bounds.sh`.

### C-07 — `__modname` helper and remaining device-base dead code (B-178, B-075, B-028 device part)
Files: `src/devices/{spx_module.h,subcircuit_instance.h,probe.cpp,octane_cell.cpp,octane_segment.cpp,
octane_matrix.cpp,clements.cpp,mesh_col.cpp,mzi.cpp,generic_transmission_device.cpp}`. Starts after C-03 is
merged (probe.cpp).
- B-178 exactly as specified (GTD keeps its own suffix order; mzi.cpp's unused macro is just deleted).
- B-075: `double delay = 0.0;`.
- B-028: delete `ModuleFlags`/`flags` from spx_module.h and the unused `signals`/`modules` members of
  `SubcircuitInstance` (keep the class and `describe()`).
- Neutral because: object names byte-identical (verify by dumping every `sc_object` name for `-t mesh`,
  `-t lambda` and a PROBE netlist before/after, as B-178's checker did; tb/mesh's VCD scope names also prove it),
  `delay` is always assigned before use, the deleted members are never read. Expected: `delay` warning gone.

### C-08 — debug prints (B-065, B-173 prints, B-026 reltol print) — the only re-record
Files: `src/devices/{crow.h,crow.cpp,mesh_col.cpp,generic_transmission_device.cpp}`, `src/specs.cpp`,
`src/main.cpp`, `src/parser/{parse_element.cpp,parse_tree.cpp}`, `tests/golden/baseline/**`,
`tests/golden/cases.json` (only to drop `unordered_streams` from tb/det and tb/detector).
- Delete: CROW index/name prints, MeshCol `odd-even`/`odd-odd`, GTD `i, j is active`, `created <name>` (PROBE),
  `-----` (PCMCELL), the copied PSHIFT/MZI "default coupling power for DC" warnings, the bidir-upgrade
  `\t<sig0>,<sig1>` print, the detector/power-meter name prints (keep the `trace()` calls), and the `reltol_val`
  echo in main.cpp. **Decision:** B-026's `reltol_val` echo is the same kind of leftover debug print as B-065/B-173,
  so it goes in this commit under the same exception and proof.
- Proof (script committed under `backlog/cleanup/`): run `check --keep` with the new binary; for every failing
  case, every stdout/stderr diff line against the baseline is a `-` line matching one of the predicted messages;
  exit codes and every output file are byte-identical. Then `record --only '<exact case regex>' --note "B-065,
  B-173, B-026: debug prints removed"` and a final strict check. Predicted cases: tb/ac_crow, tb/det, tb/detector,
  tb/mesh, cir/add_drop_pyspecs, cir-json/add_drop_pyspecs, opt/mode-sampled-time, opt/tracefile-o,
  opt/tol-resolution-overrides, regress/b131-nrings-crow, regress/b036-vci-crow (plus any other that the script
  proves to lose only these lines).
- `unordered_streams` for tb/det and tb/detector: drop it only if `check --strict-order --only '^tb/det'` passes
  with both GCC and clang; otherwise keep it.
