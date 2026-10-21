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
| `abstol` | 1e-8 | absolute emission threshold, optical **field**, √W | > 0 | `--abstol` |
| `reltol` | 1e-4 | relative emission threshold | > 0 | `--reltol` |
| `snapfactor` | 10 | emitted fields below snapfactor·abstol become 0 | >= 0 (0: off) | none |
| `timescale` (alias `ts`) | -13 | log10 of the engine time step in s: -13 is 100 fs | integer, -15 (1 fs) to 0 (1 s) | `-R`, `--resolution` |
| `resolution` | 1 | port time step = `resolution` × engine step | >= 1 | none |
| `seed` | 0 | master seed of the random numbers (see below) | integer, 0 to 2^53 | `--seed` (0 to 2^64 - 1) |

Example: `.options abstol=1e-6 reltol=1e-8 timescale=-12` (as in `circuits/add_drop_pyspecs.cir`).

- An invalid value is an error and the simulation does not start.
- With `-f`, the CLI values are appended to the netlist as `.options` lines, so they override the netlist.
  They are ignored by the legacy testbenches (`-t`), except `--seed`, which is applied directly: it also
  sets the seed of the testbenches and takes priority over a netlist's `.options seed`.
- `seed`: every random draw (today the photodetector noise and dark field, see `doc/netlist.md`) comes
  from a stream derived from the master seed and the hierarchical name of the device, so a run is
  reproducible: the default seed is 0, and the same netlist and seed give the same values. Netlist numbers
  are doubles, hence the 2^53 limit of `.options seed`; `--seed` takes any decimal unsigned 64-bit integer.
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

A non-zero delay that rounds to 0 gives a second form (this case may become an error):

    Warning (ROOT/WGR): delay 1.33426e-14 s rounds to 0 at the time resolution 1e-13 s (the delayed events happen in the same time step); use a finer time resolution, e.g. .options timescale=-15

"Time resolution" in the message is the port time step. The simulation runs anyway.

- Checked once per waveguide, only where delays matter: `.tran` and event-driven testbenches. `.op`, `.dc`
  and `-m fd` ignore delays, so nothing is reported there. Only waveguides are checked; couplers have no delay.
- A delay of at least 5 port time steps never warns (rounding is at most 0.5 step, i.e. 10 %).
- The default 100 µm waveguide at 100 fs (755 fs rounded to 800 fs, +5.95 %) does not warn.
  `circuits/add_drop_pyspecs.cir` does (2.27 ps ring arms at 1 ps: 2 ps, -11.7 %): use `timescale=-13` or finer.
- Fix: a finer `timescale` (the message suggests the coarsest one that passes), or a smaller `resolution`.
  A coarser step is cheaper but less precise, as the paper says.

In the paper's CROW benchmark (N = 3, L ≈ 350 µm), Interconnect differed from Photontorch and SPECS at 100 fs.
At 1 fs all three agreed, with pairwise RMS errors below about 3 µW.
