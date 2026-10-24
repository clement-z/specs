# Accuracy suite and CROW benchmark (F-06)

Stdlib-only Python (>= 3.9). The golden harness (`tests/golden/`) compares SPECS with itself; the scripts here
compare it with closed forms and measure its cost. Every script runs `specs` in a fresh temporary directory.

| File | What | Run |
|---|---|---|
| `crow_gen.py` | CROW netlist generator (below) | `python3 -I tests/bench/crow_gen.py 30 --seed 1 > crow30.cir` |
| `oracles.py` | closed-form responses in SPECS's conventions: add-drop ring, CROW transfer matrices | module |
| `test_oracles.py` | SPECS against `oracles.py` (Meson suite `oracle`) | `python3 -I tests/bench/test_oracles.py --specs build/specs --repo . -v` |
| `bench.py` | CROW case matrix: user/wall time, peak RSS, VCD size, `--stats` counters; appends to `history.jsonl` | `python3 -I tests/bench/bench.py --specs build/specs --repeat 3` |
| `test_bench.py` | ratio checks on the CROW benchmark (Meson suite `bench`) | `python3 -I tests/bench/test_bench.py --specs build/specs` |

`meson test -C build --suite oracle --suite bench` runs both suites (about 4 s on a release build).

## CROW generator

`crow_gen.py N` writes the benchmark of the ECOC 2023 paper (Fig. 3): N rings, N+1 couplers with power
cross-coupling `kp = 0.15`, 2N waveguides with lengths drawn from N(33.2 um, 5 um) by a seeded RNG (the two
halves of a ring differ), `neff 2.2`, `ng 4.3`, 2 dB/cm, ten 1 mW pulses of 1 ns at 500 MHz, `.tran 20 ns`,
`abstol 1e-8`, `reltol 1e-4`, 100 fs. Options change each of these (`--help`); `--nwl` adds WDM channels on a
100 GHz grid, `--source cw` a CW source, `--analysis dc` a wavelength sweep of it. `--preset crow_tb` is the
circuit of the former testbench `specs -t ac_crow` (identical rings of 500.5 x 1.55 um, `kp 0.2`, `neff 1`,
`ng 2`, four 1 ns pulses at 1553 nm, `.tran 50 ns`; same event times and powers within 7e-13 W for N = 3 and 2,
checked before the testbench was removed in v0.4.0); the golden cases `regress/crow-tb-preset-n3` and
`regress/crow-tb-preset-n8-vci` run it. The topology is that of the `CROW` class (`src/devices/crow.cpp`).

## Oracles (`test_oracles.py`)

| Check | Circuit | Compared | Tolerance |
|---|---|---|---|
| `add-drop` | `circuits/add_drop.cir`, `.dc`, 31 points | through/in, drop/in (complex) vs Yariv's closed form; the closed form also vs the N = 1 transfer matrix | 1e-5 (10 x the netlist's `reltol`) |
| `crow-dc` | 3 rings, random lengths (seed 1), `.dc` 1545-1555 nm, 101 points | the same vs the transfer matrices (Poon 2004) | 1e-5 |
| `crow-tran` | 5 rings, random lengths (seed 2), CW at 1552 nm, `.tran 2 ns` | the field at the end of the run vs the steady state | 1e-5 |

The models (`oracles.py`) are SPECS's device equations: waveguide `a e^{j 2 pi neff(lambda) L / lambda}` with
`neff(lambda) = neff + (neff - ng) (lambda - 1.55 um) / 1.55 um`, coupler through `t` and cross `j kappa`.
Every oracle check also compares SPECS with three mutated models (k read as a power ratio, no dispersion
slope, a cross coupling without its pi/2 phase) and fails if one of them comes within 10x the tolerance, so a
regression of that kind in the code is known to be caught. Measured on 2026-10-23: errors 7.2e-7 (add-drop),
4.5e-7 (crow-dc), 4.3e-9 (crow-tran); mutants off by 0.15 to 2.3; a build with the dispersion slope removed
from `waveguide.cpp`, and one reading `k` as a power ratio in the parser, both fail the suite.

## Benchmark and ratios

`bench.py` runs the case matrix of the feature scout (`discover/scout/bench/RESULTS.md`, 2026-10-20):
A scaling in N with two probes, B every net probed, C no probe, D WDM channels, E 1 fs resolution, F tighter
tolerances. Each run's own rusage is read (`os.wait4`). The results are printed as a table and appended as
one JSON line (date, `specs --version`, CPU model and count, cases) to `tests/bench/history.jsonl`, which is
not versioned: absolute times depend on the machine and its load, so they are recorded locally and not
asserted (a fixed benchmark machine is a later step). Compare user times, medians of several runs
(`--repeat`), on the same host and session.

`test_bench.py` asserts only machine-independent ratios, on the paper's CROW with 2 pulses and no trace file:

| Check | Assertion | Measured (2026-10-23) |
|---|---|---|
| `plateau-events` | processed events N = 300 / N = 30 <= 1.05 | 1.0000 (beyond about 30 rings the light is below the tolerances) |
| `wdm-events` | processed events 4 channels / (4 x 1 channel), N = 3, in [0.9, 1.25] | 1.016 |
| `plateau-time` | user time N = 300 / N = 30 <= 2.0, median of 3 | 1.2 (skipped on plain/debug and sanitizer builds) |
