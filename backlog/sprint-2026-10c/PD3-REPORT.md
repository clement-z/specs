# PD3 report: F-15 part 3 (`SC_METHOD` conversion)

Branch `ws/methods`, from `sprint/2026-10c` (39f1bf9, after D2b). Prompt
`discover/orchestration/2026-10c/PD3-methods.md`; decisions: F-15 "Follow-up decisions", Q1, Q3, Q7 of
`backlog/DECISIONS-2026-10-21.md`. No `git push`, no merge.

**Outcome.** The golden suite is bit-identical (outputs and event counters, gcc, clang, asan-gcc).
`C-notrace N=30` runs 2.55x faster than the D1 tip (target 1.5x). One effect outside the golden suite
needs the maintainer's attention ("Decision for the maintainer" below): on a net that carries several
wavelengths, a plain `PROBE` can show another of the wavelengths written in a time step. All `MLPROBE`
per-wavelength values were identical in every circuit checked.

## Items

| # | Item | Outcome | Commit | Tests | Golden impact |
|---|---|---|---|---|---|
| 1+2 | `OpticalOutputPort::on_data_ready` and the stateless device reactions (waveguide, coupler, splitter, merger, crossing, phase shifter, uni and bi) → `SC_METHOD` | done | bd92622 | gtest `ProcessStyle.*` (2 new) | none: strict 228/228, counters 227/227 identical; gtest case re-recorded (list only) |
| 2b | golden case pinning per-wavelength values of a WDM CROW | done | e7afa5b, e98f79d | `regress/f15-wdm-crow-mlprobe` (new, recorded with the base build) | new case only |
| 3 | `AGENTS.md` process style; ONBOARDING, engine audit notes; CHANGELOG | done | c1ec317, 95a54c5 | — | — |
| 4 | bench, gate, report | done | this commit | see below | — |

### 1+2. `SC_METHOD` conversion (bd92622)
- **Output port.** `on_data_ready()` is an `SC_METHOD` (`dont_initialize()`: the thread only reached its first
  `wait()` at initialization). The former `fire_pending()` method is now a plain function, called at the end
  of `on_data_ready()`. Both methods were sensitive to `m_pending_event` only, and the kernel triggers an
  event's static methods in reverse order of registration, so it ran them back to back, `on_data_ready()`
  first. The merge therefore keeps the kernel order. Checked: identical golden outputs, counters and extra
  CROW VCDs with and without the merge.
- **Devices.** Each former thread is an `SC_METHOD` with the same name and sensitivity (Q3), so the processes
  are triggered as before. The constants the threads computed when they started (S-parameters, delays,
  `PolarCache`, the waveguide's per-wavelength caches, the phase shifters' `m_memory[0] = 0`) are computed in
  `start_of_simulation()` from the same expressions. No callback or device changes these parameters after
  elaboration: composite devices (MZI, CROW) set their sub-devices' parameters in `init()` /
  `connect_submodules()`. The three loops of the waveguide share `WaveguideBase::propagate()`.
- **`--vci`.** The parameter listings were printed by each device's first thread when it started. Init-time
  threads are interleaved with the sources' and probes' threads, while methods would print first. So the
  listing is printed by a one-shot thread `print_initialization()`, created only with `--vci`, at the
  position of the former first thread. `opt/vci` and `regress/b036-vci-crow` are unchanged.
- **Not converted** (prompt and decision): sources, detector, probes, PCM cell, power meter, testbench threads.
- **Why one commit.** Converting the port alone changed `tb/lambda` and `regress/f15-stats-file-print`. The
  port method then ran before the device threads of the same delta cycle, so zero-delay writes into mergers
  coalesced differently. With the devices converted as well, both are methods and keep their relative order.

### 2b. `regress/f15-wdm-crow-mlprobe` (e7afa5b)
- **Circuit:** `crow_gen.py 3 --nwl 4`, 0.4 ns, `MLPROBE` on `drop` at the 4 wavelengths, no plain probe.
- **Recording:** made with the base build (39f1bf9); the conversion reproduces it bit for bit with gcc,
  clang and asan-gcc.
- **Sensitivity:** it fails with the PD2 time-only queue order (WIP 935f362): 255 points differ at 1551.2 nm,
  4e-5 relative. So it pins exactly what must not depend on the order of evaluation within a time step.
- **Size:** 462 kB, mostly the VCD. A listed subset of wavelengths would be smaller in the VCD, but each
  unlisted wavelength prints one stdout line per event ("Found, not done").

