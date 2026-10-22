# F-06: Benchmark and accuracy suite (paper reproduction, analytic oracles, nightly timing)

value 4 | effort S-M | risk low | prerequisites: none (prototype in `discover/scout/bench/`) | horizon 1

## Problem / user story
*As the maintainer I want the ECOC claims to be reproducible from the repository and physics regressions to fail
a test.* Today the CROW testbench hard-codes 3 rings (`src/tb/crow_tb.cpp:33`), `--nrings_crow` is a no-op
(`src/main.cpp:244-247, 341-345`), ring lengths are identical (`src/devices/crow.h`) instead of the paper's
`N(33.2, 5) um`, no test compares a result with a closed form (the only gtest checks waveguide phase with
`neff = ng`), and the golden harness compares the simulator against itself. The thesis found INTERCONNECT
"blatantly wrong" at some time steps and the paper's own numbers were obtained at an unknown build setting
(discovery report 05 §4 measured 3-8x slower at -O0).

## Prior art
INTERCONNECT's ONA offers "scattering data" vs "impulse response" analyses and recommends comparing both;
closed forms: Yariv 2000/2002 (universal coupling relations), Bogaerts et al. 2012 (all-pass and add-drop
`T(r, a, phi)`), Little et al. 1997 (multi-ring continued fraction), Poon et al. 2004 (CROW transfer
matrices, group delay); asv / pytest-benchmark / LINCC nightly benchmarking (SOURCES C, D).

## What already works (this sprint)
`discover/scout/bench/add_drop_oracle.py`: SPECS's `.dc` of `circuits/add_drop.cir` matches the closed-form
add-drop response to 1.5e-6 relative on drop and 7e-6 on through (3e-4 at the 5 uW null), bounded by the
netlist's `reltol=1e-6`. `bench_crow.py` reproduces the plateau (user time flat from 30 to 300 rings) and
quantifies tracing, WDM, resolution and tolerance costs (`bench/RESULTS.md`).

## Proposed design
1. **`bench/` in the tree** (move the three scripts; add a `README`): generators (CROW, add-drop, a
   PUF-like 3-ring chain built from `circuits/add_drop_sub.cir`), `bench.py` recording user time, peak RSS,
   VCD bytes and, once F-15 exists, event counts; results appended to `bench/history.jsonl`; assertions
   that are machine-independent: plateau ratio `t(300)/t(30) < 1.5`, tracing overhead ratio, WDM linearity.
2. **`tests/oracles/`**: deterministic tests comparing `.dc` sweeps with closed forms at tolerance
   `10 x reltol`: add-drop (done), all-pass ring (critical coupling `r = a`), CROW `N = 3` transfer matrix
   (Poon), MZI with `Delta L` (FSR), directional-coupler unitarity; plus a convergence test: CROW `N = 3`
   at 100 fs vs 1 fs RMSE below 3 uW (the paper's cross-tool agreement level). Registered as a Meson test
   suite (`oracle`) running in < 30 s.
3. **Cross-tool check (manual)**: `bench/photontorch_crow.py` building the same CROW in Photontorch (AGPL, pip)
   to regenerate Fig. 3b; not in CI.
4. **CI**: nightly job running `bench.py` on the three generators and posting deltas; the oracle suite on
   every push.
5. Retire `--nrings_crow` and parametrise or delete `crow_tb` once the generator covers it.

## Touch points
`discover/scout/bench/*` to `bench/` · `tests/golden/run_golden.py:259-357` (VCD parser reused as a module) ·
new `tests/oracles/` (pytest or gtest; pytest is simpler since numpy is available) · Meson `test()` entries
(coordinate with ws-meson) · `src/tb/crow_tb.cpp`, `src/main.cpp:244-247, 341-345`.

## Validation plan
The suite is its own validation: it must fail when a physics perturbation is injected (e.g. treating `k` as a
power coupling, or dropping the dispersion slope) and pass on gcc, clang and Debug builds.

## Effort breakdown
Move + README 1 d · all-pass/CROW/MZI oracles 2 d · convergence test 0.5 d · CI nightly 1 d ·
Photontorch script 1 d (optional).

## Open questions for the maintainer
- Publish the regenerated numbers (plateau at ~86 ms normalised on an i5-6600K, VCD on) alongside the paper?
- Absolute time budgets per machine class, or ratio assertions only (proposed: ratios in CI, absolutes in
  `history.jsonl`)?
- Keep `src/tb/crow_tb.cpp` as a golden case or delete it in favour of the generator?
