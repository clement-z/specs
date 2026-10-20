# B-027: Dead engine code in OOP, SPECSConfig and OpticalSignal
severity: low | category: style/maintainability | effort: S | fix-class: easy-unattended | verdict: CONFIRMED
location: src/optical_output_port.h:59-68,78-80,130-170, src/optical_output_port.cpp:26-58,86,190-299,402-404,416-420, src/specs.h:92-93,96,116,123-124,136, src/specs.cpp:24,73-77,162-174,202-206, src/optical_signal.cpp:66-109, src/devices/crow.cpp:96, src/devices/time_monitor.cpp:123-150
## Problem
Things that are never used, never set, or always exit:
- OOP: `on_data_ready_fd` (exits; body `#if 0`); `current_err_fd` (exits; only caller inside `#if 0`);
  `swap_wavelengths`/`delete_wavelength` (only in `#elif 0`); `m_converger` (written at crow.cpp:96, never read);
  `m_skip_convergence_check` (never set). Many commented-out members and threads.
- The `FREQUENCY_DOMAIN` case of `delayedWrite` falls through to `default` (harmless; S5.12).
- SPECSConfig: `ic_orders` (never filled; loop body commented out, which gives an unused-variable warning);
  `drop_all_events` (only set inside `if (false)` in TimeMonitor); `traceSettings()` (declared, never defined);
  empty `before_end_of_elaboration`; the in-class `engine_timescale = ONE_FS` contradicts the constructor's
  `HUNDRED_FS` (S5.15).
- `OpticalSignal::sumSignals` is unused. The cross-λ branches of `+=`/`-=` are unreachable: the only callers
  (OOP lines 354, 391) compare λ ids first.
Keep `m_skip_next_convergence_check` (B-003 needs it) and the `nodeset_orders` map (used by the parser).
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
Delete the items listed. Add `break;` after line 418. Set specs.h:96 to `HUNDRED_FS` (constructor unchanged).
Remove the `#elif 0` / `#else` arms in specs.cpp:165-174. Remove crow.cpp:96. Remove the TimeMonitor `if (false)`
block that references `drop_all_events`.
## Golden impact
None (compile-only; `drop_all_events` is not traced).
## Related
B-020, B-028.

- Overlap: the `crow.cpp:96` dead store is also removed by B-072 and the `specs.cpp:202-206` `ic_orders` loop was reported separately as B-027 (merged here). Whichever fix lands first, the other skips those lines.

- Discovery seeds: S5.12, S5.15 (see `backlog/README.md` for seed numbering).
- Merged duplicate reports: F-A7-05 (A7).
- Review ids: F-A1-27, F-A7-05.
## Verification
Grepped every symbol listed. All removals are of unreachable code or members that are never read.
