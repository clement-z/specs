# Tolerances and time resolution

SPECS does not step through time. A device that receives an event computes its outputs and schedules them
after its delay. Two settings decide how many events are produced and how exact the result is. The paper
(Zrounba et al., ECOC 2023, `doc/papers/zrounba2023-specs-ecoc.pdf`) lists them among the cursors a user can tune:

> Temporal resolution — sets the precision with which events are resolved. Decreasing resolution can greatly
> reduce simulation time, but will lower the precision of the final waveforms by masking what happens between events.

> Absolute and Relative tolerance — relax precision constraints by specifying a threshold below which small
> changes do not need to be emitted by devices. Without them, events representing infinitesimal changes in
> field values could propagate indefinitely in circuits with feedback. These should not be conflated with
> similarly named settings of time-driven simulators which, even though they share similar goals, typically
> control the time-step of the solver.

The paper's benchmarks used abstol 1e-8 (written V m⁻¹ there, 0.1 fW) and reltol 0.01 % (1e-4), at 100 fs and
1 fs resolution. Those abstol and reltol are the defaults.

## Quick reference

| `.options` keyword (case-insensitive) | Default | Meaning | Valid | CLI |
| --- | --- | --- | --- | --- |
| `abstol` | 1e-8 | absolute emission threshold, optical **field**, √W | >= 0 (see [Zero tolerances](#zero-tolerances)) | `--abstol` |
| `reltol` | 1e-4 | relative emission threshold | >= 0 (see [Zero tolerances](#zero-tolerances)) | `--reltol` |
| `snapfactor` | 10 | emitted fields below snapfactor·abstol become 0 | >= 0 (0: off) | none |
| `timescale` (alias `ts`) | -13 | log10 of the engine time step in s: -13 is 100 fs | integer, -15 (1 fs) to 0 (1 s) | `-R`, `--resolution` |
| `resolution` | 1 | port time step = `resolution` × engine step | >= 1 | none |
| `seed` | 0 | master seed of the random numbers (see below) | integer: a literal 0 to 9223372036854775807 (2^63 - 1), exponent form 0 to 2^53 | `--seed` (0 to 2^64 - 1) |
| `maxiter` | 1000000 | iterations (delta cycles) of the `.op`/`.dc` fixed point, per operating point and per sweep point (see below) | integer: a literal 0 to 2^63 - 1, exponent form 0 to 2^53 (0: no limit) | `--maxiter` (0 to 2^64 - 1) |
| `coalesce` | 1 | expert: merge the writes of a port for the same time and wavelength (see [Expert options](#expert-options-ablation)) | 0 or 1 | `--no-coalescing` |
| `maxevents` | 0 | expert: stop the run after this many processed entries (0: no cap) | integer, as `maxiter` | `--max-events` (0 to 2^64 - 1) |

Example: `.options abstol=1e-6 reltol=1e-8 timescale=-12` (as in `circuits/add_drop_pyspecs.cir`).

- An invalid value is an error and the simulation does not start.
- With `-f`, the CLI values are appended to the netlist as `.options` lines, so they override the netlist.
  They are ignored by the legacy testbenches (`-t`), except `--seed`, which is applied directly: it also
  sets the seed of the testbenches and takes priority over a netlist's `.options seed`. `--maxiter`,
  `--no-coalescing` and `--max-events` are applied directly too and take priority over `.options maxiter`,
  `coalesce` and `maxevents`.
- `seed`: every random draw (today the photodetector noise and dark field, see `doc/netlist.md`) comes
  from a stream derived from the master seed and the hierarchical name of the device, so a run is
  reproducible: the default seed is 0, and the same netlist and seed give the same values. An integer
  literal (`seed=3000000000`) is taken exactly, 0 to 9223372036854775807 (a larger literal is a parse error,
  B-180); a number with an exponent or a scale suffix (`seed=3e9`) is a double, exact as an integer up to
  2^53 only, hence that limit. `--seed` takes any decimal unsigned 64-bit integer. The same holds for
  `maxiter`. See [reproducibility.md](reproducibility.md).
- `-R` sets the **timescale**, not `.options resolution`: `-R -14` is `.options timescale=-14`.
- All optical output ports share these values. C++ testbenches can configure single ports differently.

## Tolerances

Every optical output port keeps, per wavelength, the last field it emitted, E_last. A new field E is
emitted only if both tests pass:

    |E - E_last| > abstol    and    |E - E_last| > reltol · |E_last|

that is |E - E_last| > max(abstol, reltol·|E_last|). E is complex: a pure phase change also counts. This is
not SPICE's rule (abstol + reltol·max(|new|, |old|)), and the tolerances limit **which events are emitted**,
not a solver step. Consequences:

- abstol is a field: 1e-8 √W is 1e-16 W. A field of 1e-9 √W is never emitted, even if nothing was emitted
  before (the abstol test applies to the first emission too).
- When E_last is 0 (or nothing was emitted yet), the reltol test passes for any change.
- The tolerances apply in `.tran`, `.op` and `.dc`. A photodetector's periodic samples (two or more
  wavelengths beating, see `doc/netlist.md`) use the same two values on its output current.

### Snap to zero

An emitted value with |E| < snapfactor·abstol is emitted as exactly 0. By default that is 1e-7 √W
(1e-14 W). It only matters with a large abstol. Three 1 cm waveguides at 20 dB/cm (field × 0.1),
`.options abstol=1e-2`, in `.op` (`tests/golden/netlists/b021-snapfactor*.cir`):

| Output field | `snapfactor=10` (default, below 0.1 √W) | `snapfactor=4` (below 0.04 √W) | `snapfactor=0` |
| --- | --- | --- | --- |
| out_a 0.03 √W (0.0009 W) | 0 | 0 | 0.03 √W |
| out_b 0.06 √W (0.0036 W) | 0 | 0.06 √W | 0.06 √W |
| out_c 0.2 √W (0.04 W) | 0.2 √W | 0.2 √W | 0.2 √W |

A negative `snapfactor` is an error. The snap is part of why activity dies out in lossy circuits. In the
paper's CROW benchmark, simulation time saturates beyond about 20 rings because losses prevent any
meaningful signal "according to SPECS' tolerances" from propagating further.

## Error in circuits with feedback

A change below the threshold is not propagated, so a resonator settles short of its exact value, by more
than reltol. Let r be the round-trip field transmission, and δ = max(reltol, abstol/|E_ring|) the
effective relative threshold. The circulating field is short by a relative error between δ·r/(1-r) and
δ/(1-r). Quantities at the output of a resonator are worse: the error is amplified by the build-up.

Example: lossless all-pass ring on resonance, `tests/golden/netlists/b124-allpass-ring-*.cir` (from
`backlog/repro/a5-allpass-ring-op.cir`). Coupler k = 0.3873 (k² = 0.15), r = t = sqrt(0.85) = 0.9220,
775 µm, neff = ng = 1. Exact: 1 W at the output, 24.626 W circulating. Here r/(1-r) = 11.8. On resonance the
relative error of the output power is 2(1+t) = 3.84 times that of the circulating field, so about 45 to 49 × δ.
Measured with `.op` and `--reltol`:

| reltol | Output (W) | Circulating (W) |
| --- | --- | --- |
| 1e-4 (default) | 0.995098 (-0.49 %) | 24.563207 |
| 1e-5 | 0.999535 | 24.620101 |
| 1e-6 | 0.999952 | 24.625447 |
| 1e-7 | 0.999995 | 24.625996 |
| 1e-9 | 1.000000 | 24.626058 |

In `.tran` (1 W step at 1 ps, 30 ns) the output ends at 0.9950975 W (reltol 1e-4), 0.9999522 W (1e-6) and
0.9999999 W (1e-9). At reltol 1e-9 the default abstol dominates: abstol/|E_ring| = 1e-8/4.96 ≈ 2e-9. Each
decade of reltol costs about ln(10)/ln(1/r) ≈ 28 more round trips (73 ps here) before events stop.

Choosing reltol:

- Take reltol ≈ (target relative error) × (1-r)/r. For a quantity at the resonator output, divide it
  again by the build-up (3.84 in the example).
- Check that abstol/|E| is below reltol for the fields you care about, else lower abstol too.
- If in doubt, divide reltol by 10 and check that the result moves by less than your target.
- Reference: `regress/b124-allpass-ring-tight-reltol` (reltol 1e-9 gives 1.000000 W).

## Convergence of `.op` and `.dc`

`.op`, each point of a `.dc` sweep and the operating point computed before a `.tran` are a fixed point:
every output port works without delay and the circuit iterates in SystemC delta cycles, all at the same time
stamp, until no port emits any more (every change is below the tolerances above). One iteration is one delta
cycle; a signal crossing one device takes about two.

A feedback loop whose round-trip field transmission r is close to 1 needs about ln(δ)/ln(r) round trips, δ
being the relative threshold of the previous section, so a weakly coupled lossless ring (k = 1e-4, r = 1 -
5e-9) needs about 2e9 round trips, and a loop with r = 1 never stops. To keep such a circuit from running
for hours, the fixed point stops after `maxiter` iterations (default 1000000, `.options maxiter`,
`--maxiter`; 0 is no limit) with an error and exit status 1. The message gives the residual: the output
ports that still emitted in the last tenth of the iterations, the largest last change relative to its
threshold first (by order of magnitude, then by name; at most 10 are listed)
(`tests/golden/netlists/f28-lossless-ring-op.cir`):

    Error: .op: the operating point did not converge within 1000000 iterations (delta cycles, .options maxiter)
    Residual: 2 optical output port(s) changed in the last 100000 delta cycles. Last change |dE| and emission threshold max(abstol, reltol*|E_last|), in sqrt(W):
      ROOT/ring_br (ROOT/COUPLER1) @ 1550 nm: |dE| = 0.0001 > 1.38425e-08 (7224.15 x)
      ROOT/ring_bl (ROOT/WG_RING) @ 1550 nm: |dE| = 0.0001 > 1.38425e-08 (7224.15 x)
    A feedback loop with a round-trip transmission close to 1 (lossless or weakly coupled) converges slowly or never: raise .options maxiter (0: no limit), loosen reltol/abstol, or add loss

Each net is shown with the device that writes it. In a `.dc` the first line names the sweep point
(`.dc: sweep point 1 of 3 (WL(ROOT/CWSRC1) = 1.55e-06) did not converge ...`), before a `.tran` it says
`.tran: the operating point before the transient`. A circuit that converges is not affected: same results,
same output.

The default leaves a wide margin: at the default tolerances the golden cases need at most 869 iterations (the
all-pass ring of B-124), a chain of 1000 such rings about 1e5, a CROW of 10 rings 3.5e4. A circuit of a few
devices runs 1e6 iterations in about 0.25 s; a large one is slower per iteration (a 100-ring CROW took 30 s).
If a large circuit stops while the residual keeps decreasing, raise `maxiter`; if the residual stays at the
size of the signal, the loop does not converge (r = 1 or a gain).

## Zero tolerances

`abstol` and `reltol` may be 0: `.options abstol=0 reltol=0`, `--abstol 0 --reltol 0`. A negative value or NaN
is an error (`Error: .options: ABSTOL must be >= 0 (got -1)`, `Invalid abs_tol value`). 0 was rejected by the
checks added for B-018 and B-153; before them only builds without asserts (NDEBUG) accepted it.

- `abstol = 0`: the abstol test passes for any change, and the snap never applies (snapfactor·abstol = 0): a
  decaying field is followed down to the smallest double instead of being cut at snapfactor·abstol.
- `reltol = 0`: the reltol test passes for any change.
- Both 0: the emission gate is off. A port emits every value that differs from the last one it emitted (as a
  complex number, in any bit), and only those. With only one of them at 0, the other one still gates.

When both are 0, stderr shows this once per run (also when the zeros come from `--abstol`/`--reltol`, and
for testbenches and C++ code that call `prepareSimulation()`):

    Warning: abstol = 0 and reltol = 0: the emission gate is off, every change of a value is emitted; a circuit with feedback may never quiesce in .tran (it stops only when its values repeat exactly), and .op/.dc may stop at .options maxiter with a non-convergence error

What to expect (examples in `tests/golden/netlists/b153-zerotol-*.cir`):

- Without feedback, nothing is truncated and the run ends as usual. In `b153-zerotol-open-loop.cir` a
  relative change of 5e-7 and a 1e-20 W value reach the probes; the default tolerances drop the first and
  snap the second to 0.
- `.tran` with feedback: the activity stops only when every value in the loop repeats exactly. A lossy loop
  without input decays geometrically until its field underflows to 0 (or rounds to a value that repeats),
  after about ln(|E|/2.5e-324)/ln(1/r) round trips, r being the round-trip field transmission: 280 round
  trips for r = 0.07 in `b153-zerotol-lossy-ring-tran.cir`. A contracting loop driven by a source usually
  reaches an exact floating-point fixed point (the B-124 ring after a 1 W step: about 470 round trips), but
  nothing guarantees it: the last bits can cycle, and a loop with r close to 1 needs about ln(1e-16)/ln(r)
  round trips. Give such runs a `.tran` duration.
- `.op` and `.dc` need an exact repeat too. The B-124 ring converges in 1865 delta cycles (869 at the
  default tolerances) to the exact result, 1.000000 W out and 24.626059 W in the ring
  (`b153-zerotol-ring-op.cir`). The weakly coupled ring of F-28 stops at `maxiter`
  (`b153-zerotol-lossless-ring-op.cir`); the threshold in the residual is 0 and the ratio reads `inf`:

      ROOT/ring_bl (ROOT/WG_RING) @ 1550 nm: |dE| = 9.99999e-05 > 0 (inf x)

- The phase traced for a field of exactly 0 can read π: a zero field multiplied by a complex transmission
  can come out as (-0, +0), whose argument is π. With abstol > 0 (and snapfactor > 0) the snap writes it as +0.

Use: comparing SPECS with a fixed-step discrete model of the same circuit (same delays, same time step). With
the gate off no event is dropped, so the differences left come from the models, the delay rounding and the
floating-point rounding, not from the tolerances. Keep non-zero tolerances for normal runs: they bound the
number of events and make feedback circuits quiesce.

## Time resolution

The engine time step is 10^timescale s (default 100 fs). It is the SystemC time resolution and the time unit
of the VCD. The port time step is `resolution` × the engine step. Two roundings follow:

1. Every delay is rounded to the nearest engine step (half up) when it is built. A waveguide delay is
   L·ng/c, with the default 100 µm and ng = 2.2637 giving 755 fs.
2. Events are emitted on multiples of the port time step, counted from t = 0: the nearest multiple, or the
   next one if that is in the past. With `resolution` > 1 this also moves zero-delay events: with
   `resolution=3` at 100 fs (grid of 300 fs), a source edge at 1.0 ps is emitted at 1.2 ps.

The rounded delay is what the circuit uses: a ring is simulated with the rounded round-trip time. A delay that
rounds to 0 makes a feedback loop collapse into delta cycles at one time stamp.

### Rounding warning

Before the simulation starts, each waveguide compares its delay (at 1.55 µm for a dispersive waveguide) to the
port time step. If rounding changes it by more than 10 %, stderr shows (`ROOT/WGR` is the device):

    Warning (ROOT/WGR): delay 5.33703e-14 s rounded to 1e-13 s (+87.4 %) at the time resolution 1e-13 s; use a finer time resolution, e.g. .options timescale=-14

A non-zero delay that rounds to 0 is an **error** for a waveguide written in the netlist (also inside a
`.subckt`): the run stops before the simulation, with exit status 1:

    Error (ROOT/WGR): delay 1.33426e-14 s rounds to 0 at the time resolution 1e-13 s (the delayed events would happen in the same time step); use a finer time resolution, e.g. .options timescale=-15

For a waveguide that a device creates internally (the arms of an MZI, the waveguides of OCTANE cells, CROWs
and meshes) and for waveguides built by testbenches, it is a warning, and the simulation runs:

    Warning (ROOT/MZI1/WG1): delay 4.33633e-14 s rounds to 0 at the time resolution 1e-13 s (the delayed events happen in the same time step); use a finer time resolution, e.g. .options timescale=-14

The device decides its own internal lengths, and these are feed-forward (an MZI arm only shifts arrival times by
less than a time step), so they are not rejected. The distinction is where the waveguide comes from, not its
name: the parser marks the waveguides it creates from netlist lines.

"Time resolution" in the messages is the port time step. Apart from the error above, the simulation runs anyway.

- Checked once per waveguide, only where delays matter: `.tran` and event-driven testbenches. `.op`, `.dc`
  and `-m fd` ignore delays, so nothing is reported there. Only waveguides are checked; couplers have no delay.
- A delay of at least 5 port time steps never warns (rounding is at most 0.5 step, i.e. 10 %).
- The default 100 µm waveguide at 100 fs (755 fs rounded to 800 fs, +5.95 %) does not warn.
  `circuits/add_drop_pyspecs.cir` does (2.27 ps ring arms at 1 ps: 2 ps, -11.7 %): use `timescale=-13` or finer.
- Fix: a finer `timescale` (the message suggests the coarsest one that passes), or a smaller `resolution`.
  A coarser step is cheaper but less precise, as the paper says.

In the paper's CROW benchmark (N = 3, L ≈ 350 µm), Interconnect differed from Photontorch and SPECS at 100 fs.
At 1 fs all three agreed, with pairwise RMS errors below about 3 µW.

## Expert options (ablation)

Two switches change how the engine handles events, to measure what a mechanism contributes (ablation
studies, performance work). They are not meant for normal runs, and outputs with them are not covered by the
compatibility of the defaults.

**No coalescing** (`.options coalesce=0`, `specs --no-coalescing`). An output port keeps the values its device
writes in a queue until their time. By default a value written for a (time, wavelength) that is already
pending replaces it (or is added to it, for devices that write deltas): only the last write of a time step
is processed, and the queue holds at most one entry per (time, wavelength). Equal-time entries are then
taken by decreasing |E|. Without coalescing, every write is its own entry, and entries of the same time are
taken in the order they were written (FIFO). The values per wavelength stay the same (the last write is
taken last) but every intermediate value goes through the emission gate and may be emitted, in its own delta
cycle: more events, longer queues. A net holds only the last value written to it, of any wavelength, so a
plain probe (or the `.op` listing) of a multi-wavelength net can show another wavelength at the end of a time
step: `tests/golden/netlists/f15-coalesce-off.cir` shows `v` at 1550 nm where the default shows 1560 nm;
MLPROBEs and per-wavelength values do not change. `--stats` counts the difference (`coalesced`).

**Event cap** (`.options maxevents=N`, `specs --max-events N`; 0, the default: no cap). The run stops once the
output ports have processed N pending entries in total (the `processed` counter of `--stats`), at the end of
that delta cycle, so a few more entries of the same delta cycle are processed too. In a `.tran` the
transient ends there: traces up to that time, `Simulated <t>`, exit status 0, and on stderr

    Warning: .tran stopped at the event cap at 30 ps: 21 pending entries processed (.options maxevents=20)

In an `.op`, a `.dc` or the operating point before a `.tran`, a fixed point cut short is not a result: error
`Error: .op: the operating point stopped at the event cap: ...` and exit status 1. With `--stats`, the
statistics are written in both cases (`"event_cap_reached": true`). Use it to bound a run whose activity may
not die out (zero tolerances, no coalescing, a lossless loop) or to time a fixed number of events.

## Run statistics (`--stats`)

`specs --stats` counts what the optical output ports did during the run and writes it as JSON next to the
trace file, `<name>.stats.json` (`-o name`, default `traces/delete_me`). It shows where the events went: how
many values the devices produced, how many the emission gate dropped and why, how long the queues got. The
counters are kept per output port (no global state) and summed at the end; they cost nothing when `--stats`
is off. Netlists only (`-f`): the `-t` testbenches ignore it.

| Option | Effect |
| --- | --- |
| `--stats` | run totals (section `summary`) |
| `--stats=ports` | also one row per output port: port, net, writer (the device), wavelengths, counters |
| `--stats=devices` | also one row per device type (the C++ class of the device that owns the port) |
| `--stats=ports,devices` | both |
| `--stats-file path` | write the JSON there instead (implies `--stats`) |
| `--print-stats` | also print the JSON on stdout (implies `--stats`) |

Without a trace file (`--no-trace`, `-o ""`) and without `--stats-file`, the JSON is printed on stdout. When
it is written to a file, stdout shows `Run statistics written to <file>`. It is written at the end of the
analysis, and also when the run stops on a non-convergence error or at the event cap (below).

Counters (per port, summed in `totals` and per device type):

| Key | Counts |
| --- | --- |
| `scheduled` | values written to the port by its device (`delayedWrite()`), at a time and wavelength |
| `coalesced` | of which merged into the entry already pending for the same time and wavelength (the later value replaces the earlier one, or is added to it for devices that write deltas) |
| `processed` | pending entries taken at their time |
| `emitted` | of which written to the net with a new value (the gate passed) |
| `suppressed.abstol` | not written: \|E - E_last\| <= abstol |
| `suppressed.reltol` | not written: above abstol, \|E - E_last\| <= reltol·\|E_last\| |
| `suppressed.snap` | the gate passed, but the value was snapped to 0 and 0 was already the last value emitted at that wavelength: written again, no change |
| `pending_max`, `pending_mean` | entries pending in the port after each scheduled value: largest, mean |
| `pending_end` | entries still pending at the end of the run (scheduled after its end) |
| `last_emission_s` | time of the last emitted value (`null` if none); simulated time, 0 for `.op` |

Every value is accounted for: `scheduled = coalesced + processed + pending_end` (plus the entries a `.dc`
drops between two points, normally none) and `processed = emitted + suppressed.abstol + suppressed.reltol
+ suppressed.snap`. The other keys: `analysis` (`OP`, `DC`, `TRAN`), `sections`, `options` (the tolerances,
`snapfactor` and the expert options below), `event_cap_reached`, `end_time_s` (simulated time at the end), and
`wall_time_s`, the wall-clock time from the start of the analysis (elaboration excluded), the only value that
changes from one run to the next. In a `.dc` sweep, the counters cover every point (and the wavelengths of
every point); in a `.tran`, the operating point before it too. Example (`--stats` on
`tests/golden/netlists/b021-snapfactor.cir`, an `.op`):

    {
      "format": "specs-run-stats",
      "format_version": 1,
      "analysis": "OP",
      "sections": ["summary"],
      "options": {"abstol": 0.01, "reltol": 1e-04, "snapfactor": 4, "coalesce": true, "maxevents": 0},
      "event_cap_reached": false,
      "end_time_s": 0,
      "wall_time_s": 0.000218417,
      "totals": {"output_ports": 6, "scheduled": 6, "coalesced": 0, "processed": 6, "emitted": 5, "suppressed": {"abstol": 0, "reltol": 0, "snap": 1}, "pending_max": 1, "pending_mean": 1, "pending_end": 0, "last_emission_s": 0}
    }

Numbers are written in the shortest form that reads back to the same double. The rows of `ports` are in name
order, those of `devices` in type order; one row per line. A port that carries several wavelengths lists them
all (a `.dc` wavelength sweep lists every swept wavelength).

The golden harness (`tests/golden/run_golden.py`) masks `wall_time_s` in files named `*.stats.json` and on
stdout.

### Progress (`--progress`)

`specs --progress` prints a line on stderr every second of wall-clock time (`--progress=0.2`: every 0.2 s):

    Progress: 12.0 s wall, simulated 1234500 ps (41.2 % of 3 us), 8812431 delta cycles

The share of the duration is shown for a `.tran` with a duration. The line is printed from a SystemC stage
callback after a delta cycle; it only reads the simulation time and the delta count, so it never changes the
results, advances time or stops the run. A long `.op` iteration shows the delta cycles going up at a fixed
simulated time.
