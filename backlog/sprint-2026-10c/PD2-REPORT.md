# PD2 report: F-15 part 2 (bit-identical engine refactors)

Branch `ws/engine2`, from `sprint/2026-10c` (56e5808). Decisions applied: F-15 "Follow-up decisions", Q1, Q2, Q8
of `backlog/DECISIONS-2026-10-21.md`. No `git push`, no merge. **Decision Q8 (time-only order) is not applied:
stopped, see item 3.**

## Items

| # | Item | Outcome | Commit | Tests | Golden impact |
|---|---|---|---|---|---|
| 1 | Tolerance gate on squared norms | done | e50d693 | gtest `OpticalOutputPort.SquaredNormGateMatchesFormerGate` | none (gtest re-recorded) |
| 2 | Per-wavelength S-parameter caches (waveguide, coupler, phase shifter) | done | 23bae2f | gtest `TransmissionCache.*`; golden `regress/f15-dispersive-waveguide` (new) | none (new case recorded with the build before the change; gtest re-recorded) |
| 3 | Pending events [Q8] | **partly done.** The bit-identical part landed: `sc_event_queue` replaced, no allocation per notification. **Time-only order stopped** (beyond Q8's prediction) | 67d1702; WIP `PD2-wip/q8-time-order` 935f362 (not merged) | gtest `OpticalOutputPort.Pending*` | none (gtest re-recorded) |
| 4 | `OpticalSignal`: no virtual destructor, id behind a compile-time flag [Q2] | done (`-Dsignal_ids`, off by default) | a935b7e | `-Dsignal_ids=true` build: unit tests pass, gives the former listings | 127 cases re-recorded, `[NNNN]` prefix only (predicted, Q2) |
| 5 | Bench after each step | done | — | — | — |

### 1. Squared-norm gate (e50d693)
A plain `norm(d) > abstol²` gate does **not** take the same decisions as `abs(d) > abstol`. Two kinds of
input flip:
- values one ulp from the threshold (rounding);
- subnormal changes with zero tolerances (`norm` underflows to 0). The existing test
  `EmissionGateWithZeroTolerances` catches this case.

`compareSquaredMagnitudes()` decides on squared norms only when both squares lie in [1e-280, 1e280] and differ
by more than a relative 1e-13 (the roundings are below 1e-14). Otherwise the former `abs()` expression decides.
So the decisions are those of the former gate for every input, including zero, NaN and infinities. Same
treatment for the snap test (`check_snap_to_zero()`). The two F-28 report magnitudes are now computed only when
the report needs them. The new test compares the gate, the reltol test and the snap against copies of the
former expressions:
- over 100,000 inputs and 27 tolerance settings;
- the inputs include zero, -0, subnormals, huge values, inf, NaN, and ±3 ulps around every threshold;
- mutation-checked: dropping the guard, or setting the margin to 0, makes the test fail.

### 2. Transmission caches (23bae2f)
- **Waveguides:** S12 is cached per wavelength id (`WavelengthCache`, `src/utils/transmission_cache.h`); with
  dispersion, the group-delay `sc_time` is cached too. The dispersion-free delay `sc_time` is built once per
  thread.
- **Couplers:** the delay `sc_time` is built once per thread (their S-parameters were already constant).
- **Phase shifters:** `PolarCache` recomputes `polar()` only when the phase changes. Phases are compared bit for
  bit, so -0 and +0 stay distinct. The voltage reaction computes the transmission once for all wavelengths.

The same expressions are evaluated, only less often. No golden case had a dispersive waveguide (D ≠ 0), so I
added `regress/f15-dispersive-waveguide` (uni and bidirectional), recorded with the build before the change.
Also checked by hand: a 4-λ CROW with D ≠ 0 (`.tran`, 3 M VCD lines) and an add-drop `.dc` sweep with D ≠ 0 give
identical outputs before and after.

### 3. Pending events (67d1702 landed; Q8 stopped)
**Landed (bit-identical everywhere).** Each `OpticalOutputPort` now makes the notifications of its former
`sc_event_queue` itself:
- one `sc_event`;
- an allocation-free heap of pending times (`std::priority_queue` over a reused vector);
- `notify_pending()`, which matches `sc_event_queue::notify`;
- the method `fire_pending()`, which matches `fire_event`. It is created before the port thread, as the queue's
  method was.

The kernel therefore runs the processes in the same order. The coalescing heap keeps its order: time, then |E|,
ties included. Checked beyond the golden suite: on a 4-λ CROW, the VCD with every net traced, the VCD with an
MLPROBE on every net, stdout (signal ids included) and all port counters are identical to the previous commit.

**Stopped: decision Q8 (time-only order, write order on ties, merge as a lookup).** The full implementation is on
`PD2-wip/q8-time-order` (935f362, keep, do not merge; its golden re-records are included).

- **Golden suite: as predicted.** 224 of 228 cases are bit-identical. The changed ones:
  - `tb/lambda`, `regress/b001-fd-multilambda-stale`, `regress/b050-pdet-beat`, `regress/f15-stats-file-print`:
    listings of multi-wavelength nets show another wavelength as the last write of the time step;
  - `regress/f15-stats-file-print` also changes one statistic: `pending_mean` of `ROOT/MERGERM`, 1.75 → 1.5.
- **Event counters:** 225 of 227 cases identical. The two b001 cases differ only in that `pending_mean`.
- **Beyond the prediction: per-wavelength values change.** Circuit: `discover/scout/bench/crow_gen.py 3 --nwl 4`
  (3 rings, 4 wavelengths from one VL source, 20 ns), with an MLPROBE on all 14 nets for the 4 wavelengths:
  - at default tolerances (abstol 1e-8, reltol 1e-4), 56 of 57 MLPROBE magnitude signals differ;
  - the largest difference is 5.3e-6 √W on a peak of 6.4e-3 √W (8.4e-4 relative, about 8× reltol), inside
    ring 2 (`r2b_in`, `r2b_out`); at `through`, 3.0e-6 √W (9e-5 relative);
  - per-port counts change too, e.g. `ROOT/COUPLER1.out1`: processed 228425 → 212000, emitted 216626 → 198920;
  - **at abstol = reltol = 0 (3 ns), all 176 MLPROBE signals are identical.**
- **Mechanism.** A device with two inputs (a coupler) receives the wavelengths of one time step in different delta
  cycles. When the equal-time order changes, its zero-delay outputs carry different intermediate (partially
  updated) values. The emission gate compares each value with the last one emitted at its wavelength, and that
  includes those intermediate values. So whether a value is emitted, and which value is left within tolerance,
  depends on the equal-time order. The differences then accumulate around the rings. The |E| order has the same
  dependence; it is simply a different arbitrary order.
- **Variant tried: one `sc_event` re-armed by the port thread, |E| order kept** (`PD2-wip/single-event-thread-rearm`,
  cd450ed, not merged). Golden 228/228 bit-identical. On the 4-λ CROW, per-wavelength values are identical, but
  13 of 80 plain-probe signals and several port counters change. The cause is the kernel's process order: the
  re-arming moved from the method pass to the thread pass of the delta cycle. Not landed for that reason. D3's
  `SC_METHOD` conversion will change the process order anyway.

**Options for Q8 (for the maintainer):**
1. Keep the current tie order (|E|, heap ties) as on `ws/engine2` today. Bit-identical. It forgoes the
   time-only queue's speed: 1.51× instead of 1.86× (see below).
2. Accept and re-record: merge `PD2-wip/q8-time-order`. Per-wavelength values of multi-wavelength circuits then
   change within tolerance-level amounts. The golden suite changes only as predicted, so this would need a golden
   case that pins the effect (e.g. the 4-λ CROW with MLPROBEs) and a note in `doc/numerics.md`.
3. Make the result independent of the equal-time order: gate each wavelength against its value at the end of the
   previous time step, not against intermediate same-time emissions. This is a change of the emission-gate
   semantics (physics/numerics, `needs-human`). It would make both orders give the same per-wavelength values,
   but it changes current outputs.

### 4. `OpticalSignal` (a935b7e)
- The id, its global counter and its `sc_trace` line exist only with `-Dsignal_ids=true`. This is a new Meson
  option that defines `SPECS_SIGNAL_IDS`; it is documented in `meson.options`, README, AGENTS.md and CHANGELOG.
- Without the flag, the copy constructor and assignment are defaulted, there is no virtual destructor, and a
  signal is a 24-byte value (40 before).
- Listings print `@ 1550 nm (...)` instead of `[0042]  @ 1550 nm (...)`.
- Re-recorded the 127 cases whose files change. Every file of every case is identical to the former baseline once
  `[NNNN]  @ ` is replaced by `@ ` (checked file by file for gcc, clang and asan-gcc against their previous
  recordings). The `-Dsignal_ids=true` build reproduces the former listings (same check, reversed).

## Performance (CROW bench, `discover/scout/bench`)
Pixi gcc 15 release builds of each step, interleaved, median user time, same host and session (shared, load 1.3 to
4.7). Case `C-notrace N=30` (`crow_gen.py 30 --traceall 0 --probes 0`), 5 rounds:

| build | median user s | rounds | speed-up vs base |
|---|---|---|---|
| base (56e5808) | 4.710 | 4.721, 4.691, 4.744, 4.710, 4.667 | 1.000 |
| item 1 (e50d693) | 4.050 | 4.108, 4.050, 4.050, 4.045, 4.098 | 1.163 |
| item 2 (23bae2f) | 3.788 | 3.788, 3.742, 3.776, 3.795, 3.822 | 1.243 |
| item 3 (67d1702) | 3.196 | 3.232, 3.196, 3.211, 3.167, 3.192 | 1.474 |
| **item 4 (a935b7e) = D2** | **3.110** | 3.143, 3.110, 3.076, 3.092, 3.117 | **1.514** |
| WIP thread re-arm (cd450ed, items 1–2 + variant, no item 4) | 2.765 | 2.768, 2.703, 2.739, 2.776, 2.765 | 1.703 |
| WIP Q8 (935f362, items 1–2 + Q8, no item 4) | 2.527 | 2.519, 2.537, 2.550, 2.527, 2.503 | 1.864 |

Other cases, 3 rounds, D2 vs base: `A-scaling N=30 probes=2` 5.132 → 3.513 s (1.461×), `D-wdm N=10 nwl=4`
17.569 → 11.921 s (1.474×). All runs simulated 20 ns and exited 0. The [Q7] target (≥ 1.5× on `C-notrace N=30`
by the end of D3) is already met on this host.

## Decisions I had to take (conservative)
1. **Item 1:** a guarded squared-norm gate, with `abs()` deciding near thresholds and out of range, instead of a
   plain one. A plain one is not decision-identical (shown by the test).
2. **Item 2:** caches are local to each device thread, like the parameters those threads already read once at
   start. A `WavelengthCache` vector is indexed by wavelength id, so its memory is O(number of wavelengths in
   the run) per device. Couplers and phase shifters have no wavelength dependence; I cached what they recompute
   per event (the delay `sc_time`, `polar` of the phase).
3. **Item 3:** stopped the time-only order (above). Landed only the exact `sc_event_queue` emulation, including the
   re-arming method, because removing the method changes the process order outside the golden suite. "Merge as a
   lookup" stays the former linear `find_if` (it belongs to the time-ordered structure).
4. **Item 4:** without ids, listings start at `@` (the leading space of `" @ "` is dropped with the prefix). The
   flag is a Meson option, `signal_ids`.
5. A golden case for dispersive waveguides was added (coverage gap for item 2), recorded with the base build.

## Found, not done
- **Maintainer proposal (2026-10-23, verbatim; relayed to the coordinator, who scopes it as a separate item):**
  "This blow up seems like a very logical finding. You will have an exponentially growing list of signals" /
  "Expected behavior. And it has only physical explanation if you work with deltas, not with fields. I think we
  can drop the deltaE idea too." Coordinator guidance: do not remove the no-coalescing switch or `m_use_deltas`
  in PD2.
  - Finding behind it: `--no-coalescing` on the 4-λ CROW (`crow_gen.py 3 --nwl 4`) exhausts memory:
    `std::bad_alloc` under a 3 GB limit within 0.5 ns simulated (base build too), and 4.9 GB RSS after 10 min of
    CPU for the 20 ns run (stopped by me).
  - Without coalescing, every write is its own entry; in a ring, each coupler event writes both outputs, so the
    number of entries grows exponentially with the round trips.
  - `doc/numerics.md` (Expert options) says that without coalescing "MLPROBEs and per-wavelength values do not
    change". The item 3 mechanism suggests that intermediate values going through the gate can change them at
    tolerance level. Not verified, because the run cannot complete.
- The coalescing heap edits merged entries in place without re-sifting. After a merge, equal-time entries are not
  strictly in |E| order (heap order on the secondary key; time order is unaffected). Pre-existing; documented in
  `OpticalOutputPort.PendingSameTimeAndWavelengthMerges`.
- MLPROBE at the start of a `.tran` (sampling after the operating point) seems to record only the wavelength last
  written to the net. Example: two CW sources at 1.55 and 1.56 µm into a merger, MLPROBE on its output: 1560 nm
  reads 0 for the whole `.tran`. Same with the base build. The merger itself keeps one memory per wavelength.
- The `discover/01-architecture-engine.md` audit keeps its 2026-10 line references. I only noted the F-15 changes
  (`sc_event_queue`, `m_id`).

## Final results (tip a935b7e + doc commits)
- `pixi run test gcc`: 12/12 OK.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: 228 passed, 0 failed/new/missing. That is 226
  pre-existing `specs` cases, 1 new, and gtest.
- Event counters (`stats-record` / `stats-compare`, gcc): identical to the base for all 226 pre-existing cases
  after every item, and for all 227 cases between items 2, 3 and 4.
- clang release and asan-gcc recordings, strict:
  - items 1–3: identical to the build before each item;
  - item 4: identical up to the predicted id prefix.
- `pixi run test asan-clang`: 12/12 OK, no sanitizer message, after every item. `tests/sanitizers/lsan.supp`
  unchanged.

## Housekeeping
Heartbeat `8e2ab409` deleted after this commit. WIP branches `PD2-wip/q8-time-order` (935f362) and
`PD2-wip/single-event-thread-rearm` (cd450ed) are kept for the maintainer. Scratch in `/tmp/pd2` (binaries per
step, recordings, scripts `gate.sh`, `gate4.sh`, `idcheck.py`, `vcdcmp.py`, `vcdnum.py`, `bench.py`) is not
needed for the merge. Build dir `build-pixi/gcc-ids` (`-Dsignal_ids=true`) is local only.
