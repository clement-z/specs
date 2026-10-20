# B-027: Dead engine code in OOP, SPECSConfig and OpticalSignal
severity: low | category: style/maintainability | effort: S | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 39500e1 (ws-cleanup C-01), except the `drop_all_events` member: now unused but kept, removing it shifts printed signal IDs in regress/b031-mzi-bidir (B-016)
location: src/optical_output_port.h:59-68,78-80,130-170, src/optical_output_port.cpp:26-58,82,108-109,190-299,402-404,416-420, src/specs.h:92,96,116,123-124,136, src/specs.cpp:24,162-174,202-206, src/optical_signal.cpp:66-109, src/devices/time_monitor.cpp:126-149
## Problem
Things that are never used, never set, or always exit:
- OOP: `on_data_ready_fd` (exits; body `#if 0`); `current_err_fd` (exits; only caller inside `#if 0`);
  `swap_wavelengths`/`delete_wavelength` (only called in the `#elif 0` arm of specs.cpp:165-171); `m_converger`
  (written at crow.cpp:96, never read); `m_skip_convergence_check` (never set). Many commented-out members and
  threads.
- The `FREQUENCY_DOMAIN` case of `delayedWrite` falls through to `default` (harmless; S5.12).
- SPECSConfig: `ic_orders` (never filled; its loop body in runTRANAnalysis is commented out, which gives an
  unused-variable warning; `ICDirective::create` exits); `drop_all_events` (only set inside `if (false)` in
  TimeMonitor, read only at optical_output_port.cpp:108); `traceSettings()` (declared, never defined); empty
  `before_end_of_elaboration`; the in-class `engine_timescale = ONE_FS` contradicts the constructor's `HUNDRED_FS`
  (S5.15).
- `OpticalSignal::sumSignals` is unused (it prints "Use of deprecated").

Scope correction (second check): the reviewer also claimed that the cross-wavelength branches of
`OpticalSignal::operator+=`/`-=` are unreachable. **That is false**: CROSSING (`crossing.cpp:46-47`) and the
GenericTransmissionDevice reach them, as `backlog/repro/a2-crossing-multi-lambda.cir` shows in `.op` and `.tran`.
They are **not** part of this cleanup; their behaviour belongs to B-033 (needs-human).
## Evidence
Grep results:
- `grep -rn "m_converger\|m_skip_convergence_check\|swap_wavelengths\|current_err_fd\|sumSignals\|traceSettings\|ic_orders\|drop_all_events" src`
- time_monitor.cpp:68-99 is `#if 0`; :128 is `if (false)`.

Additional evidence (independent report F-A7-05, A7 reviewer):

```
202  // Set values of signals according to IC directive
203  for (auto &ic_order : ic_orders)
204  {
205      //ic_order.second.first->write(ic_order.second.second);
206  }
```
## Expected
none of the above.
## Suggested fix
Delete only the following (line numbers at b66dc53, corrected by the second check). Keep
`m_skip_next_convergence_check` (B-003 needs it), the `nodeset_orders` map and the NODESET apply loop at
specs.cpp:73-77, and the cross-λ branches of `operator+=`/`-=`.
- `optical_output_port.h`: 59-60 and 62-68 (commented-out members; **keep :61 `sc_event_queue m_event_queue;`**);
  :78 `m_converger` and :80 `m_skip_convergence_check`; 130-170 (`swap_wavelengths`, `delete_wavelength`, the
  commented `current_err`, `current_err_fd`) and the `on_data_ready_fd` declaration.
- `optical_output_port.cpp`: 26-28, 32-36 and 39-58 (commented-out threads; **keep :37, the constructor's closing
  brace**); :82 (commented `make_shared`; **:86 `m_mode = m_config->m_mode;` is live, keep it**); 108-109
  (`if (specsGlobalConfig.drop_all_events) continue;`); 190-299 (`on_data_ready_fd`); the `#else` arm at 402-404;
  add `break;` after :418.
- `specs.h`: :92 `ic_orders`; :96 set the in-class initialiser to `HUNDRED_FS` (constructor unchanged); :116
  `drop_all_events`; 123-124 empty `before_end_of_elaboration`; :136 `traceSettings()` declaration.
- `specs.cpp`: :24 `drop_all_events = false;`; the `#elif 0` / `#else` arms at 165-174 (keep `oop->reset();` and
  drop the `#if 1` wrapper); 202-206 (`ic_orders` loop).
