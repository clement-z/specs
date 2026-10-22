# 01 — Architecture & Simulation Engine (discovery probe 1/5)

Scope: `src/main.cpp`, `src/specs.{h,cpp}`, `src/optical_signal.*`, `src/optical_output_port.*`, `src/utils/*`, and the device↔engine contract.
I read the code and checked the SystemC 3.0.1 kernel sources in `thirdparty/systemc/src/sysc`. Nothing was built or run.

## 1. Summary

- SPECS is a single-threaded SystemC 3.0.1 (C++20) program. `sc_main` (main.cpp:191) parses the CLI, runs the flex/bison netlist parser, elaborates modules and signals in `ParseTree::build_circuit`, and then hands control to one global `SPECSConfig specsGlobalConfig` (an `sc_module`, specs.cpp:19). That global holds all configuration, owns every elaborated object, and runs the analyses.
- An event is a scheduled write of one `OpticalSignal` (complex field plus wavelength id) to one output port. Each device output owns an `OpticalOutputPort` (OOP). The OOP is a child `sc_module` with an `SC_THREAD`, a private priority queue and an `sc_event_queue` (F-15: replaced by its own allocation-free equivalent, an `sc_event`, a heap of times and an `SC_METHOD`). It emits only when the change passes **both** abstol and reltol (optical_output_port.cpp:161-165).
- WDM is time-multiplexed: a net (`sc_signal<OpticalSignal>`) holds only the **last written (λ, field) pair**. Each wavelength is a separate write in a separate delta cycle, and stateful consumers keep their own per-λ maps.
- Analyses: `.op` and `.dc` run in "frequency domain" (every OOP is forced to `NO_DELAY` and the circuit iterates in delta cycles to a fixed point). `.tran` always runs an OP first, then time-domain event-driven simulation. Outputs are stdout, one VCD file and an optional flattened-netlist JSON.
- Main risks: error handling is `cerr`+`exit(1)` everywhere; several CLI options are silently broken (`-m sampled-time`, `--mon`); delays are quantized to the time resolution, sometimes to zero, with no warning; results/output order depend on pointer order; per-event overhead is high (linear queue scan, heap allocations, SC_THREAD-only processes).

## 2. Component map

```
 CLI (taywee/args)          netlist(s) -f a.cir -f b.cir  + footer ".options k=v" (from --abstol/--reltol/-R)
   main.cpp:sc_main ──────────► parser.l/parser.y (yyparse) ──► ParseTree ──flatten()──► build_circuit()
        │                                                          │  new Device(...) ; ParseNet::create → sc_signal(s)
        │ (-m sets global mode; .op/.dc/.tran overwrite it)        │  connect_uni/connect_bi bind ports
        ▼                                                          ▼  directives → write specsGlobalConfig fields
 specsGlobalConfig (SPECSConfig : sc_module, static global) ◄── register_object(shared_ptr<sc_object>)
   runAnalysis ─► applyEngineResolution ─► prepareSimulation (init(), VCD, OOP configs, auto-probes)
             └─► runOPAnalysis | runDCAnalysis | runTRANAnalysis  (multiple sc_start calls)
                                   │
   ┌───────────── SystemC kernel (evaluate → update → delta notify → timed) ─────────────┐
   │ Device SC_THREAD ──read p_in──► compute S-param ──► OOP.delayedWrite(sig, delay)    │
   │      ▲                                                  │ PQueue<(t,sig)> + sc_event_queue
   │      │ value_changed (next delta)                        ▼                          │
   │ sc_signal<OpticalSignal,SC_MANY_WRITERS> ◄── write ── OOP::on_data_ready (tol gate)  │
   └──────────────────────────────────────────────────────────────────────────────────────┘
   Sinks: Probe/MLambdaProbe/Detector/PowerMeter ──sc_trace──► <trace_filename>.vcd ; stdout tables
```

