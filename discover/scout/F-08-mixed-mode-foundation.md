# F-08: Mixed-mode foundation: sampled-time islands and quantized-state device dynamics

value 5 (strategic enabler) | effort L | risk high (numerics; needs-human sign-off of a written spec) |
prerequisites: F-01, F-06 | horizon 3

## Problem / user story
*As a modeller I need devices with internal dynamics (heaters, carriers, PCM temperature, lasers) and circuits
with dense feedback to simulate at a predictable cost.* The paper names mixed-mode as future work #1; issues #9
(sampled-time) and #12 (convergence) and the thesis conclusion say the same. Today `SAMPLED_TIME` is an empty
stub (`src/optical_output_port.cpp:360-369`) that is still selectable (`-m sampled-time`,
`src/main.cpp:297-301`) and silently drops every event; `spx_module::ModuleFlags {NON_LINEAR, TIME_VARIANT,
FREQUENCY_DEPENDENT}` is declared and unused (`src/devices/spx_module.h:22-28`); the frequency-domain fixed point
has no iteration cap (`src/specs.cpp:104`); and feedback-heavy circuits blow up in events: the thesis PUF
benchmark (5 rings, 25 wavelengths) was only 2.2x faster than INTERCONNECT, and `bench/RESULTS.md` case E shows
1 fs resolution costing 240x over 100 fs because events stop coalescing.

## Prior art
- SystemC-AMS (IEEE 1666.1): TDF clusters with a static schedule and converter ports to the DE kernel; Dynamic
  TDF lets a sampled block re-time itself from events (`request_next_activation`).
- INTERCONNECT sample mode vs block mode; Caphe `calculate_dydt` states with RK4/Euler; Verilog-AMS connect
  modules.
- **Quantized State Systems** (Kofman & Cellier; QSS1-3, LIQSS for stiff systems; Castro et al. 2024 review):
  integrate `x' = f(q)` where `q` is a hysteretic quantisation of `x`; each state computes its own next
  crossing time, so integration becomes asynchronous events with an error bound proportional to the quantum.
  SPECS's output gate (emit only if the change exceeds abstol and reltol) is already an output-side quantiser;
  QSS adds the state side. (SOURCES D §8.)

## Proposed design (three mechanisms, cheapest first)
1. **Quantized device states (no kernel change).** `src/qss.{h,cpp}`: `QuantizedState { x, q, dq_abs, dq_rel,
   hysteresis; advance(now); next_crossing(); }`. Between events SPECS inputs are piecewise constant, so first-
   order linear states have the closed form `x(t) = x_inf + (x0 - x_inf) exp(-t / tau)` and the next crossing of
   `|x - q| = dQ` is analytic; nonlinear states use QSS1/LIQSS1 steps. A device holding such a state schedules
   an `sc_event` at the crossing, recomputes outputs and writes them through the normal `OpticalOutputPort`
   gate. First adopters: heater RC and thermo-optic phase (F-09), photodiode low-pass (F-04), free carriers
   (F-17), PCM cell temperature (F-10), modulator electrical bandwidth (F-18). Quantum defaults derive from
   abstol/reltol; `.options qtol=` overrides.
2. **Sampled-time islands.** Implement `delayedWriteSampledTime` as its comment describes (average the events
   that fall in a port timestep and emit one value per `Ts`), add a `SampledModule` base (`SC_METHOD` on a
   periodic event, zero-order-hold reads of inputs, outputs through ports in `SAMPLED_TIME` mode) and per-element
   or per-subcircuit mode selection (`mode=sampled ts=1e-12` kwargs; `OpticalOutputPortConfig` already exists
   per port, `src/optical_output_port.h:37-45`). Converters: event to sample = hold/average; sample to event =
   the existing tolerance gate. Users: lasers with RIN/phase noise, SOAs, FIR dispersion, noise sources.
3. **Convergence and coalescing (needs-human).** Iteration cap and residual report for the FD fixed point
   (`src/specs.cpp:94-104`, `optical_output_port.cpp:371-405`; issue #12); an optional per-port coalescing
   window (merge events within `w` ticks; the snapping code at `optical_output_port.cpp:301-324` with
   `resolution_multiplier` is already half of it) to trade delay accuracy for event count explicitly.

## Touch points
`src/optical_output_port.{h,cpp}` (modes `:21-33`, stub `:360-369`, snap `:301-324`, FD write `:371-405`) ·
`src/devices/spx_module.h:22-30` (flags, `init`) · `src/specs.cpp:62-108` (OP loop) · new `src/qss.{h,cpp}`,
`src/devices/sampled_module.h` · parser: `.options qtol`, per-element `mode`/`ts` kwargs
(`src/parser/parse_directive.cpp:18-48`, `parse_element.cpp`) · remove `-m sampled-time` until (2) exists.

## Validation plan
- QSS unit tests: first-order step against the analytic exponential (error < quantum); a stiff two-state system
  with LIQSS does not chatter at equilibrium; event count scales with 1/quantum as theory predicts.
- Thermal ring hysteresis sweep (F-09) against Carmon et al. 2004; Caphe's Kerr-ring example as a golden case
  (F-17).
- Sampled island: a CW source with RIN at `Ts = 1 ps` shows the specified PSD; island + event circuit
  round-trip conserves energy.
- FD: a lossless loop now ends with a clear non-convergence error instead of hanging.
- Bench case E (1 fs) with a coalescing window returns to the 100 fs cost at a documented delay error.

## Effort breakdown
QSS core + tests 1 w · two adopters 1 w · sampled islands + converters 2 w · convergence/coalescing 2 d ·
numerics spec and maintainer review 1 w. Total about 6 weeks.

## Open questions for the maintainer
- One tolerance vocabulary (reuse abstol/reltol for state quanta) or a separate `qtol`?
- Islands per element or per `.subckt` (proposed: both, subckt inherits)?
- Is event coalescing acceptable as an *opt-in* accuracy knob? (Never as default.)
- Should `SystemC-AMS` (issue #14) be evaluated instead of a home-grown island? Its TDF static schedule fits
  lasers well but adds a dependency and a second kernel; proposed: borrow the semantics, not the library.
