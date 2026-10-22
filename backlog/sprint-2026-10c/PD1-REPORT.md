# PD1 report: F-15 part 1 (`--stats`, ablation switches, `--progress`, counter tool)

Branch `ws/stats`, from `sprint/2026-10c` (4f177fb). Decisions applied: F-15 "Follow-up decisions" and Q1, Q4,
Q5, Q6 of `backlog/DECISIONS-2026-10-21.md`. No `git push`, no merge.

## Items

| # | Item | Outcome | Commit | Tests | Golden impact |
|---|---|---|---|---|---|
| 1 | Per-port counters, `--stats[=summary\|ports\|devices]`, `--stats-file`, `--print-stats` | done: `OpticalOutputPortStats` per port (no globals), collected only when the port configuration says so; `RunStats` (`src/run_stats.*`) sums them per run, per port, per device type; JSON `<trace>.stats.json` | ec3e0f3 | gtest `RunStats.*` (classification of every entry, off by default, JSON layout, sections, numbers); harness `RunStats.*` (wall-time masking) | 212 existing cases bit-identical; re-recorded `cli/help`, `cli/no-args` (help lines, additions only), gtest; new `regress/f15-stats-{tran,op-no-trace,file-print,dc}`, `err/f15-stats-bad-section` |
| 2 | Expert ablation switches [Q4]: no coalescing (FIFO ties), event cap; `.options coalesce`/`maxevents`, `--no-coalescing`/`--max-events`; `doc/numerics.md` | done (engine side in ec3e0f3, user-facing in 51db5ea) | ec3e0f3, 51db5ea | gtest `RunStats.NoCoalescingKeepsEveryWriteInOrder`, `RunStats.EventCapStopsTheSimulation`; 9 golden cases | none for existing cases; new `regress/f15-coalesce-options`, `regress/f15-no-coalescing-cli`, `regress/f15-maxevents-options-tran`, `regress/f15-max-events-cli-priority`, `err/f15-max-events-op`, `err/f15-max-events-invalid`, `err/f15-options-maxevents-fraction`, `regress/f15-progress-quiet`, `err/f15-progress-invalid` |
| 3 | `--progress[=seconds]` [Q6] | done: `RunProgress` (`src/run_progress.*`), a SystemC stage callback (`SC_POST_UPDATE`) that prints at most one line per wall-clock period on stderr; reads only `sc_time_stamp()`/`sc_delta_count()` | ec3e0f3 (code, gtest), 51db5ea (golden) | gtest `RunProgress.Off`, `RunProgress.PrintsWithoutChangingTheSimulation` (same times and values with and without, never stops) | none |
| 4 | Counter collection for the D2/D3 gates | done: `run_golden.py stats-record` (every case with `--stats=ports,devices --stats-file golden-case.stats.json` appended; exit status + stats without wall time per case) and `stats-compare` (exact, rows keyed by port/device type, `--ignore`, `--report`); `--stats` also for `-t` testbenches (the 20 `tb/*` cases are covered) | a53004c | harness `Stats.*` (fake specs); gtest `RunStats.PortOfADestroyedSignalHasNoNet`; golden `regress/f15-stats-testbench` | re-recorded `cli/help`, `cli/no-args` (one help line), gtest |
| 5 | Cost of stats collection (CROW bench) | done, see below | — | — | — |

Counter tool checks (2026-10-22): two `stats-record` runs of the gcc build: 225/225 cases identical (133 with
statistics; the others are parse errors, `-n`, help, JSON export). asan-clang build vs gcc build: 225/225
identical, same exit statuses (the 6 cases that differ in the last bits between these builds keep the same
counters). For D2/D3: `tests/golden/README.md`, "Event counters".

## Performance (stats off must not slow down)

`discover/scout/bench` case `C-notrace N=30` (`crow_gen.py 30 --traceall 0 --probes 0`), pixi gcc release
builds, base = 4f177fb (`/tmp/PD1-wt/base`), new = a53004c; variants interleaved, 5 rounds, median user time,
same host and session (shared, load 1.5 to 4.5):

| variant | median user s | rounds | ratio to base |
|---|---|---|---|
| base | 4.698 | 4.726, 4.650, 4.649, 4.698, 4.731 | 1.000 |
| new, stats off | 4.666 | 4.666, 4.662, 4.607, 4.714, 4.731 | 0.993 |
| new, `--stats=ports,devices` | 4.922 | 4.922, 4.876, 4.893, 4.937, 4.968 | 1.048 |

A first run (5 rounds) gave 0.995 for stats off. Stats off: no slowdown (within noise). Collecting: about +5 %.
All 15 runs simulated 20 ns, exit 0.

## Final results

- `pixi run test gcc`: 12/12 OK.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: 227 passed (226 `specs` cases + gtest), 0 failed /
  new / missing. The 212 cases of the start (211 + gtest) are bit-identical except the predicted re-records
  (`cli/help`, `cli/no-args`, gtest).