## Decision for the maintainer: plain probes on multi-wavelength nets
**Finding.** Outside the golden suite, plain `PROBE`s (`PROBE{net}` auto-probes included) on multi-wavelength
nets change. Two 4-wavelength CROWs were run with an `MLPROBE` (4 wavelengths) and a `PROBE` on every net, base
vs. PD3, gcc:

| circuit | VCD signals | differing | of which `MLPROBE` | port counters |
|---|---|---|---|---|
| 3 rings, 20 ns | 328 | 24 (`PROBE` power/abs/phase/wavelength on 6 nets) | 0 | coalesced 746937 → 747033, emitted and processed −96 (of 1.6 M) |
| 10 rings, 20 ns | 712 | 73 (`PROBE` on internal nets) | 0 | coalesced +492 of 13.0 M, processed −492 of 19.0 M |

- **What changes.** A plain probe records the last write of the time step. At some time steps another
  wavelength comes last, so the trace shows that wavelength's power and phase. stdout is identical.
- **Cause** (traced with event logs of both builds; first divergence at t = 2 ps, delta 48):
  - SystemC triggers the delta notifications in **reverse** order of notification. The value-change events of
    the nets are notified in the update phase, after the ports' re-arm notifications, so they trigger first.
  - Before PD3, the re-arm methods (`fire_pending`) ran in the method pass, ahead of every thread, so before
    the device reactions.
  - Now the device reactions are methods too, so they run before the re-arm in the same delta, and the order
    of `notify()` calls changes.
  - The kernel's timed queue (`sc_ppq`, ordered by time only) breaks ties by insertion history. So later,
    ports with the same time fire in another order.
  - A port edits a coalesced entry in place without re-sifting (PD2 finding). The order of the writes into a
    port can therefore change which of its same-time wavelengths it emits last.
  - The sources (still threads, as decided) also moved relative to the methods: methods now run before
    threads in each delta cycle.
- **Why it cannot be avoided.** No method conversion avoids this: `fire_pending` would have to run before
  every device method of its delta cycle, and SystemC offers no process priorities. Keeping the old order
  would mean keeping threads.
- **Per-wavelength values.** All `MLPROBE` signals were identical, as was the new golden case. In principle a
  different equal-time order can still change per-wavelength values at tolerance level (the PD2 Q8 mechanism:
  a coupler that receives a wavelength in two delta cycles of one time step emits an intermediate value). It
  did not happen in any circuit checked.
- **Why I landed it.** The decision anticipates that "evaluation order inside a delta cycle may change"
  (counters explained, Q1). The defined gate passes: strict golden bit-identical, and counters identical in
  all 227 pre-existing cases. This is the same kind of effect that the original Q8 decision listed as
  acceptable (which wavelength a plain probe shows on a multi-wavelength net).
- **Options:**
  - (a) accept (CHANGELOG states it);
  - (b) document that a plain probe on a multi-wavelength net shows one of the wavelengths written in a time
    step, in an unspecified order (I did not change `doc/netlist.md`: it is a user-facing semantic);
  - (c) do not merge D3: the D2 speed-up stays, 1.5x.

## Event counters (Q1)
- **227 pre-existing cases:** `stats-record` base vs PD3 identical, before and after the merge of `fire_pending`.
- **New case:** only the queue-length statistics change. `VLSource` `pending_mean` 1.75 → 1 (`pending_max`
  2 → 1): the source thread now runs after its port's method in the same delta cycle, so the port has already
  taken the previous wavelength when the source writes the next one. `COUPLER1` ports' `pending_mean` moves in
  the 4th digit, from the equal-time order. Scheduled, coalesced, processed and emitted counts are unchanged.
- **Extra CROWs:** see the table above (< 0.01 %).

## Performance (CROW bench, `discover/scout/bench`)
Pixi gcc 15 portable release builds, interleaved runs, median user time, same host and session (shared host,
load 1.1 to 2.5). Builds: v0.2.0 (`-Dnative=false`), D1 tip 56e5808 (the reference of Q7), base 39f1bf9
(after D2/D2b), final (this branch).

`C-notrace N=30` (`crow_gen.py 30 --traceall 0 --probes 0`), 5 rounds:

