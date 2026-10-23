# Feasibility experiments for the feature scout (sprint 2026-10)

The scripts of this directory moved to **`tests/bench/`** (F-06, sprint 2026-10c phase PF06), where they are
maintained and tested:

| Scout script (2026-10-20) | Now |
|---|---|
| `crow_gen.py` | `tests/bench/crow_gen.py` (same netlists for the same options, plus CW/`.dc` forms and the `crow_tb` preset) |
| `bench_crow.py` | `tests/bench/bench.py` (same case matrix; per-run counters, local history file) |
| `add_drop_oracle.py` | `tests/bench/oracles.py` and `tests/bench/test_oracles.py` (Meson suite `oracle`) |

`RESULTS.md` and `results.json` stay here as the record of the scout's measurements (2026-10-20, before
F-15, CMake build with `-march=native`); they are not maintained.
