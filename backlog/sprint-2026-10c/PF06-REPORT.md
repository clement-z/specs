# PF06 report: accuracy suite (F-06) and static activity map (F-31), branch `ws/accuracy`

Base: `sprint/2026-10c` at cb3a5c3. Decisions: "Decisions of 2026-10-23, third round", rows "Accuracy suite",
"Activity map" and "Follow-ups". **No product code changed** (`git diff cb3a5c3 -- src tests/golden` is empty):
outputs and counters are bit-identical by construction, and the gates below confirm it.

## Items

| # | Item | Outcome | Commit(s) |
|---|---|---|---|
| 0 | Graphviz in pixi's `py` environment | done: `graphviz` added to `[feature.py.dependencies]`; `pixi lock` added packages to the `py` environment only (no line removed, default environment unchanged); `pyspecs/requirements.txt` comment | af2324b |
| 1 | CROW generator moved to `tests/bench/` | done: `crow_gen.py` writes the same netlists as the scout version for the same options (bodies diffed on 4 option sets, identical); adds `--source cw`, `--analysis dc`, `--wl`, `--width/--period`, `--preset crow_tb`, module API | 095d46c |
| 2 | Analytic oracles as tests | done: `oracles.py` (add-drop closed form, Yariv/Bogaerts; CROW transfer matrices, Poon; unequal ring halves) and `test_oracles.py`, Meson suite `oracle` (0.8 s release, 28 s clang ASan) | 7d31851 |
| 3 | Benchmark: ratios in CI, absolute times local | done: `bench.py` (scout matrix, per-child rusage via `os.wait4`, `--stats` counters, `--repeat`, history in git-ignored `tests/bench/history.jsonl`); `test_bench.py`, Meson suite `bench` (3 s release) | 480fbc9 |
| 4 | Activity map `utils/activity_map.py` | done (Sonnet subagent, diff reviewed, renders looked at): DOT (stdlib), SVG/PNG (numpy, matplotlib, `dot`), clusters, linear colour, background field, `--suppression` discs, legend and totals; test `tests/tools/test_activity_map.py` (suite `tools`); `doc/activity_map.md`, link in `doc/mainpage.md` | dc2933c, d032b2a |
| 5 | Docs | `tests/bench/README.md`, README section, CHANGELOG `[unreleased]` "New", B-071 status and `backlog.md` row, `discover/scout/bench/README.md` pointer | eab8c08 |

### Oracle results (gcc release, 2026-10-23)
| Check | Max \|H_specs - H_model\| | Tolerance | Smallest mutant miss |
|---|---|---|---|
| add-drop (`circuits/add_drop.cir`, `.dc`, 31 points, complex through/in and drop/in) | 7.2e-7 | 1e-5 | 0.26 (k read as power) |
| crow-dc (3 random-length rings, seed 1, 1545-1555 nm, 101 points) | 4.5e-7 | 1e-5 | 1.87 |
| crow-tran (5 random-length rings, seed 2, CW 1552 nm, `.tran 2 ns`, end of run) | 4.3e-9 | 1e-5 | 0.15 (k read as power) |
| crow-tb (`-t ac_crow`, N = 3 and `--nrings_crow 2`, vs `--preset crow_tb`) | same event times; dP <= 6.8e-13 W | 1e-9 W | (consistency check, no mutant) |

The closed-form add-drop equals the N = 1 transfer matrix to 1e-12 at every point (checked in the test).
**Injected C++ bugs** (scratch worktree, not committed): (a) `m_dneff_dlambda = 0` in `waveguide.cpp`: add-drop,
crow-dc and crow-tran fail (errors 1.9, 1.9, 0.62); (b) the parser reading `k` as a power ratio and `kp` as a
field coefficient: every check fails (crow-dc's `.dc` no longer converges; crow-tb's event times differ).

### Benchmark ratios
| Check | gcc release | clang ASan |
|---|---|---|
| plateau-events: processed N=300 / N=30 <= 1.05 | 1.0000 (2455447 both) | 1.0000 (same counts) |
| wdm-events: 4 channels / (4 x 1), in [0.9, 1.25] | 1.0161 | 1.0161 |
| plateau-time: user N=300 / N=30 <= 2.0, median of 3 | 1.21-1.22 | skipped (`--no-timing`) |

### Does the generator cover `crow_tb`?
**Yes**: `crow_gen.py N --preset crow_tb` reproduces `specs -t ac_crow --nrings_crow N` (same event times on
through and drop, powers within 7e-13 W, N = 3 and 2), checked on every `meson test` with `old_tb`. Per the
decision, `src/tb/crow_tb.cpp` and its golden cases stay; deleting them is now possible and is the maintainer's
call.