| build | median user s | rounds | vs D1 tip |
|---|---|---|---|
| D1 tip (56e5808) | 4.785 | 4.675, 4.804, 4.786, 4.691, 4.785 | 1.000 |
| v0.2.0 | 4.654 | 4.639, 4.633, 4.661, 4.688, 4.654 | 1.028 |
| base (39f1bf9, D2 + D2b) | 3.002 | 3.030, 3.002, 3.011, 2.968, 2.982 | 1.594 |
| **final (PD3)** | **1.880** | 1.934, 1.871, 1.920, 1.873, 1.880 | **2.545** |

[Q7] target (>= 1.5x on `C-notrace N=30` against the tip before D2) met: 2.55x, of which D3 alone gives
1.60x (base → final). Against v0.2.0: 2.48x (CHANGELOG: "about 2.5x").

Other cases, 3 rounds:

| case | D1 tip | base | final | final vs D1 tip | final vs base |
|---|---|---|---|---|---|
| `A-scaling N=30 probes=2` (`30 --traceall 0 --probes 1`) | 5.144 | 3.407 | 2.239 | 2.297 | 1.522 |
| `D-wdm N=10 nwl=4` (`10 --traceall 0 --probes 1 --nwl 4`) | 17.524 | 11.805 | 9.069 | 1.932 | 1.302 |

An earlier 3-round run of `C-notrace N=30` (before the docs commits, s3 binary): 2.510x vs the D1 tip. All
runs simulated 20 ns and exited 0. Script: `/tmp/pd3/scripts/bench.py` (same measurement as
`bench_crow.py`: `RUSAGE_CHILDREN` user time).

## Decisions I had to take (conservative)
1. **Items 1 and 2 in one commit:** the port alone changes golden outputs.
2. **`fire_pending()` merged into `on_data_ready()`:** it keeps the kernel order and saves one method
   activation per entry.
3. **Device constants in `start_of_simulation()`**, from the same expressions as the thread prologues.
   `WaveguideBi`'s two directions share one per-wavelength cache: same expression, same values. Each
   phase-shifter input keeps its own `PolarCache`, as before.
4. **`--vci` listings by a one-shot thread created only with `--vci`**, read from
   `specsGlobalConfig.verbose_component_initialization` in the constructor. `main()` sets it before any
   circuit or testbench is built. Without `--vci`, the converted devices create no thread.
5. **Landed despite the plain-probe change outside the golden suite** (decision section above), with the
   CHANGELOG saying so.
6. **The new golden case has no plain probe**, so that it pins order-independent values only.
7. **CHANGELOG:** the "Faster engine" entry is reworded (it said "same results bit for bit"), and its
   speed-up is measured against a v0.2.0 build (portable, `-Dnative=false`, pixi gcc 15).

## Found, not done
- `MLambdaProbe` prints "received unsupported signal" on stdout for every event at an unlisted wavelength
  (`src/devices/probe.cpp:168`): a per-event print in a hot path. `VLSource` prints every emission.
- Plain-probe semantics on multi-wavelength nets (option (b) above). Related: B-184.
- The other threads (probes, power meter, PCM, detector input reaction) could be methods too. Probes use
  `wait(enable.posedge_event())`. Traced runs (`B-traceall`) have one probe thread per net.
- `SC_HAS_PROCESS` deprecation warnings (pre-existing, 36 uses).

## Final results (tip 95a54c5 + this report)
- `pixi run test gcc`: 12/12 OK.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: 229 passed, 0 failed/new/missing (227
  pre-existing `specs` cases with no output re-record, 1 new case, gtest re-recorded for its list only).
- Event counters, gcc (`stats-record` / `stats-compare`, base 39f1bf9 vs final): 227 same, 1 new (the new
  case; its base-vs-final difference is explained above).
- clang release and asan-gcc, strict recordings of base vs PD3 (`compare --rtol 0 --atol 0`): 227/227
  identical. The new case passes strict on base and PD3 for both, against the gcc baseline.
- `pixi run test asan-clang`: 12/12 OK, no sanitizer message in the test log. `tests/sanitizers/lsan.supp`
  unchanged.
- Extra circuits (not in the repository): 3- and 10-ring 4-wavelength CROWs, see "Decision for the
  maintainer".

## Housekeeping
- Heartbeat `acf69ae1` deleted after this commit.
- Scratch in `/tmp/pd3` (binaries per step, recordings, extra circuits, scripts `gate.sh`, `after.sh`,
  `extra.sh`, `vcdcmp.py`, `tracecmp.py`, `bench.py`, debug patches).
- Worktrees `/tmp/pd3-wt/{base,d1tip,s2dbg,q8,v020}` (detached, local only): not needed for the merge; the
  coordinator can remove them (`git worktree remove`).