- `pixi run test asan-clang`: 12/12 OK, no sanitizer message (run after ec3e0f3 and after a53004c's code).
  `tests/sanitizers/lsan.supp` unchanged.
- Before/after, strict (`compare --rtol 0 --atol 0 --print-slack 0`, the 209 pre-existing cases without the two
  help cases): clang release 209/209 identical, asan-gcc 209/209 identical.

## Decisions taken (not covered by the decisions file; conservative choices)

1. **Counter definitions.** `scheduled` = `delayedWrite()` calls (event-driven and FD); `coalesced` = of which
   merged into the pending entry of the same (time, wavelength); `processed` = entries taken at their time;
   every processed entry is exactly one of `emitted` (written, also forced writes), `suppressed.abstol`,
   `suppressed.reltol` (abstol test passed first), `suppressed.snap`. **"Suppressed by snap"** = the gate passed
   but the value was snapped to 0 while 0 was already the last value emitted at that wavelength: the port still
   writes it (behaviour unchanged), it changes nothing. Added `pending_end` so that `scheduled = coalesced +
   processed + pending_end` holds.
2. **Queue length** sampled after each scheduled value: `pending_max`, `pending_mean = sum / scheduled`.
3. **Last emission time** = time of the last `emitted` value (snap repeats excluded); run value = max.
4. **Tiers** [Q5]: `--stats` = summary; `ports` and `devices` each add their section; a comma list
   (`--stats=ports,devices`) gives both (the decision did not say whether `devices` includes the port rows).
   One row per output port lists all its wavelengths (a `.dc` wavelength sweep lists every swept wavelength).
   Device type = demangled C++ class of the module that owns the port.
5. **Where the JSON goes**: `<trace>.stats.json`; `--stats-file path` (explicit path, implies `--stats`);
   `--print-stats` (also on stdout, implies `--stats`); with no trace file and no `--stats-file`, printed on
   stdout. A file write prints `Run statistics written to <file>` on stdout. Also written on a non-convergence
   error and at the event cap. `--stats` and `--progress` are CLI only (Q4 is about the ablation switches).
6. **Wall time** from the start of `runAnalysis()` (or of the testbench), on its own JSON line, masked by the
   golden harness in `*.stats.json` files and on stdout.
7. **No coalescing**: a separate `std::multimap<sc_time, OpticalSignal>` queue (FIFO among equal times) used
   instead of the heap; the default path keeps the heap code (one extra branch). D2 may unify both when it
   replaces the queue. With it, a plain net/probe of a multi-wavelength net can show another wavelength at the
   end of a time step (`regress/f15-coalesce-options`: OP listing of `v` at 1550 nm instead of 1560 nm; values
   per wavelength unchanged), the Q8 effect.
8. **Event cap** counts processed entries over the run (shared `OpticalOutputPortEventCap` in the port
   configuration, no global); `sc_stop()` at the end of the delta cycle that reaches it (a few more entries of
   that delta cycle are processed). `.tran`: ends there, warning, exit 0. `.op`/`.dc`/OP of a `.tran`: error,
   exit 1 (a cut fixed point is not a result). CLI values win over `.options` (as `--maxiter`).
9. **Progress**: default period 1 s, `--progress=<s>`, netlists only (testbenches do not go through
   `runAnalysis()`); the percentage only for a `.tran` with a duration.
10. **`--stats` with `-t`** (beyond the prompt, to cover the `tb/*` cases in the D2/D3 gate): written next to
    the testbench's trace file when it returns. This exposed a crash (below), fixed in the same commit.

## Found and fixed within scope

- `stats-record` found a SIGSEGV (5 of 8 runs of `-t ac_crow --stats`): the net of a port was resolved by
  dereferencing the channel pointer, which dangles for signals of the testbench function destroyed when it
  returns. Nets are now looked up among the live channels by pointer comparison (`get_interface(0)`, no
  conversion to the virtual base `sc_interface`); such ports show `"net": null`. Regression test added.

## Found, not done

- `ac_crow` testbench: its CROW ports stay bound to signals destroyed when the testbench function returns
  (dangling `sc_port` interfaces after the run; harmless today because nothing reads them afterwards).
- Port names of sources repeat the hierarchy: `ROOT/VLSRC1.ROOT/VLSRC1_out_writer` (cosmetic, visible in
  `--stats=ports`).
- Existing build warnings: `SC_HAS_PROCESS` deprecated (clang, IEEE 1666-2023), strict-aliasing casts in
  `sc_trace(SPECSConfig)` (gcc).
- The expert switches also reach the `-t` testbenches that call `prepareSimulation()` (not tested there; the
  event cap would make a testbench's later `sc_start()` fail after `sc_stop()`).

## Housekeeping

Heartbeat `1c34605e` deleted after this commit. Scratch: `/tmp/PD1-wt/base` (worktree of 4f177fb, branch
`PD1-wip/base`) removed; recordings in `/tmp/pd1-*` are not needed.