## Decisions taken (not covered by the decision file)
- Place: `tests/bench/` (generator, oracles, bench) and `tests/tools/` (tests of `utils/` tools); both registered
  unconditionally in `meson.build` (no new Meson option). The scout's `RESULTS.md`/`results.json` stay in
  `discover/scout/bench/` as the scout's record; the README there points to the new place.
- Timing ratio only on optimised builds without sanitizers (Meson passes `--no-timing` for `plain`/`debug`
  buildtypes or `b_sanitize != none`); the event-count ratios run everywhere (deterministic).
  `bench-ratios` is `is_parallel: false`. Bounds are generous (2.0 for a measured 1.2) because CI runners are
  shared; not yet observed on GitHub runners (nothing pushed).
- Oracle tolerance 1e-5 on complex transfer ratios (10 x the add-drop netlist's `reltol`); generated circuits
  use `abstol 1e-12`, `reltol 1e-8`. Comparison of complex fields (amplitude and phase), from the CSV table.
- Mutants: k as a power ratio, no dispersion slope, real cross coupling (no pi/2). The cross-phase *sign*
  (+j vs -j) is not a mutant: it only multiplies drop/in by (-1)^couplers, invisible in these circuits
  (documented in `oracles.py`).
- Prerequisites of the brief: `--nrings_crow` was already fixed (B-131, 449d63c). Identical ring lengths in the
  `CROW` class are **kept**: random lengths there would change `tb/ac_crow`'s outputs; the paper's random
  lengths come from the netlist generator instead (B-071 updated, still `needs-human` for the deletion and
  publication questions).
- Activity map rendering: Graphviz does the layout (`dot -Tjson`: node positions, edge splines, cluster boxes);
  matplotlib draws it, because the interpolated background field needs a raster under the graph and the same
  drawing must give SVG and PNG. The `.dot` (coloured nodes, clusters) is always written, so a plain Graphviz
  rendering is available (`dot -Tsvg OUT.dot`), also without numpy/matplotlib (`--dot-only`, exit 3 when a
  render dependency is missing). Field: f(x) = sum a_i K_i / (sum K_i + K0), Gaussian K_i with sigma = median
  nearest-neighbour distance between devices, K0 = e^-2 (zero far away, interpolates between devices).
  Colours: white-to-blue single-hue ramp (activity, linear 0..max), white-to-orange (suppression, fixed 0..1).
- Activity of a device = sum of `processed` of the output ports it writes (probes 0). Edge direction from the
  stats writers (bidirectional nets: double arrow; unknown writer: plain line); nets with one device end in
  a terminal label; nets with more than two devices go through a junction point.
- History file: git-ignored (`/tests/bench/history.jsonl`); it stores CPU model and count, not the host name.

## Found, not done
- Per-event cost grows with the number of WDM channels: about 139 ns per processed event with 1 channel,
  246 ns with 4, 329 ns with 16 (N = 3-10, no trace, gcc release, this host). Worth a look in the engine
  (per-wavelength containers in the output port?); not studied.
- `specs --json`: device types read "DIRECTIONAL COUPLER (bidir)" / "WAVEGUIDE (bidir)" also for devices on
  unidirectional nets, and `"subcircuits": "TODO"`; the map derives the instance tree from the names instead.
- Activity map: long chains give very wide figures (a 4-ring CROW is about 6300 px wide at 150 dpi;
  `rankdir=LR`); the suppression disc can overlap part of a narrow node's name (PROBE). Fonts: "DejaVu Sans" is
  requested (matplotlib falls back with a warning where it is missing, e.g. some macOS setups).
- `tests/golden/netlists/f15-wdm-crow-mlprobe.cir` names the old generator path in a comment; left unchanged
  (the netlist's hash is in the case's run manifest).
- Not done from the brief (outside this phase's prompt): all-pass critical-coupling and MZI oracles,
  the 100 fs vs 1 fs convergence test, the Photontorch cross-tool script, a nightly CI job.

## Final results (tip of `ws/accuracy`)
- `pixi run test gcc`: 15/15 OK (unit, 9 tb, golden-harness, golden, oracles, bench-ratios, activity-map).
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: 245 passed, 0 failed, 0 new, 0 missing (no
  re-record).
- `pixi run test asan-clang`: 15/15 OK, no AddressSanitizer/UBSan/LeakSanitizer message; `lsan.supp` unchanged.
- Activity-map test with the full render: `pixi run -e py python -I tests/tools/test_activity_map.py --specs
  build-pixi/gcc/specs --repo .` ok; with the default pixi python (no numpy) and the system python: ok, render
  part skipped.
- Before the phase (cb3a5c3): gcc 12/12, golden 245/245 strict, asan-clang 12/12.
