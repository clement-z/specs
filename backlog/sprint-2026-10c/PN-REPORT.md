# PN report: equal-time order by wavelength, then a per-time-step queue (branch `ws/timeorder`)

Base: `ws/timeorder` at ec05f36. Decisions: `backlog/DECISIONS-2026-10-21.md`, rows "Equal-time order" and
"Equal-time order, follow-up" (2026-10-24). Details and every measurement: `backlog/sprint-2026-10c/PN-progress.md`.

## Items

| # | Item | Outcome | Commit | Tests | Golden / counters |
|---|---|---|---|---|---|
| 1 | Equal-time pending entries ordered by wavelength, smallest first (heap kept) | done | f1b94ea (prediction), 2674103 | gtest `PendingEqualTimesBySmallestWavelength` (new); `PendingQueueMatchesLinearScanQueue` against a (time, wavelength) reference; 4 order expectations updated | 6 cases re-recorded, all in the predicted set; new pinning case; `stats-compare` vs base: 3 differ, as predicted |
| 2 | Per-time-step queue (sorted ring buffer) instead of the heap | done, kept: faster at every channel count | 94ca1f3 | `PendingQueueMatchesLinearScanQueue` extended (`write()`, every entry checked, ring growth/wrap/insertions on both sides) | strict golden unchanged (no re-record); `stats-compare` step 1 vs step 2: 278 same |
| 3 | Docs | done | 2674103 (numerics, CHANGELOG Changed), 4ab8bf6 | | |

### 1. Order by wavelength (2674103)
`PendingEntryLater` orders by time, then by the wavelength of the id (a per-queue table filled from the
registry), smallest first. (time, wavelength) is unique and a merge keeps the key, so the order is total and
no longer depends on the heap layout, merges or the standard library.

**Prediction first (f1b94ea).** A scratch build of the base (`PN-wip/instr`, not merged) counted, at every pop,
whether the wavelength order would have taken another equal-time entry than the |E| order. 11 cases disagree;
every other case has 0 disagreements, so it is bit-identical by construction. Of the 11, **6 changed** (+ the
gtest list):

| case | change |
|---|---|
| `tb/lambda` | testbench listing of OUT: the two wavelengths of a time step in the other order |
| `regress/b001-fd-multilambda-stale` | `.op` listing: nets m, o, u show 1560 nm (the last write) instead of 1550 nm |
| `regress/f15-stats-file-print` | same listing, and `pending_mean` (Merger 1.625 -> 1.5, `ROOT/MERGERM` 1.75 -> 1.5, totals) |
| `regress/b062-mlprobe-computed-wavelength`, `regress/b184-mlprobe-tran-start-multilambda` | operating-point listing before the `.tran`: net m shows 1560 nm instead of 1550 nm |
| `regress/f15-wdm-crow-mlprobe` | MLPROBE at 1551.2 nm only: up to 3.76e-8 sqrt(W) on a peak of 9.11e-4 (4.1e-5 of the peak); other channels identical |

Predicted but unchanged: `b033-crossing-two-lambda`, both `b186-*`, both `probe-latch-dc*`. One per-case miss:
for b062/b184 I had predicted a CSV row-order change; what changed is the listing (a category of the
prediction and of the decision: "listings of multi-wavelength nets"). No change outside the predicted set.

**Pinning case** `regress/pn-wdm-crow-equal-time-order` (`tests/golden/netlists/pn-wdm-crow-equal-time-order.cir`:
`crow_gen.py 3 --nwl 4 --pulses 1 --duration 4e-10`, MLPROBEs on through, r2b_out and drop, default
tolerances; VCD stored gzipped, 460 kB): against the base, 1551.2 nm moves by up to 9.0e-5 of its peak, the other
three channels are identical. Recorded with `--repeat 3`.

Re-record: `--only` the 6 cases + `gtest/unit_tests` + the pinning case, note "equal-time order by wavelength,
decision 2026-10-24 (phase N) ...", in the code commit. `stats-compare` base vs step 1: 274 same, 3 differ
(b001 and f15-stats-file-print: `pending_mean`; the WDM CROW: couplers emitted 44085 -> 41640, coalesced 18644 ->
21094, waveguides coalesced 2445 -> 0, ...), 1 new.

Docs: `doc/numerics.md` "Order within a time step" (the order, last write on multi-wavelength nets, the
tolerance-level dependence of per-wavelength values, B-198); CHANGELOG `[unreleased]` Changed;
`discover/01-architecture-engine.md` (the one sentence on the |E| order).

### 2. Per-time-step queue (94ca1f3)
`OpticalPendingQueue` keeps the entries in (time, wavelength) order in a power-of-2 ring buffer: each time step
is one run (bucket) in wavelength order, `pop()` is O(1), and `write()` merges or adds in one call: append when
later than the last entry (most writes; the entries of a time step usually arrive in wavelength order because
the ports upstream emit them in that order), direct merge into the last entry, else a binary search and an
insertion shifting the shorter side. The B-015 per-wavelength index is gone. The port calls
`notify_pending()` before `write()` (a value earlier than every pending entry is never a merge: same
notifications). Same order as step 1, so the same emissions.

Bit-identity to step 1: strict golden 279 passed with no re-record; `stats-compare` 278 same; beyond the suite,
CROWs (3 rings 4 channels with every net traced, 61 MB VCD; 3 rings 64 channels traced; 10 rings 16 channels):
VCD, stdout and counters identical.