- `optical_signal.cpp`: 66-109 `sumSignals` (and its declaration in `optical_signal.h`).
- `time_monitor.cpp`: 126-149 (the `if (false)` block that sets `drop_all_events`; 123-124 end a live `cout`
  statement and must stay).
- `crow.cpp:96` (`m_converger` store) is deleted by B-072, not here; delete it here only if B-072 has not landed
  (the member disappears).
## Golden impact
None in values. With the corrected subset the check passes 44/44, but `cir/subckt_tests` and `opt/vci`
report "line order differs (reordered)": removing the `drop_all_events` signal changes heap layout and therefore
the pointer-ordered OP printout (B-016). Default `check` passes; `--strict-order` would fail until B-016 is fixed.
Re-record those two cases if strict order is in use.
## Related
B-020, B-028.

- Overlap: the `crow.cpp:96` dead store is also removed by B-072 and the `specs.cpp:202-206` `ic_orders` loop was reported separately as B-027 (merged here). Whichever fix lands first, the other skips those lines.

- Overlaps: `optical_output_port.cpp:107-120` is also changed by B-020 (error paths after `sc_stop()`);
  `time_monitor.cpp` is also restructured by B-058 (`--mon`). Whichever lands first, the other adapts.

- B-027 (merged) proposed keeping the `ic_orders` member with a TODO; it is deleted here because B-174 removes `ICDirective` entirely.

- Discovery seeds: S5.12, S5.15 (see `backlog/README.md` for seed numbering).
- Merged duplicate reports: F-A7-05 (A7).
- Review ids: F-A1-27, F-A7-05.
## Verification
Grepped every symbol listed. All removals are of unreachable code or members that are never read.

**Independent second check (V3, of F-A1-27): ADJUST.** Method: grepped every symbol, applied the neutral items on the scratch copy, rebuilt specs with all testbenches, golden check --atol 0; ran backlog/repro/a2-crossing-multi-lambda.cir (.op and .tran) and a link-error probe for operator+/- callers

(1) SPLIT OUT the claim "cross-lambda branches of +=/-= are unreachable". It is false. operator+ is called by CrossingUni/CrossingBi::on_input_changed (crossing.cpp:46-47 and the Bi twin), and operator- by GTD (generic_transmission_device.cpp:121,185,213). The a2 crossing repro hits the FD replace branch in .op (out1 = 0 W @1560 nm) and the TD cerr branch in .tran ("Attempted to sum signals of different wavelengths" x2). Deleting those branches changes multi-lambda CROSSING results, so this belongs to B-033 / needs-human. (2) Fix the locations, which as written would break the build or behaviour. optical_output_port.cpp:86 is the live `m_mode = m_config->m_mode;`; the intended line is 82 (the commented make_shared). optical_output_port.h:59-68 contains the live `sc_event_queue m_event_queue;` at :61, so delete only 59-60 and 62-68. optical_output_port.cpp:26-58 contains the constructor's closing `}` at :37, so delete only 26-28, 32-36 and 39-58. time_monitor.cpp:123-150 should be 126-149, because 123-124 end a live cout statement. Add optical_output_port.cpp:108-109 (`if (specsGlobalConfig.drop_all_events) continue;`), which must go together with the drop_all_events signal. specs.cpp:73-77 is the NODESET apply loop and contradicts "keep nodeset_orders": keep it (as I did) or remove both together. (3) With the corrected neutral subset: check 44/44 PASS, but cir/subckt_tests and opt/vci report "line order differs (reordered)". This is heap-layout ordering (B-016), deterministic per binary. Golden impact should mention it instead of "None". (4) Overlaps: crow.cpp:96 (m_converger store) is also in B-072, and time_monitor.cpp is also restructured by B-058. Assign each to one finding.

**Orchestrator:** Problem, suggested fix and golden impact rewritten from the second check: two ranges would have deleted live code (`m_event_queue`, the constructor brace, `m_mode` assignment), the cross-λ operator claim was wrong and is removed from this issue, `drop_all_events` uses at oop.cpp:108-109 added.

**Independent second check (V2, of F-A7-05): CONFIRMED.** Method: replaced specs.cpp:202-206 by the TODO comment, rebuilt, compared warnings, golden check

Baseline `specs.cpp:203:16: unused variable 'ic_order'` disappears; the other specs.cpp warnings (strict-aliasing, lines 385-388 -> 381-384 after the line removal) are pre-existing and unchanged. `ic_orders` is never filled (git grep: only specs.h:92 and the loop) and `ICDirective::create()` (parse_directive.cpp:128-132) prints "IC directive is not implemented" and exits, so the loop was dead. Golden 44/44 at atol=rtol=0.
