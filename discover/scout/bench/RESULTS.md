# Feasibility measurements (2026-10-20)

Binary: CMake Release build of b66dc53 (`-O3 -march=native -DNDEBUG`, gnu++20, SystemC 3.0.1) in
`/tmp/ws-scout-build`. Host: Intel i5-6600K (4 cores), shared with two other sprint workstreams, so **wall
time is noisy; read the user-time column**. Scripts: `crow_gen.py`, `bench_crow.py`, `add_drop_oracle.py`
(this directory). Raw JSON: `results.json`.

## 1. Analytic oracle: `.dc` sweep of `circuits/add_drop.cir` vs closed form

`add_drop_oracle.py` compares the 30-point wavelength sweep (1535-1564 nm) of the shipped add-drop netlist
(two couplers `k=0.85`, two half-rings of 101 x lambda0/neff with 2 dB/cm, `abstol=1e-12 reltol=1e-6`)
with the Yariv add-drop transfer function evaluated with SPECS's own waveguide dispersion law.

| quantity | max abs error (W) | max rel error |
|---|---|---|
| through (`PROBE{ROOT/out}.power`) | 3.7e-7 | 3.3e-4 (at the 1550 nm null, P = 5.2e-6 W); 7e-6 elsewhere |
| drop (`PROBE{ROOT/drop}.power`) | 1.4e-6 | 1.5e-6 |

Takeaways: (a) the frequency-domain fixed point converges to the analytic result to within the netlist's
`reltol`, so **closed-form oracles are a cheap, deterministic test class** (feature F-06); (b) the match
requires treating the `k` keyword as the *field* cross-coupling (`kappa = 0.85`, through power 0.2775), which
settles the k-vs-kp ambiguity raised in `discover/02-devices-physics.md` section 4 in favour of the code.

## 2. CROW benchmark (paper Fig. 3): scaling, tracing cost, WDM, resolution, tolerances

Generated with `crow_gen.py` (seed 1): N rings, N+1 couplers `kp=0.15`, 2N waveguides `L ~ N(33.2, 5) um`,
`neff=2.2 ng=4.3 att=2 dB/cm`, 10 x 1 ns 1 mW pulses at 500 MHz, `.tran 20e-9`, `abstol=1e-8 reltol=1e-4`,
`timescale=-13` (100 fs) unless stated. Paper normalisation: x0.015 (20 ns -> 300 ps).

| case | N | tracing | user s | wall s | VCD MB | user x0.015 (ms) |
|---|---|---|---|---|---|---|
| A | 3 | 2 probes | 0.37 | 0.42 | 7.5 | 5.6 |
| A | 10 | 2 probes | 3.79 | 4.18 | 32.2 | 57 |
| A | 30 | 2 probes | 5.72 | 12.5 | 15.4 | 86 |
| A | 100 | 2 probes | 5.75 | 11.3 | 15.4 | 86 |
| A | 300 | 2 probes | 6.09 | 12.7 | 15.4 | 91 |
| C | 3 | none | 0.22 | 0.24 | 0 | 3.3 |
| C | 30 | none | 4.94 | 5.98 | 0 | 74 |
| B | 3 | traceall=1 | 1.34 | 3.45 | 65 | 20 |
| B | 10 | traceall=1 | 15.1 | 37.2 | 729 | 226 |
| B | 30 | traceall=1 | 21.9 | 32.7 | 1003 | 328 |
| D | 3 | 2 probes, 4 WDM channels | 1.31 | 1.53 | 8.5 | 20 |
| D | 3 | 2 probes, 16 WDM channels | 6.90 | 8.19 | 10.8 | 103 |
| D | 10 | 2 probes, 4 WDM channels | 18.2 | 19.9 | 37.2 | 273 |
| E | 3 | 2 probes, **1 fs** resolution | 88.0 | 129 | 1129 | 1320 |
| F | 10 | 2 probes, abstol 1e-10 / reltol 1e-6 | 3.91 | 4.25 | 35.0 | 59 |

Elaboration only (`specs -n`, no probes): N=300 in 0.11 s user / 17 MB RSS (902 modules); N=1000 in 1.5 s
user / 42 MB RSS (3002 modules). A full N=300 run with 2 pulses and no probes peaks at 27 MB RSS.

Takeaways:
1. **The paper's plateau reproduces**: user time is flat from N=30 to N=300 (loss, not tolerance, stops
   propagation: tightening tolerances 100x changes N=10 by 3 %). Normalised time (~86-91 ms) is about 2x
   the paper's ~40 ms on a different CPU with VCD output on; with no tracing it is ~74 ms. A `bench/` target
   can therefore assert `t(300)/t(30) < 1.5` and track absolute numbers (F-06).
2. **Tracing dominates cost and disk**: the default `trace_all_optical_nets = 1` (`src/specs.cpp:38`) makes a
   30-ring run 4.4x slower and writes a 1 GB VCD; even two probes write 15-32 MB because every event of
   every wavelength is a VCD record. Output format, probe selection and decimation are first-order
   performance features, not conveniences (F-05).
3. **WDM cost is linear in channel count** (N=3: 0.39 s -> 1.31 s -> 6.9 s for 1/4/16 channels; N=10: 4.8x
   for 4 channels). Each wavelength is a separate event through separate coroutine switches; a per-event
   wavelength vector would cut the constant factor but not the O(channels) physics (F-08).
4. **Time resolution is the sharpest knob**: 1 fs costs 240x over 100 fs for N=3 (events of different ring
   round trips no longer coalesce on the same tick, so the number of distinct event times explodes, and the
   VCD grows to 1.1 GB). Event coalescing windows and sampled-time islands (F-09) would decouple accuracy
   of *delays* from the number of *emitted* events.
5. Memory is not a scaling limit: SystemC thread stacks are lazily touched; 3002 modules fit in 42 MB.

## 3. Profile (perf, N=10, no probes, 100 fs)

Top self-time symbols (`perf record -F 499 -g`, 1567 samples):

| share | symbol | note |
|---|---|---|
| 13.4 % | `sc_core::sc_event::trigger()` | kernel event delivery: one `sc_event_queue` notify + one signal `value_changed` per hop |
| 11.4 % | `__hypot_finite` | `std::abs(complex)` in `check_emit_by_abstol/reltol` and the `10*abstol` floor (`src/optical_output_port.cpp:63,70-71,169`), three `hypot` per emitted event |
| 10.7 % | `OpticalOutputPort::on_data_ready()` | pop + gate + write |
| 7.8 % + 3.9 % + 2.1 % + 2.1 % | `qt_abort`, `next_cor`, `yield`, `wait` | **~16 % coroutine switching**: every device process and every output port is an `SC_THREAD` |
| 4.6 % | `delayedWriteEventDriven` | linear `find_if` over the pending queue (`:341`) |
| 4.4 % | `sincosf32x` | `polar()` recomputed per event in `WaveguideUni::on_port_in_changed` (`src/devices/waveguide.cpp:62-65`) although the phase only depends on lambda |
| ~4 % | `malloc`/`_int_free` | `sc_event_queue::notify` allocates an `sc_time` per call; `PQueue::push` |
| 3.8 % | `sc_ppq_base::heapify/insert` | the kernel's timed-event heap, duplicated by the port's own `PQueue` |

Cheap single-thread wins with predictable golden impact (bit-identical or better): squared-norm comparisons
instead of `abs`, per-wavelength S-parameter caches, `SC_METHOD` for stateless devices and output ports,
one timed queue instead of two. Expected gain: 1.5-2x; the big levers remain tracing (section 2.2) and the
number of emitted events (tolerances, resolution, coalescing).
