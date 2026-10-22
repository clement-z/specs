# F-10: PCM cell v2 for OCTANE (continuous state, complex transmission, SET/RESET physics, stochasticity)

value 5 (the core user) | effort M | risk high (needs-human: the current model is published) |
prerequisites: F-01 (seeded stochastic transitions), F-08 optional (thermal proxy as a QSS state) | horizon 2

## Problem / user story
*As an OCTANE researcher I need the PCM cell to follow my measured programming curves, including phase, pulse
shape dependence, randomness and electrical actuation.* Cardoso's thesis perspectives ask for phase as a
function of PCM state, transients without return to equilibrium, endurance and volatile VO2; the ASP-DAC 2024
talk lists ~1 ns per amorphisation event, limited endurance and "natural randomness" as open issues; the 2026
Dwivedi/Pavanello review moves to electrically actuated GeSe phase shifters (SOURCES B). The current model
(`src/devices/pcm_device.cpp`): integer state `k`, `T(k) = Tc + (Ta - Tc) tanh(speed k / N)` (`:142-149`, never
reaching `Ta`), one step per 1 ns window when the integral of *incident* power exceeds `Emelt` (`:95-140`,
evaluated only at the next rising edge so the last pulse never programs), real transmission (no phase), one
direction only, constructor defaults `Tc = Ta = 0` (`src/devices/pcm_device.h:49-56`) that make every OCTANE
cell opaque (backlog), no randomness, no drift, no electrical port, positional `Tc/Ta` unreachable in the parser.

## Prior art (SOURCES D §6)
Li et al. 2019: 34 levels; write = a single 20-50 ns pulse whose **peak power** sets the amorphous fraction;
erase = fixed 50 + 200 ns double step; ~200 ns thermal decay, 500 ns settling. Chen et al. 2023 (Sb2S3, PIN
heaters): 32 levels at 0.5 dB/step with trains of identical partial pulses, ~10 us thermal relaxation,
RESET 150-550 ns vs SET ~200 ms. Zheng et al. 2020 electrothermal model: melt at 888 K, crystallisation window
around 673 K, quench rate 1e10-1e11 K/s, "dead time". Rios 2015/2019, Feldmann 2019/2021 (synapses, tensor
core). Partial crystallisation optics: Lorentz-Lorenz effective medium in the crystalline fraction. Carrillo et
al. 2021 built a behavioural PCM model for commercial PIC simulators; electrical PCM Verilog-A models use a
crystalline-fraction state with JMAK kinetics.

## Proposed design (`PCMCELL ... MODEL=2`; v1 kept as default until signed off)
1. **State.** Crystalline fraction `x` in [0, 1] (continuous), optional quantisation to `N` levels for
   reporting; optical response `(n(x), kappa(x))` from Lorentz-Lorenz mixing of the amorphous and crystalline
   complex indices or a user table `TX=[...] PHIX=[...]`; the output field factor is complex (`sqrt(T) e^{j phi}`),
   so phase-sensitive circuits (rings, meshes) see the state.
2. **Thermal proxy.** Cell temperature rise `dT` with `RTH` (K/W) and `TAU_COOL` (s), driven by absorbed optical
   power `(1 - T(x)) P_in` (not incident) plus optional electrical heater power `V^2 / R` from an electrical
   input port; a first-order QSS state (F-08) or, before F-08, a per-pulse closed form.
3. **Transitions.** Amorphisation when `dT` exceeds `TMELT - TAMB` during a pulse and the cooling rate after
   it exceeds `QUENCH`: `delta x` set by the overshoot amplitude (Li 2019); crystallisation by the thermal
   budget above `TX` integrated over time (JMAK-like rate `k0 exp(-Ea / kT)` simplified to a threshold energy
   per level). Dead time emerges from `TAU_COOL`. Stochastic jitter `SIGMA_X` per transition from the seeded
   generator (F-01); drift `T(x, t)` log term and an endurance counter (`MAX_CYCLES`, beyond which `delta x`
   shrinks) optional.
4. **Compatibility mode.** `LEVELS=63 ESTEP=... ERESET=...` maps the thresholds onto the group's existing
   calibration so v1 netlists can be ported with one keyword.

## Touch points
`src/devices/pcm_device.{h,cpp}` (whole model) · `src/parser/parse_element.cpp` (PCMCELL kwargs; positional
assert) · `src/devices/octane_cell.{h,cpp}:32, 77` (defaults; pass parameters through) ·
`src/devices/octane_segment.cpp`, `octane_matrix.cpp` · `src/tb/pcm_device_tb.cpp` (becomes a golden case
for v1) · docs and a `circuits/pcm_programming.cir` example.

## Validation plan
- Li 2019 curve: transmission level versus write-pulse peak power is monotone over 34 steps; a fixed erase pulse
  returns to the crystalline level from any state.
- Chen 2023: `n` identical partial pulses give `n` equal dB steps until saturation; two pulses closer than
  `TAU_COOL` interact, farther apart they do not.
- Energy bookkeeping: absorbed energy equals incident minus transmitted over a pulse.
- Monte-Carlo (F-02) with `SIGMA_X` reproduces the measured 7.73 % normalised device variability of the
  OCTANE cell; v1 golden outputs unchanged.

## Effort breakdown
Model 5 d · parser + OCTANE plumbing 1 d · tests/fixtures 3 d · calibration session with the OCTANE team 2 d.

## Open questions for the maintainer
- Which materials ship as presets (GST, GSST, Sb2Se3, Sb2S3, GeSe) and with whose constants?
- Must v1 remain the default because JLT 2024 results depend on it? Proposed: yes until the OCTANE team
  re-validates with v2.
- Expose `x` (fraction) or level index to OCTANE users? Electrical port conventions for heater-actuated cells?
