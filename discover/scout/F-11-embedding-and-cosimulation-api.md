# F-11: Embedding and co-simulation API (library with parser, `Circuit` object, processor + photonics example)

value 4 | effort M-L | risk med | prerequisites: F-27 (diagnostics without `exit`) | horizon 3

## Problem / user story
*As an architect I want to instantiate a photonic accelerator from a netlist inside my own SystemC/TLM model of a
processor.* This is SPECS's unique asset: Cardoso already wrapped the COMET RISC-V core as a SystemC module
next to the photonic GEMM, NEUROPULS evaluates on gem5 (which has a SystemC/TLM bridge), and the paper's closing
vision is "co-simulation of computing cores and photonic accelerators" (SOURCES B). Yet `libspx` excludes the
parser and `main` (`CMakeLists.txt:20-26, 50`), `build_circuit` lives in `src/main.cpp:143-185`, all state is the
static global `specsGlobalConfig` (`src/specs.cpp:19`), sources and sinks are found by `dynamic_cast` on
concrete types inside the analysis drivers (`src/specs.cpp:62-241`), the scanner keeps global state
(`src/parser/parser.l`, `netlist_content_fifo`, `cur_pt`), and every error is `cerr` + `exit(1)`.

## Prior art
SystemC TLM-2.0 sockets; gem5's SystemC coupling; Simics running SystemC subsystems; PySysC; Verilog-A
co-simulation as the community's alternative (Morison/Shastri 2024, Azhigulov 2024); SystemC-AMS converter
ports (SOURCES B, C, D).

## Proposed design
1. **Library completeness.** Build the parser into `spx` (needs the reentrant-scanner globals moved into
   `ParserState`, `src/parser/parser_state.h`) and move `build_circuit` into `src/api/circuit.{h,cpp}`.
2. **`spx::Circuit`.** Owns a `ParseTree`, the modules and signals (today pushed into
   `specsGlobalConfig.additional_objects`, `src/parser/parse_tree.cpp:812-816`); constructed from a file or a
   string plus options; exposes `optical(name) -> sc_signal<OpticalSignal>&`, `electrical(name) ->
   sc_signal<double>&`, `device<T>(name)`, `probes()`; `Simulation` options object replacing the globals with
   `specsGlobalConfig` kept as a thin shim so the CLI and testbenches are unchanged.
3. **Analysis hooks.** `prepare()`, `run_op()`, `run_tran(duration)` callable from a user `sc_main`; the user's
   own processes drive sources (write to an optical signal) and read detectors. The source/sink enable logic
   becomes an interface (`spx::Controllable` with `enable()/reset()`) instead of per-type casts.
4. **Bridges.** `tlm_optical_target` (TLM-2.0 target that converts write transactions into value-list-style
   optical events) and `detector_tlm_initiator`/`sc_in<double>` adaptors; an `examples/embed/` with a toy
   processor model issuing matrix-vector products to an OCTANE-like crossbar and reading photocurrents.
5. **Errors.** Depends on F-27: library users must get exceptions, not process exit.

## Touch points
`CMakeLists.txt:20-26` / Meson lib target · `src/main.cpp:143-185` (move) · `src/specs.{h,cpp}` (globals,
analysis drivers `:62-241`) · `src/parser/parser.l` (globals), `parser_state.h` · `src/parser/parse_tree.cpp:688-818`
(`build_circuit`, `:812-816` ownership) · new `src/api/`, `examples/embed/` · `src/test/devices/waveguide.cpp:136-170`
(tests move to the API).

## Validation plan
- `examples/embed` builds and runs in CI; running `circuits/add_drop.cir` through the API yields the same VCD
  as the CLI (golden parity).
- Unit tests use `Circuit` instead of poking globals; a TLM test writes 100 transactions and checks the
  photocurrent sequence.
- Two `Circuit` objects in one process (different name prefixes) elaborate together.

## Effort breakdown
Parser into the library + scanner state 3 d · `Circuit` and options 5 d · controllable interface 2 d ·
TLM bridge 5 d · example + docs 3 d. Total about 3.5 weeks.

## Open questions for the maintainer
- API stability promise and naming (`spx::` already exists for types).
- Keep the global config for the CLI (proposed shim) or remove it in one step?
- Which processor model to ship in the example: a pure-SystemC toy, or a documented recipe for gem5's TLM bridge?
