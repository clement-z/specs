# Onboarding guide: SPECS

Generated 2026-10-20 with the `codebase-onboarding` skill, adapted to a C++/SystemC simulator,
from five parallel discovery probes. Detailed reports:
[01 engine](01-architecture-engine.md) · [02 devices/physics](02-devices-physics.md) ·
[03 parser/netlists/pyspecs](03-parser-netlist-pyspecs.md) · [04 build/test/CI](04-build-test-ci.md) ·
[05 paper/history/conventions](05-paper-history-conventions.md) · paper text: [paper-zrounba2023-specs-ecoc.txt](paper-zrounba2023-specs-ecoc.txt)

## Overview
SPECS (Scalable Photonic Event-driven Circuit Simulator, INL Lyon, ECOC 2023) simulates photonic integrated
circuits as **event-driven** systems on SystemC. You give it a SPICE-like netlist (`.cir`); it builds a graph of
SystemC device modules exchanging per-wavelength complex optical fields, and runs `.op`, `.dc` sweeps or
`.tran` analyses. Outputs go to stdout tables and a VCD trace. The main claim is that propagating only
*changes* above abs/rel tolerances (1e-8 / 1e-4) scales better than time-stepped or frequency-domain simulators.

## Tech stack
| Layer | Technology | Version / notes |
|---|---|---|
| Language | C++ | C++20 (`gnu++20`) |
| Kernel | SystemC | 3.0.2 (submodule `thirdparty/systemc`, built as C++20) |
| CLI | taywee/args | submodule `thirdparty/args`, header-only |
| Parser | flex + bison | reentrant scanner, caseless; generated into the build dir |
| Build | **Meson** ≥1.3 (+ Ninja) | replaced CMake and the Makefile in sprint 2026-10 (`discover/meson/`) |
| Tests | GoogleTest (WrapDB wrap `thirdparty/gtest.wrap`) with fork-per-test macros; legacy `specs -t <tb>` testbenches with no assertions; **golden-output harness** `tests/golden/` (new) | |
| Python | `pyspecs/` subprocess wrapper + VCD→pandas | not packaged, undeclared deps |
| CI | `.github/workflows/ci.yml` and `.gitlab-ci.yml` (Ubuntu 24.04, Meson) | not yet run on a remote |
| Licence | LGPL-3.0 | no per-file headers |

## Architecture (one screen)
```
specs -f circuit.cir [-o trace] [-T ts] [--abstol/--reltol] [-t tb]
  main.cpp:sc_main ─ args ─► flex/bison (src/parser) ─► ParseTree ─ flatten() ─ build_circuit()
                                                                    │ new Device(...) ; bind sc_signal<OpticalSignal>
  global specsGlobalConfig (SPECSConfig : sc_module, specs.cpp:19) ◄┘ owns all objects, runs analyses
     .op/.dc → every output port NO_DELAY, delta-cycle fixed point   .tran → OP first, then event-driven
  Device SC_THREAD ─ read p_in ─► compute ─► OpticalOutputPort.delayedWrite(sig, delay)
                                           └► PQueue + sc_event_queue ─ abstol∧reltol gate ─► sc_signal write
  Sinks: Probe / MLambdaProbe / Detector / PowerMeter ─► VCD + stdout
```
- WDM is time-multiplexed: a net holds only the **last (λ, field)** written. Stateful devices keep per-λ maps.
- Device pattern: `XxxBase` (parameters) + `XxxUni` / `XxxBi` (ports), one `SC_THREAD` per input.
  Only waveguides have delay.
- Errors: `cerr` + `exit(1)` everywhere, plus `assert` on user input (core dumps on bad netlists).

## Key entry points
- **CLI / sc_main**: `src/main.cpp`
- **Global config & analyses**: `src/specs.{h,cpp}` (`SPECSConfig`, `runOPAnalysis/runDCAnalysis/runTRANAnalysis`)
- **Event scheduling**: `src/optical_output_port.{h,cpp}`. This is the heart of the event-driven model.
- **Signal type**: `src/optical_signal.{h,cpp}`
- **Devices**: `src/devices/` (27 classes; catalogue in report 02)
- **Netlist language**: `src/parser/parser.{l,y}`, `parse_element.cpp` (one `create()` per element type)
- **Legacy testbenches**: `src/tb/*_tb.cpp`, registered in `src/tb/alltestbenches.cpp`
- **Unit tests**: `src/test/` (gtest + `utils.h` fork macros)

