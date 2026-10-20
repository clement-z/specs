# Feasibility experiments for the feature scout (sprint 2026-10)

Small, stdlib-only scripts (Python >= 3.9, `numpy` not required) used to ground the feature proposals in
`../FEATURES.md` with measurements on the current binary. They do not modify product code and are not wired
into the build; `tests/golden/` is the regression harness.

| Script | Purpose | Run |
|---|---|---|
| `crow_gen.py` | Generates the paper's CROW benchmark (ECOC 2023 Fig. 3): N rings, N+1 couplers `kp=0.15`, 2N waveguides with seeded `N(33.2 um, 5 um)` lengths, 10 x 1 ns pulses at 500 MHz, optional WDM channels. | `python3 -I crow_gen.py 30 --seed 1 > crow30.cir` |
| `bench_crow.py` | Runs a small matrix of generated CROWs (scaling in N, auto-probe tracing on/off, no tracing, WDM channel count, 1 fs resolution, tighter tolerances) and records wall/user time, VCD size and `Simulated` time. | `python3 -I bench_crow.py --specs build/specs --out results.json` |
| `add_drop_oracle.py` | Runs `circuits/add_drop.cir` (`.dc` wavelength sweep) and compares `PROBE{ROOT/out}` / `PROBE{ROOT/drop}` against the closed-form add-drop ring response (Yariv 2002) using the VCD parser from `tests/golden/run_golden.py`. | `python3 -I add_drop_oracle.py --specs build/specs --repo .` |

Results of the runs made on 2026-10-20 (Release build of a9e5799, i5-6600K, machine shared with two other
sprint workstreams, so wall times are noisy and **user time** is the figure to read) are in `RESULTS.md`.

Prefix for every generated/compared netlist is the repository root: `.include` paths and `traces/` are
resolved relative to the working directory, so the scripts always run `specs` in a fresh temporary directory.
