# B-055: Detector RNG is seeded from `std::random_device`, twice, with no seed option
severity: med | category: determinism | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: fixed in 083c8be (sprint 2026-10b P4, F-01): --seed, .options seed, per-name mt19937_64 streams seeded once in start_of_simulation()
location: src/devices/detector.cpp:15,132-136, src/devices/detector.h:38-39, src/specs.cpp:359-361
## Problem
`Detector::init()` seeds `m_rngGen` from `std::random_device`. It is called by `prepareSimulation()` (specs.cpp:361) and again at the start of the `on_port_in_changed` thread (detector.cpp:15).
- Any simulation with `nb=0` (noise on) cannot be reproduced.
- There is no CLI or `.options` seed.
- `std::default_random_engine` is implementation-defined, so the sequence is not portable across standard libraries even with a fixed seed.
- Every detector draws from its own independently seeded engine, so a global seed must define per-instance streams.
## Evidence
Code: detector.cpp:134-135 `std::random_device rd; m_rngGen.seed(rd());`; detector.cpp:15 `init(); // rng seed`; specs.cpp:360-361 `for (auto &mod : all_spx_mod) mod->init();`.
## Expected
Reproducible noise given a seed, with a documented default (fixed or time-based).
## Suggested fix
Add a global seed (`.options seed=` / `--seed`). Derive per-detector seeds deterministically (for example the hash of `name()` combined with the global seed). Use `std::mt19937_64`. Seed once, in `init()` only, and remove the call at detector.cpp:15. This is a design decision on the option name and defaults.
## Golden impact
None today, since no golden detector is enabled with noise on.
## Related
B-056, B-054, B-059.

- Discovery seeds: S1.14, S2.17 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-09.
## Verification
Code reading of the cited lines.