## Directory map
| Path | Purpose |
|---|---|
| `src/devices/` | photonic/electrical device models (SystemC modules) |
| `src/parser/` | flex/bison netlist grammar, parse tree, element → module instantiation |
| `src/tb/` | legacy C++ testbenches, run with `specs -t <name>` (no assertions) |
| `src/test/` | gtest unit tests (only the waveguide so far) |
| `src/utils/` | priority queue, SystemC object-tree helpers, string helpers |
| `circuits/` | example netlists. **Only about half parse/run today** (report 03 §4) |
| `pyspecs/`, `utils/` | Python wrapper, VCD analysis, graph drawing, GTKWave launcher, KiCad symbol lib |
| `doc/` | Doxygen config, stub mainpage, cheatsheet (stale), poster, **paper** (`doc/papers/`) |
| `tests/golden/` | golden-output regression harness (sprint 2026-10) |
| `thirdparty/` | submodules: systemc, args |

## Simulation lifecycle
CLI parse → `.options` footer synthesised from flags → `yyparse` → `ParseTree::flatten` (re-parses subckt
bodies) → `build_circuit` (bidirectional elements first, then unidirectional; directives in order; **only the first
analysis runs**) → `runAnalysis` → `applyEngineResolution` → `prepareSimulation` (`init()` on every module,
VCD open, default port configs, auto-probes) → `sc_start` one or more times → stdout + VCD.
Full trace with file:line in report 01 §3.

## Conventions
- `#pragma once`; project includes are quoted and relative to `src/` (`-iquote src`); `<systemc.h>`.
- PascalCase classes; `m_` members **with units in the name** (`m_length_cm`); ports `p_in`/`p0_out`;
  threads `on_<port>_changed()`; method case varies, so **follow the surrounding class**; `typedef`, not `using`.
- Units: SI in netlists, **cm internally for waveguides**, dB/cm; field is √W (labelled V/m).
- **Never run `ninja clang-format` / bulk clang-format**: 104/108 files are non-compliant. Match local style.
- Git: work on `dev`, `--no-ff` merges to `master`; lowercase imperative commit subjects; one tag `v0.1`.

## Common tasks (Meson; decisions in `discover/meson/DECISIONS.md`)
- Deps: SystemC + args installed in `~/.local/opt/specs-deps` (shared by all sprint worktrees). Elsewhere
  `./install.sh` builds SystemC from the submodule, or Meson falls back to building it as a CMake subproject.
- Build: `meson setup build -Dcmake_prefix_path=$HOME/.local/opt/specs-deps && meson compile -C build`
  (release by default; options in `meson.options`: `native`, `old_tb`, `spx`, `tests`, `golden`, `docs`)
- Tests: `meson test -C build` (suites `unit`, `tb`, `golden`; `--suite golden` for the gate only)
- Run a netlist: `build/specs -f circuits/add_drop.cir -o /tmp/trace`
- Legacy testbench: `build/specs -t wg`
- Golden regression: `python3 -I tests/golden/run_golden.py check --specs build/specs --unit-tests build/src/test/unit_tests --baseline tests/golden/baseline`
- Docs: `ninja -C build doc` → `build/doc/html`; coverage: `-Db_coverage=true`, `meson test`, `ninja -C build coverage-html`

## Where to look
| I want to… | Look at… |
|---|---|
| add a device | `src/devices/` (copy a `Base`+`Uni`/`Bi` trio), `devices/alldevices.h` |
| expose it in netlists | `parser.l` prefix → `parser.y` token/rule → `parse_element.{h,cpp}` `XxxElement::create` |
| change event/tolerance logic | `src/optical_output_port.cpp` |
| add an analysis | `src/parser/parse_analysis.cpp`, `src/specs.cpp` |
| add a unit test | `src/test/devices/<dev>.cpp` with `SPECS_TEST_HEADER/FOOTER` |
| check physics against the paper | report 05 §3 (traceability table) |

## Known sharp edges (highlights; the full list will be in `backlog.md`)
- Bidirectional active MZI is miswired; OCTANE/PCM defaults make cells opaque; detector electrical output is never driven.
- `-m sampled-time` silently drops all outputs; `--mon` exits after 0.5 s wall time.
- Subcircuits: nets shared across instances break, top-level `.param`/`.subckt` aren't visible inside subcircuits, `.include` inside `.subckt` corrupts the body.
- Positional args of CWSRC/VLSRC/PCMCELL are shifted by one.
- Coupling `k` is a field coefficient in code but a power ratio in the paper.
- Nondeterminism: pointer-ordered `std::set` traversal; detector RNG seeded from `random_device`.