| File | Responsibility |
|---|---|
| src/main.cpp | `sc_main`: CLI definition (194-285); mode, tolerance and timescale overrides (288-356); testbench dispatch (`-p`/`-t`, via `tb_map` in tb/alltestbenches.cpp); `build_circuit` hand-off to the parser (143-185); JSON export; `runAnalysis`. Also contains dead code: `do_circuit`, `raphael_main`. |
| src/specs.h / specs.cpp | `spx::` type aliases for value, signal, interface and port types (specs.h:21-52). `SPECSConfig`: global options, wavelength registry `wavelengths_vector`, owned objects `additional_objects`, analysis drivers OP/DC/TRAN, trace setup, OP printout. |
| src/optical_signal.{h,cpp} | `OpticalSignal` value type: field, λ-id, debug id. Arithmetic with λ-mismatch rules. Global λ registry lookup (`getIDFromWavelength`). |
| src/optical_output_port.{h,cpp} | `OpticalOutputPort`: per-output scheduler. Modes are EVENT_DRIVEN, SAMPLED_TIME (unimplemented) and NO_DELAY/FREQUENCY_DOMAIN. Holds the time-snap logic, per-λ desired/emitted state, the abstol/reltol gate and delta accumulation (`m_use_deltas`). |
| src/utils/pqueue.h | `PQueue<T>`: a `std::priority_queue<T, vector, greater>` with its container exposed (begin/end) so entries can be scanned and edited in place. |
| src/utils/sysc_utils.{h,cpp} | Recursive traversal of the SystemC object tree, plus `sc_get_all_{module,object}_by_type<T>()` (`dynamic_cast`, returns `std::set<T*>`). |
| src/utils/general_utils.h | `is_close`, `range(min,max,step)` (used for `.dc` sweeps). |
| src/utils/strutils.h | trim, toupper, iequals helpers. |
| src/devices/spx_module.h | Device base class: `sc_module` + typedefs + `flags` + `virtual init()` (called before simulation) + `describe()`. |
| src/devices/* | Concrete devices; covered by probe 2. Examples read here: waveguide, cw_source, probe, detector, time_monitor, generic_transmission_device. |
| src/parser/* | Covered by probe 3. Engine hand-off points: parse_tree.cpp:688-818, parse_directive.cpp:18-46, parse_analysis.cpp:23-109. |

## 3. Simulation lifecycle (CLI → output)

0. **Static init.** `SPECSConfig specsGlobalConfig{"SPX"}` is constructed before `main` (specs.cpp:19-39). This is an `sc_module` built during static initialization. Defaults: timescale `HUNDRED_FS` (the in-class initializer at specs.h:96 says `ONE_FS` but the constructor overwrites it), mode `EVENT_DRIVEN`, analysis `TIME_DOMAIN`, trace file `"traces/delete_me"` (resolved relative to the working directory).
1. SystemC's `main` calls `sc_elab_and_sim` (thirdparty .../kernel/sc_main_main.cpp:66), which calls `sc_main` (main.cpp:191).
2. **CLI parsing** (main.cpp:194-285). Flags: `-f` (repeatable), `-R`, `-m`, `-T`, `-o`, `--json`, `-n`, `--reltol`, `--abstol`, `--nrings_crow`, `--vci`, `-p`, `--mon`, `-t`, `-h`.
3. **Overrides.**
   - `-m` writes `specsGlobalConfig.simulation_mode` directly (290-312).
   - `--reltol`, `--abstol` and `-R` are validated, then serialized into a footer `.options k=v` that is parsed after the netlists (314-356, 383-388).
   - `--vci` sets a global verbose flag.
   - `-o` sets `trace_filename` (368-372).
   - `--mon` creates a `TimeMonitor` (374-380).
4. `-p` and `-t` run the legacy testbenches and return (365-367, 427-429).
5. **Parse** (main.cpp:143-185).
   - `yylex_init_extra(new ParserState)`. This object is never freed.
   - `yy_add_content_from_file` for each `-f` (parser.l:147), then the footer via `yy_add_content_from_string` (parser.l:131).
   - `yy_load_next_buf` (parser.l:99), then `yyparse(scanner,&pt)` (166), then `pt.print()`.
6. **Elaborate** (`ParseTree::build_circuit`, parse_tree.cpp:688-818).
   - `flatten()` expands subcircuits.
   - Elements on bidirectional nets are created first (702-751), then unidirectional ones (755-805).
   - Each element runs `create()` → `instantiate_and_connect_{uni,bi}` (parse_element.cpp:15+), which does a raw `new Device(name)` and binds ports.
   - Signals are created lazily by `ParseNet::create` (parse_tree.cpp:25-54). A bidirectional net becomes two unidirectional signals, `net:0` and `net:1`.
   - Directives then run in source order (806). `OPTIONSDirective::create` writes global tolerances and timescale (parse_directive.cpp:18-46).
   - **Only `analyses.at(0)`** is created (809). OP, DC and TRAN each set `analysis_type` and **overwrite `simulation_mode`** (parse_analysis.cpp:26,32,85).
   - Every module and signal is moved into `specsGlobalConfig.additional_objects` as `shared_ptr` (812-817).
7. `--json` writes `pt.to_json()` (main.cpp:397-405). `-n` skips simulation (407-420).
8. **`runAnalysis`** (specs.cpp:41-60).
   - `applyEngineResolution` calls `sc_set_time_resolution(10^(15+ts) fs)` (specs.h:131-134). This is only legal because no non-zero `sc_time` exists yet. I checked device headers and the parser: none is created during elaboration.
   - `prepareSimulation` (358-378):
     - calls `init()` on every `spx_module`, in pointer order;
     - `printConfig` and `verifyConfig` (asserts only);
     - opens the VCD;
     - gives every OOP without a config one shared default `OpticalOutputPortConfig` (243-258);
     - `applyDefaultTraceFileToAllSignals` hooks up Probes, MLambdaProbes, Detectors and PowerMeters, and creates an **auto `Probe` for every optical signal** whose base name does not start with `_` (280-298).
9. **OP** (specs.cpp:62-108).
   - First `sc_start()` call: elaboration callbacks run, `OOP::start_of_simulation→applyConfig` (optical_output_port.cpp:91), and every SC_THREAD runs to its first `wait()`.
   - NODESET (the directive currently exits in parse_directive.cpp:67).
   - Probes, ML-probes and detectors are disabled. Every OOP gets `m_mode=NO_DELAY` written directly. CW sources are enabled.
   - Second `sc_start()` runs to starvation: a delta-cycle fixed point at the same timestamp.
   - `printOPAnalysisResult` prints every optical net to stdout.
10. **DC** (110-188).
    - Same initial setup.
    - For each value of the **first** `cw_sweep_orders` entry: call the setter (CW λ, P or f) → `oop->reset()` on all OOPs → `sc_start(1 tick)` → pulse CW `reset`.
    - Sweep point *k* therefore sits at simulation time *k* ticks in the VCD.
11. **TRAN** (190-241). Runs the full OP (including the print). Then enables probes and detectors, calls `oop->applyConfig()` to restore the configured mode, enables VL/EVL sources, and calls `sc_start(tran_duration, SC_SEC)`, or `sc_start()` when the duration is infinite. `.ic` handling is commented out (203-206).
12. **Event inner loop (time domain).**
    1. A device SC_THREAD wakes on its input's `value_changed` (e.g. waveguide.cpp:53-73). It reads one `OpticalSignal`, multiplies by `polar(T, φ(λ))`, and calls `delayedWrite(s, sc_time(delay_ns, SC_NS))`.
    2. `delayedWriteEventDriven` (optical_output_port.cpp:326-358) computes `t = now + delay` and snaps it to the port timestep (301-324). It then either merges into an existing (t, λ) entry (linear `find_if`) or pushes into the `PQueue` and calls `m_event_queue.notify(t-now)`.
    3. At time t, `on_data_ready` (95-188) pops the top entry and checks `t==now`. It sets `desired` to the absolute field, or to the accumulated value when `m_use_deltas` is on. It gates on abstol **and** reltol against `m_emitted_fields[λ]`, zeroes values with |E|<10·abstol, and calls `m_port->write()` (185).
    4. During the update phase, `sc_signal` compares with `OpticalSignal::operator==` (field and λ-id exactly). If the value changed, `value_changed_event` fires in the next delta and downstream threads wake.
    5. One hop costs one device thread switch, one OOP thread switch, one signal update and ≥1 delta.
13. **Event inner loop (FD / NO_DELAY).** `immediateWriteFrequencyDomain` (371-405) pushes `(now, v)` and calls `notify(SC_ZERO_TIME)`, so every hop costs 2 deltas. Feedback loops keep circulating until the tolerance gate stops emission. SystemC has no delta-cycle limit, so a non-converging loop hangs.
14. **Outputs.**
    - stdout: banners, per-element "Creating …" lines, the parse-tree dump, `CWSource "emitted:"` lines (cw_source.cpp:33), the OP table, and `Simulated <t>` (specs.cpp:240).
    - VCD at `<trace_filename>.vcd`, with time unit = engine timescale. Delta cycles are not traced.
    - JSON (`--json`).
    - No CSV in the CLI path.
    - The VCD is **never closed** in the CLI path; only testbenches call `sc_close_vcd_trace_file`.
    - `sc_main` returns 0, or 1 when only help was printed.

## 4. Core data types

- **`OpticalSignal`** (optical_signal.h:17-191):
  - `complex<double> m_field`: the comment says V/m, but `power()=norm(field)` is printed in W and `CWSource::setPower` uses `sqrt(P)`, so the field is actually √W-normalized.
  - `uint32_t m_wavelength_id`: index into the global `specsGlobalConfig.wavelengths_vector` (metres).
  - `unsigned m_id`: debug id from a global counter, re-drawn on **every** copy or assignment (F-15: only in builds with `-Dsignal_ids=true`; no virtual destructor any more).
  - A virtual destructor, which adds a vptr (≈32 B per value instead of 24).
- **Wavelength registry** (optical_signal.cpp:111-147):
  - Lookup is a linear `find` with exact `double ==`; a new λ is appended.
  - A NaN λ maps to **id 0**, which is the first registered real λ. There is no "no-λ" sentinel, so every `isnan(getWavelength())` check is dead.
  - `getWavelength` calls `exit(1)` if the id is out of range, for example when the vector is empty.
- **Arithmetic** (optical_signal.cpp:8-64): `+=`/`-=` add fields when λ matches. If λ differs:
  - in TIME_DOMAIN: print to `cerr` and keep lhs (rhs power is dropped);
  - in FREQUENCY_DOMAIN (read from the **global** mode): keep whichever signal has the longer λ.
- **Equality and ordering.** `operator==` compares field and λ-id exactly; `sc_signal` uses it for change detection. `operator<` is **inverted** (`a<b` ⇔ |a|>|b|). Through `pair<sc_time,OpticalSignal>` in `PQueue` with `greater<>`, this makes equal-time entries pop in **decreasing |E|** order, not FIFO.
- **Time and units.**
  - Kernel resolution is `10^ts` s, with `ts∈[-15,0]` (default -13 = 100 fs).
  - OOP timestep = `m_timestep_value` ticks (default 1).
  - Delays: waveguide in ns (`sc_time(double,SC_NS)` rounds to the nearest tick); GTD `tau` in s.
  - Waveguide length in cm. Dispersion is referenced to a hard-coded 1.55 µm.
  - Detector beats wavelengths with `exp(i2π(c/λ)t)` (detector.cpp:66-71).
- **Multi-λ semantics.**
  - Linear stateless devices (e.g. waveguide) process each λ-event independently.
  - The OOP keeps `std::map<λid, field>` for desired and emitted values.
  - Summing or stateful consumers must keep per-λ memory. Detector does (`m_memory_in`). GenericTransmissionDevice does **not**: it calls `sc_stop` on a λ change (generic_transmission_device.cpp:201-208).
- **Electrical signals.** Analog is `double` (`ea_*`), digital is `sc_logic`/`sc_lv_base` (`ed_*`). Every signal uses `SC_MANY_WRITERS`, which skips the per-port check and only checks for same-delta conflicts.
- **Ports.** Inputs are `sc_port<IF,0,SC_ZERO_OR_MORE_BOUND>`: multiport, may be unbound, and `->read()` reads interface 0 only. Outputs are `sc_port<IF>`, exactly one binding.

## 5. Conventions observed

- **Naming.** Files are snake_case and classes PascalCase. Members use the `m_` prefix and ports use `p_in`/`p_out`/`p0_in`. Methods mix camelCase (`delayedWrite`, `applyConfig`) and snake_case (`on_data_ready`, `drop_queue`). Headers use `#pragma once`. `using namespace std;` appears in a header (optical_signal.h:15), so it leaks into every TU.
- **Device contract.**
  - Derive from `spx_module`.
  - Declare ports and one `OpticalOutputPort` member per output, built with a reference to its port.
  - `SC_THREAD(x); sensitive << p_in;` with the loop `wait(); read; compute; delayedWrite`.
  - Optional `enable`/`reset` `sc_signal<sc_logic>` members, toggled by `SPECSConfig` after it finds devices by concrete type with `dynamic_cast` (Probe, MLambdaProbe, Detector, CWSource, VLSource, EVLSource). Any new source or sink type has to be hard-coded into specs.cpp.
- **Error handling.** Outside tb/test: ~119 `exit(` call sites, 9 `sc_stop()`, 63 `assert`, 0 `SC_REPORT_*`, 2 `throw`, ~184 `cerr <<`. Asserts stay active only because CMake sets no build type, so there is no `NDEBUG`.
- **Logging.** Raw `cout`/`cerr` with box-drawing banners. No levels except `--vci`. Some per-event printing (CW emission, "unsupported signal" in MLambdaProbe, the λ-mismatch warning on every sum).
- **Ownership.**
  - Parser devices: raw `new`, wrapped in `shared_ptr` in `circuit_modules`, then moved into the global `additional_objects`. They are destroyed during static destruction, after `sc_main` returns. The simcontext is leaked (never deleted, kernel/sc_simcontext.cpp:1573), so that destruction is probably safe.
  - OOPs are by-value members. GTD uses vectors of `shared_ptr`. MLambdaProbe uses `unique_ptr` signals.
  - Leaks: `ParserState` (main.cpp:146) and the scanner string copies (parser.l:131-137).
- **Processes.** 64 `SC_THREAD`, 0 `SC_METHOD`, 2 `sc_spawn`. On x86_64 each thread gets a 256 KiB stack (kernel/sc_thread_process.cpp:74). There are 36 `SC_HAS_PROCESS` uses, which SystemC 3.0 marks `[[deprecated]]` (kernel/sc_module.h:413), so builds produce deprecation warnings.
- **Global state.**
  - `specsGlobalConfig`: config, λ registry, owned objects, and the `drop_all_events` signal.
  - `OpticalSignal::nextId`, `tb_map`, the scanner's `netlist_content_fifo` and `static FILE* last_file`.
  - No threads, no unordered containers in core code.
- **Determinism.**
  - `std::set<T*>` ordered by **pointer** (sysc_utils.h:38-66) drives the order of `init()` calls, auto-probe creation, VCD variable declarations and OP printout. These change from run to run under ASLR.
  - Detector RNG is seeded from `std::random_device` (detector.cpp:134-135), and there is no seed option.
  - `m_id` values printed to stdout depend on how many copies the compiler elides.
- **Build defaults.** CMake has no default `CMAKE_BUILD_TYPE`, so `install.sh` builds at `-O0` (plus `-march=native`). The legacy Makefile used `-O2` and C++17.

## 6. Unknowns / open questions

- Does `.options` in a netlist ever override the CLI footer? It depends on directive order after `flatten()`; I did not check how directives from multiple files and the footer are merged.
- Is it intended that probes are disabled during OP and that the OP steady state never reaches the VCD? See seed P-08. Repro candidate: circuits/add_drop_pyspecs.cir (.tran).
- How OP convergence is meant to work with lossless feedback. There is no iteration cap and no non-convergence diagnostic (the commented-out `drop_all_events` machinery in time_monitor.cpp:126-148 suggests an earlier attempt).
- SAMPLED_TIME mode: abandoned, or planned? (`delayedWriteSampledTime` is an empty TODO.)
- Destroying all `sc_module`s during static destruction after `sc_main` returns: not verified under ASan/valgrind.
- Whether any shipped device writes to an OOP during the initialization phase. If one does, the queue reset at the start of `on_data_ready` (optical_output_port.cpp:98) drops the entry, and the stale `sc_event_queue` notification later hits the empty-queue path.
- Real-world numerical impact of rounding delays to the 100 fs default resolution on ring and CROW results (the paper benchmarks 100 fs vs 1 fs).
- Does any device read a multiport `p_in` that is bound to more than one signal? `->read()` only reads interface 0.

## 7. Seeds for backlog

- [severity: high] src/devices/time_monitor.cpp:105-107 — `--mon` (main.cpp:258,375-380) prints "Time monitor incompatible with new multi-wavelength features" and calls `exit(1)` once 0.5 s of wall-clock time has passed. Any simulation longer than 0.5 s is killed. An advertised CLI flag is effectively a crash switch.
- [severity: high] src/optical_output_port.cpp:360-369,413-415 — `delayedWriteSampledTime` is an empty TODO, but `-m sampled-time` is advertised (main.cpp:204-207,297-301). In TRAN, every event in every port is silently dropped and the results are empty or all zero, with no error.
- [severity: high] src/devices/waveguide.cpp:72,98 + src/optical_output_port.cpp:329-331 — delays become `sc_time(double)`, which rounds to the nearest tick (default 100 fs). Delays under half a tick become **zero**, which can produce zero-delay feedback loops and delta storms in rings. The "Event cannot be correctly described" check compares `(now+delay)-now` with `delay` (two integer `sc_time` values), so it can never fire.
- [severity: high] src/test/utils.h:83 — `static_assert(false, "Work in progress…")` at namespace scope is always ill-formed. The `unit_tests` target is in ALL when `BUILD_TB=ON` (the default; `install.sh` passes `-DBUILD_TB=1`), so `make -j` in install.sh fails. It also needs network access for FetchContent (cmake/googletest.cmake).
- [severity: high] src/utils/pqueue.h:12 + src/optical_signal.h:120-123 + src/optical_output_port.cpp:380-400 — FD writes only compare against the queue **top**. With ≥2 λ on one port, the same λ can be queued twice at `now`. Equal-time entries pop in decreasing |E| order (inverted `operator<`), not insertion order, so a **stale smaller value can be emitted last** and stay as the converged OP/DC value. Needs a multi-λ repro.
- [severity: med] src/optical_output_port.cpp:107-120,131-139 — on an empty queue or a time desync, the thread calls `sc_stop()` but **keeps executing**. That means `m_queue.top()` on an empty vector (UB or segfault) and a write of the wrong-time value. Should `return`/`continue` or raise `SC_REPORT_ERROR`.
- [severity: med] src/optical_output_port.cpp:98 — `m_queue = queue_type()` runs when the OOP thread first starts. Anything enqueued earlier in the initialization phase (process order is unspecified) is lost while its `sc_event_queue` notify remains, which leads to the empty-queue path above.
- [severity: med] src/specs.cpp:161-163 + src/devices/generic_transmission_device.cpp:150-195 + src/optical_output_port.cpp:152-156 — the DC sweep resets each OOP's accumulated `m_desired_fields`, but devices that use `m_use_deltas` keep their `last_signal`. From sweep point 2 onward the GTD outputs only Δ instead of the full value, so the results are wrong. A λ sweep through a GTD hits `sc_stop` instead (line 207).
- [severity: med] src/devices/probe.cpp:27-31 + src/specs.cpp:80-82,209-211 — probes are disabled during OP. When re-enabled for TRAN they `continue` back to `wait()` without sampling the current value, so nets that stay constant after OP (e.g. a CW path) never appear in the VCD and read as 0. Verify with a `.tran` netlist.
- [severity: med] src/specs.cpp:280-298 + src/devices/probe.cpp:33-46 — auto-probes on multi-λ nets write power, abs, phase and λ for whichever λ arrived last. Since VCD does not trace delta cycles, the waveform for a WDM net shows one arbitrary λ per timestep, not total power.
- [severity: med] src/specs.cpp:366-369 — if `trace_filename` is empty (`-o ""`), `default_trace_file` stays null and `->set_time_unit` dereferences it. Separately, the default `traces/delete_me` is relative to the working directory: running outside the repo root fails when the VCD is opened, and inside the repo it rewrites a tracked file (traces/delete_me.vcd).
- [severity: med] src/specs.cpp:367 (no `sc_close_vcd_trace_file` in the CLI path) — the VCD is never closed. The destructor never runs, so the final timestamp is missing, and the file relies on stdio flush at `exit`.
- [severity: med] src/utils/sysc_utils.h:38-66 — `std::set<T*>` is ordered by pointer. That order drives `init()` order (and therefore GTD `sc_spawn` and process order), auto-probe creation, VCD variable order and the OP printout, so output files differ across identical runs. The fix is to sort by `name()`.
- [severity: med] src/devices/detector.cpp:134-135 — RNG seeded from `std::random_device` and re-seeded on every `init()` call (prepareSimulation plus the thread start). Noisy simulations cannot be reproduced and there is no `--seed`.
- [severity: med] src/optical_signal.h:31-40 + src/optical_signal.cpp:139-147 — "no wavelength" aliases λ-id 0, the first registered λ. NaN checks are dead (optical_signal.cpp:19-22, probe.cpp:35, GTD:178). Printing an unset signal with an empty registry calls `exit(1)`. src/devices/detector.cpp:12,68 creates `m_memory_in[0]` and then indexes `wavelengths_vector[0]` unchecked, which is out of bounds when no λ is registered.
- [severity: med] src/optical_signal.cpp:111-137 — the λ registry uses exact `double ==` with a linear search. `setFrequency` (c/f) and arithmetic from `range()` produce near-duplicate ids. λ sweeps grow the registry without bound, and every construction from a λ costs O(#λ).
- [severity: med] src/optical_signal.cpp:17-32,46-61 — summing different λ silently drops power in TD mode (with a `cerr` line per event, which can flood output), and keeps the longer λ in FD mode. The behaviour is keyed on the **global** `simulation_mode`, which says TIME_DOMAIN during the OP phase of a `.tran` even though the ports are in NO_DELAY.
- [severity: med] src/main.cpp:290-312 vs src/parser/parse_analysis.cpp:26,32,85 — any `.op`/`.dc`/`.tran` overwrites the `-m` choice, so `-m` only matters for netlists without an analysis directive. Only `analyses.at(0)` runs (parse_tree.cpp:809); extra analyses are silently ignored.
- [severity: med] src/optical_output_port.cpp:341-343 — every event-driven write does a linear `find_if` over the whole pending queue, which is O(n²) for long delay lines and bitstreams. The OOP also keeps a second heap inside `sc_event_queue`, which allocates `new sc_time` on every notify (kernel sc_event_queue.cpp:82). That is ≥2 heap allocations plus two O(log n) structures per event.
- [severity: med] CMakeLists.txt (no `CMAKE_BUILD_TYPE` default) + install.sh:27 — the default build is `-O0`. Performance claims (the paper benchmarks) presumably used the Makefile's `-O2`. Set a Release default.
- [severity: med] src/specs.h:28-31 — every signal uses `SC_MANY_WRITERS`, which disables SystemC's single-driver port check. Two outputs wired to one net are caught only if they write in the same delta; otherwise their writes interleave silently.
- [severity: low] src/optical_signal.h:52-56,147-153 — the copy constructor and `operator=` both call `getNewId()`, and `operator=` returns **by value** (an extra copy). Every `sc_signal` write and update and every queue move bumps a global counter. The virtual destructor is unneeded (no subclasses found), and the 32-bit `nextId` wraps on long runs.
- [severity: low] src/optical_output_port.cpp:67-73,169 — the reltol check divides by |last|; when it is 0 the code relies on inf/NaN comparisons. The hard-coded rule "snap to 0 if |E|<10·abstol" is marked `TODO: check this`. abstol AND reltol (instead of SPICE-style `abstol+reltol·|x|`) is undocumented.
- [severity: low] src/specs.cpp:149 — `*cw_sweep_orders.begin()` is UB when the map is empty (guarded only by an `assert` in parse_analysis.cpp:34). Only the first sweep (by map key order) runs; others are silently ignored.
- [severity: low] src/utils/general_utils.h:30-33 — `range()` accumulates float error with `val += step`, excludes `max`, and calls `exit(1)` when `max<=min`. Sweep endpoints drift and the last point is missing.
- [severity: low] src/specs.cpp:385-388 — `sc_trace` through `(int&)` casts of `uint64` and enums is a strict-aliasing violation; on big-endian it would trace the wrong half.
- [severity: low] src/specs.h:138-146 + src/parser/parse_directive.cpp:35-39 — `verifyConfig` is asserts only, a no-op under `NDEBUG`. `.options resolution=<double>` is cast to `uint64` (UB when negative). The timescale integer is cast to the enum unchecked. The second `TRACEALL` branch (lines 38-39) is dead because the first one (line 28) always matches.
- [severity: low] src/main.cpp:65-141,187,189,319,341-345,357-359 — dead or no-op code: `do_circuit`, `raphael_main`, the `tests_module_registry_access` declaration, a debug `cout` of reltol, a `--nrings_crow` no-op and a `-T` no-op. `exit(parse_result)` inside `sc_main` (395) bypasses `sc_elab_and_sim` cleanup. `new ParserState` leaks (146).
- [severity: low] src/devices/*.cpp (48 sites, e.g. waveguide.cpp:44-46) — `--vci` printing does `dynamic_cast<oa_signal_type*>(p.get_interface())->name()` with no null check, which segfaults on an unbound or non-optical port.
- [severity: low] src/devices/cw_source.cpp:18,36 — `first_run` is never cleared, so the condition is dead. src/optical_output_port.cpp:416-420 has a missing `break` (harmless fallthrough). `m_converger` and `m_skip_convergence_check` are never set.
- [severity: low] src/optical_signal.h:15, src/utils/*.h — `using namespace std` / `using std::…` in headers. src/optical_signal.h:26,234 labels the field "V/m" even though it is √W.
- [severity: low] 36× `SC_HAS_PROCESS` — `[[deprecated]]` in SystemC 3.0 (kernel/sc_module.h:413), producing warning noise. 64 SC_THREADs × 256 KiB stacks costs memory and context switches on large meshes and CROWs. SC_METHOD would suit stateless devices.