ns per processed event (gcc release, idle host, no build during the runs, `bench.py`'s `run_once`, user time,
median of 3, N = 30 of 5, step 1 and step 2 interleaved):

| case | step 1 | step 2 | ratio |
|---|---|---|---|
| N=3, 1 ch | 119.8 | 115.6 | 0.965 |
| N=3, 4 ch | 116.2 | 102.6 | 0.883 |
| N=3, 16 ch | 132.1 | 106.8 | 0.808 |
| N=3, 64 ch | 157.6 | 117.7 | 0.747 |
| N=10, 1 ch | 128.9 | 122.9 | 0.954 |
| N=10, 4 ch | 146.2 | 130.7 | 0.894 |
| N=10, 16 ch | 188.9 | 139.6 | 0.739 |
| C-notrace N=30, `--no-trace` | 151.9 | 140.3 | 0.923 |
| C-notrace N=30, `-o` | 150.0 | 140.0 | 0.933 |

Single channel: faster (-3.5 % to -7.7 %). Cost at 16 channels / 1 channel: N = 3 0.92, N = 10 1.14; 64 / 1
(N = 3) 1.02. Profile (perf, N = 10): `emit_due_entry()` 16.5 % (1 ch) and 19.8 % (16 ch), PM measured 17 % /
34 % with the heap. B-015 delay line (about 1230 entries pending per port): 0.10 -> 0.09 s user, same VCD.

### 3. Docs (4ab8bf6)
`backlog/B-188-*.md` (what remained after PM, the phase N fix, ns/event table PM / step 1 / step 2, what
remains), `backlog/B-015-*.md`, `backlog/B-198-*.md` (measured effect of the order change), `backlog.md` rows,
CHANGELOG `[unreleased]` performance entry, comments in `src/optical_pending_queue.h` and
`src/optical_output_port.cpp`.

## Decisions I had to take (conservative)
- **"By wavelength" = by the wavelength value (m), not the id**, as the PROBE latch compares values. Ids are
  registration order (the CROW registers 1551.2 nm first, so the value order is the reverse of the id order).
- **Unregistered id.** A signal without a wavelength has id 0 even when the registry is empty (the legacy
  `-t phaseshifter` testbench pushes such an entry from a device memory before any source writes; a lookup
  then exited with "Wavelength not found"). Such an id sorts after the registered ones and is looked up again
  at its next write. Its key can change once it is registered; that cannot reorder anything while it is the
  only id (empty registry). New static `OpticalSignal::registeredWavelengths()` for this.
- **`compareSquaredMagnitudes()` moved** from `optical_pending_queue.h` to `optical_output_port.h` (the
  emission gate is its only user now); unchanged.
- **Pinning case**: a new case next to `f15-wdm-crow-mlprobe` (which only probes the drop): MLPROBEs on three
  nets, 0.4 ns, to stay under 1 MB stored. `f15-wdm-crow-mlprobe`'s note in `cases.json` says it was
  re-recorded for phase N; its netlist is unchanged.
- **Step 2 design**: a sorted ring buffer (buckets as contiguous runs) rather than one container per time step:
  no allocation or indirection per bucket; merges other than into the last entry use a binary search
  (O(log pending)) rather than a per-wavelength index (positions move on insertions). Measured faster at every
  channel count, so kept.
- `src/utils/pqueue.h` restored to its pre-PN content (step 1 had added a constructor for the stateful
  comparator; nothing uses `PQueue` now).
- Timing PM -> step 1 is not measured in this session (the B-188 table quotes PM's numbers from another
  session); the brief asks for step 1 vs step 2.

## Found, not done
- `PQueue` (`src/utils/pqueue.h`) is no longer used anywhere; `OpticalSignal::operator<` (inverted |E|
  comparison) no longer orders the pending entries and may be unused.
- Two *unregistered* wavelength ids in one port would tie (both sort last) and could not be merged correctly;
  only the default id 0 can be unregistered in practice.
- The test `DcPointWritesFirstValueOfEachWavelengthWithAnEvent` says it covers a forced write "equal to the
  value its net holds", but with two alternating wavelengths every write changes the net (before and after
  this phase).
- `discover/01-architecture-engine.md` is otherwise still stale (PM's note).
- A 20000-entry inline `values=[...]` list fails with `memory exhausted` (parser depth): already recorded in
  B-191; a value file works.

## Final results (tip 14862ed, code of 94ca1f3)
- `pixi run test gcc`: 17/17 OK. `pixi run test asan-clang`: 17/17 OK, no AddressSanitizer, LeakSanitizer or
  UBSan message in the test log; `tests/sanitizers/lsan.supp` unchanged. `pixi run test clang`, `pixi run test
  asan-gcc`: 17/17 OK each (asan-gcc log clean as well).
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: `Summary: 279 passed (0 with notes), 0 failed,
  0 new, 0 missing, 0 skipped`. Before the phase: 278 passed.
- clang and asan-gcc, base (ec05f36) vs tip, `record` then `compare --rtol 0 --atol 0 --print-slack 0`: each
  `271 passed, 7 failed, 1 new`: the same 6 cases as gcc + `gtest/unit_tests`, and the new pinning case.
- `stats-compare` (gcc): base vs step 1 274 same, 3 differ (predicted), 1 new; step 1 vs step 2 278 same.
