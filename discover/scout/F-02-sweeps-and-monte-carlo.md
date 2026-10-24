# F-02: Parameter sweeps and Monte-Carlo (`--param`, `.step`, `.mc`, process-parallel runner)

value 5 | effort M | risk low-med | prerequisites: F-01 (seeds), F-05 (table outputs) | horizon 2

> **Status 2026-10-23: implemented (sprint 2026-10c, phase PF02, branch `ws/sweeps`, report
> `backlog/sprint-2026-10c/PF02-REPORT.md`).** Functions in expressions (math, `gauss`/`uniform`/`lognormal`/
> `choice`, per-instance draws, `shared=`), `.step param` (linear with the stop included, `list`, nested),
> `.mc N [seed=]`, one process per run (`-j N`), one run table `<output>.runs/` (appendable `.npy` columns +
> `runs.csv`, failures recorded, resume), `--run K`, `pyspecs/runtable.py`. Decisions 2026-10-23 (third round)
> answered the open questions: SPICE syntax with `param`, stop included, independent draws per instance with
> `shared=`, one table. Not done: `pyspecs.sweep()`/`mc()` runners (F-07), in-process `reconfigure()`.

## Problem / user story
*As a PUF researcher I simulate 20 device instances x 200 challenges; as an OCTANE researcher I run 1e5 random
matrix multiplications per tile size; as a filter designer I sweep a coupling ratio.* Today:
- `.dc` sweeps only a `CWSource`'s wavelength, power or frequency (`src/parser/parse_analysis.cpp:36-73` binds
  `CWSource::setWavelength/setPower/setFrequency`, `src/devices/cw_source.h:30-56`), one sweep per netlist
  (`src/parser/parser.y:405-409`), endpoint excluded (`src/utils/general_utils.h:30-33`,
  `src/parser/parse_analysis.h:59-66`), only the first analysis runs (`src/parser/parse_tree.cpp:808-809`).
- Device parameters are set by poking members after construction (`src/parser/parse_element.cpp:94-124`) but
  every device computes its S-parameters once in its thread prologue (`src/devices/waveguide.cpp:6-30`), so an
  in-process sweep of a device parameter has no effect without a recompute hook.
- There are no random distributions in expressions (`src/parser/parser.y` expression grammar, `Variable` in
  `src/parser/parse_tree.h`), no CLI parameter override, and the "paralel run" idea in
  `pyspecs/specs.py:8-15` was never implemented.
- Users do this elsewhere: INTERCONNECT Monte-Carlo/corner utilities, Luceda Circuit Analyzer (four MC
  variants), PhotonForge `RandomVariable`, SAX `jax.vmap`; Columbia wrapped OptSim in polynomial chaos because
  brute-force MC was too slow (SOURCES A, B, D).

## Prior art
SPICE `.step`/`.param` with `gauss()`/`agauss()` (HSPICE, LTspice, ngspice); Xyce `.SAMPLING`; INTERCONNECT
statistical compact models (`.lib` with Gaussian/Pearson IV); Simphony layout-aware MC (`sigma_w, sigma_h,
correlation_length`); the SPECS gtest harness already forks one process per simulation
(`src/test/utils.h:28-76`) because a SystemC kernel elaborates once per process (Accellera forum, SOURCES C).

## Proposed design
Two layers; the second reuses the first.
1. **Per-run parameterisation.**
   - `--param NAME=VALUE` (repeatable). Because `{name}` is evaluated at parse time, the override must be
     injected as a *header* `.param` line (the scanner already queues `[header, file, footer]` per file,
     `src/parser/parser.l`, `yy_add_content_from_file`) and marked *locked* so later `.assign/.param` lines in
     the netlist are ignored with a warning (`ParseTree::set_local_variable`, `src/parser/parse_tree.h`).
   - Expression functions: `gauss(mu, sigma)`, `uniform(a, b)`, `lognormal`, `choice([..])`, `sqrt/exp/log/...`
     in the `pow`/`constant` rules (`src/parser/parser.y`, expression section). Random draws use the run's
     master seed (F-01) and are evaluated where they appear, so a `.param L=gauss(33.2e-6,5e-6)` inside a
     `.subckt` body is re-drawn per instance at flatten time (`src/parser/parse_tree.cpp`, `flatten()`), which
     is what a CROW or PUF needs.
2. **Plan directives and runner.**
   - `.step PARAM start stop step` / `.step PARAM list [v1,v2,...]` (nestable) and `.mc N [seed=S]` are
     parsed into a `RunPlan` (new `src/parser/parse_plan.{h,cpp}`, token rules next to
     `src/parser/parser.l:460-470`).
   - The CLI expands the plan to a list of points and **forks one child per point before elaboration**
     (the global `specsGlobalConfig` exists but no `sc_start` has run, which is exactly the state in which the
     gtest harness forks). Each child gets the point's `--param` overrides and seed, writes
     `<out>_p0007.{vcd,csv}`; the parent runs `-j N` children and writes `<out>.index.csv`
     (point id, parameter values, seed, exit code, wall time). Table outputs (F-05) make aggregation trivial.
   - `pyspecs.sweep()` / `pyspecs.mc()` (F-07) call the same CLI, or drive a `multiprocessing` pool with the
     `spawn` context and `maxtasksperchild=1`, and return a long DataFrame with parameter columns.
   - `.dc` stays as the fast in-process path for CW parameters; an in-process generalisation (a
     `reconfigure()` hook on `spx_module`) is deferred because it requires resetting device state and is a
     numerics decision.

## Touch points
`src/main.cpp:194-285` (flags), `src/main.cpp:143-185` (header injection next to the footer) ·
`src/parser/parser.l:460-470` (`.STEP`, `.MC`), `src/parser/parser.y` (directive rules, expression functions) ·
`src/parser/parse_tree.h` (locked assignments, `global_assignments`), `src/parser/parse_tree.cpp` (flatten
re-evaluation) · new `src/parser/parse_plan.{h,cpp}`, `src/run_plan.{h,cpp}` (fork loop; pattern from
`src/test/utils.h:28-76`) · `src/specs.cpp:110-188` (unchanged `.dc`) · `pyspecs/specs.py:126-236`.

## Validation plan
- `.step k list [0.1,0.2,0.3]` produces three outputs byte-identical to three `--param k=...` runs.
- `.mc 8 seed=1` is reproducible; `-j 4` equals `-j 1`; the bench CROW generator's random lengths move into
  `.param L=gauss()` and reproduce `bench/RESULTS.md` numbers.
- Golden: new cases for `--param` and a 3-point `.step`; existing cases unchanged.
- Timing: 16 MC points on 4 cores finish in about 4.5x one point.

## Effort breakdown
`--param` + locking 2 d · expression functions + per-instance draws 2 d · plan parser 1 d · fork runner +
index file 3 d · pyspecs runner 2 d · tests/docs 2 d. Total about 12 days.

## Open questions for the maintainer
- Syntax: SPICE-like `.step param X 1 10 1` or the shorter `.step X 1 10 1`? Include the endpoint (SPICE does;
  today's `.dc` does not; changing `.dc` is a results change, so `needs-human`).
- Should every `.subckt` instance draw independently by default (proposed yes) with a `shared=` escape hatch?
- Output policy for 1e5 runs: per-point files versus one appended table with a point column (proposed: table
  per point, index file, pyspecs concatenates).
