# F-04: Receiver chain (photodiode bandwidth, TIA, ADC, electrical output) and source RIN

value 5 | effort M | risk med (needs-human for noise/bandwidth models) | prerequisites: backlog fix for the
never-written `p_readout` | horizon 2

## Problem / user story
*As a PNN or reservoir researcher the receiver decides my accuracy, and I need to model it.* INL's own results
hinge on it: the GEMM study reports 95 % versus 43 % MNIST accuracy depending on photodiode bandwidth and noise
at 1 GHz (Cardoso JLT 2024); the time-delay reservoir papers model a 4th-order Butterworth photodiode with
Gaussian noise, TIA NEP and dark current (Abdalla, in Photontorch); the convolution-accelerator study sweeps
ADC/DAC quantisation (Costa 2025); ADEPT sizes DACs and laser power from shot-noise SNR (SOURCES B).
Today `Detector` computes `I = R |sum_k E_k e^{j omega_k t}|^2` on a fixed sampling tick
(`src/devices/detector.cpp:30-86`, `wait(sampling_time, trigger)` at `:50-51`) with shot, thermal and
TIA-density noise (`:94-112`) but **no bandwidth** (`m_opFreq_Hz` is only a noise bandwidth), no TIA gain, no
quantiser, no saturation; the electrical output write is commented out (`:84`), the readouts are uninitialised
(`src/devices/detector.h:18-19`), and no probe exists for electrical nets (only `sc_trace` of detector members
at `src/specs.cpp:301-306`). Sources have no RIN or linewidth.

## Prior art
INTERCONNECT `photodetector_simple`: responsivity(f, V), dark current, shot/thermal noise, saturation,
**bandwidth as a low-pass filter**; VPI PIN/APD with TIA; Photontorch detector with Butterworth + noise;
INTERCONNECT DML with Langevin RIN/linewidth as outputs; Verilog-A receiver models (SOURCES A, D).

## Proposed design
1. **Photodiode bandwidth and output.** `PDET` kwargs `BW` (3 dB Hz), `ORDER` (Butterworth order, default 1),
   `SAT` (A), `ID` (dark current). The detector already has a sampled island (the `Ts` tick); implement the
   filter as a discrete IIR updated on each tick (bilinear transform of the n-th order Butterworth), write the
   filtered current to `p_readout` when |delta I| exceeds a new electrical tolerance
   (`default_eabstol` next to `default_abstol`, `src/specs.h:103-105`) so downstream electrical nets stay
   event-sparse. Noise bandwidth defaults to `BW` (today a separate `FOP`).
2. **Electrical elements** (new `src/devices/electrical/*.{h,cpp}`, `spx::ea_*` types already exist,
   `src/specs.h:24-51`): `TIA` (transimpedance ohm, bandwidth, input-referred noise A/sqrt(Hz), output V),
   `ADC` (bits, Vref, sample period or clock net, `bits=1` gives a comparator), `LPF` (generic), `EPROBE`
   (trace any electrical net, like `PROBE` for optical nets). These are the pieces every PNN testbench needs
   between the photodiode and the decision.
3. **RIN and phase noise on `CWSRC`** (`RIN` in dB/Hz, `LINEWIDTH` in Hz, `TS` sample period): emit a new
   field sample every `TS` from a seeded generator (F-01); this is a sampled island at the source (cost
   proportional to 1/TS) and the first concrete use of F-08's pattern; keep it off by default.
4. Keep `readout_no_interference` as an option; initialise both readouts to 0.

## Touch points
`src/devices/detector.{h,cpp}` (`detector.h:17-19, 28-35`; `detector.cpp:30-86, 84, 94-112`) ·
`src/parser/parse_element.cpp` (PDET kwargs block, currently `TS`, `R`, `NB`, `FOP`) · new electrical devices +
`src/parser/parser.l:362-457` prefixes (`TIA`, `ADC`, `LPF`, `EPROBE`) and `parser.y:248-392` rules ·
`src/specs.cpp:301-306` (tracing), `src/specs.h:103-105` (electrical tolerance) · `src/devices/cw_source.{h,cpp}`
(RIN) · `utils/specs.kicad_sym` (new symbols).

## Validation plan
- Step response of `PDET BW=1e9 ORDER=1`: 10-90 % rise time 0.35/BW within 2 % (unit test on the sampled
  trace); order 4 matches a reference Butterworth step computed with numpy.
- Noise: with a fixed seed over 1e5 samples the variance matches
  `B (2 q (I + I_d) + 4 k T / R + i_TIA^2)` within 3 %.
- `ADC bits=1` on a PRBS-modulated input reproduces the bit pattern with the expected BER at a chosen SNR
  (analytic Q-function).
- Golden: a receiver-chain netlist (CW source, modulator placeholder, PDET, TIA, ADC) added as a case; existing
  cases unchanged when the new kwargs are absent (bandwidth infinite by default, needs-human to confirm).

## Effort breakdown
Filter + output + tolerance 3 d · TIA/ADC/LPF/EPROBE 3 d · RIN/linewidth 2 d · tests 3 d · docs 1 d.

## Open questions for the maintainer
- Default photodiode bandwidth: infinite (golden-neutral) or a physical default such as 10 GHz?
- Electrical event semantics: always sample at `Ts`, or emit only on tolerance crossing (proposed)?
- Should the ADC clock be a net (`EVLSRC` or a future `CLK` element) or a period parameter? Proposed: both.
