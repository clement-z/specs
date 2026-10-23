# 02 — Device library & physics (discovery probe 2/5)

Scope: `src/devices/` (55 files, ~6.5 kLOC), plus the netlist mapping in `src/parser/parse_element.{h,cpp}` because that is where units get converted. I cross-checked against `discover/paper-zrounba2023-specs-ecoc.txt` (ECOC 2023). Static reading only: nothing was built or run. Paths are relative to `src/` unless stated.

---

## 1. Summary

- **Representation.** Each event carries a complex envelope `m_field` plus a wavelength id (`optical_signal.h:26-27`), and `power() = |E|^2` in W. The field is therefore really in sqrt(W). The comment at `optical_signal.h:26` and the paper both call it "V/m", which is wrong.
- **Device pattern.** Most devices follow one recipe (details in section 3): a `XxxBase` holds the parameters, `XxxUni`/`XxxBi` add ports and threads, there is one `SC_THREAD` per input port, S-parameters are computed once when the thread starts, and outputs go out through `OpticalOutputPort::delayedWrite()`.
  - That port object does the event logic: abstol AND reltol gating, per-wavelength desired/emitted maps, time snapping, and merging events that land on the same timestamp (`optical_output_port.cpp:60-73,95-188,326-358`).
- **Linear elements.** All are linear, time-invariant and instantaneous except the waveguide, which carries the group delay. There is no nonlinearity anywhere: `spx_module::ModuleFlags` (`spx_module.h:22-28`) exists but nothing uses it.
- **What checks out.**
  - Waveguide dispersion math: `dneff/dλ = (neff−ng)/λ0`, `d²neff/dλ² = −cD/λ0`, `dng/dλ = cD`.
  - Directional coupler is unitary with `[[t, jκ],[jκ, t]]`.
  - Merger `(E1+E2)/√2` and splitter `√r, √(1−r)` are physically consistent.
  - Detector noise formulas are the standard ones.
  - CROW topology matches the paper: N+1 couplers, 2N waveguides, κ = 0.15 *power* (`crow.h:30`).
- **Serious problems found.**
  1. **The OCTANE crossbar is optically dead.** PCM transmission is always 0 because the constructor defaults are Tc = Ta = 0.
  2. **The photodetector never drives its electrical output.** The write is commented out.
  3. **The bidirectional active MZI is miswired.** The two arms are cross-connected and `p_vin` is bound twice, which raises a SystemC error.
  4. **The crossing is non-passive.** Its crosstalk is in-phase and not energy-limited, and it has no per-wavelength memory.
  5. **The netlist `k` is a field coupling,** while the paper describes κ = 0.15 as a power ratio for the same `k=0.15` netlist.
- **Duplication.** Code is heavily copy-pasted: 3× in the waveguide, 6× in the coupler, 2× in the phase shifter, and the value-list source is cloned. Units are mixed: lengths are cm inside the C++ but m in netlists, and loss is dB/cm or dB/m depending on the class.
- **Tests.** There is one gtest file (`test/devices/waveguide.cpp`). It only checks waveguide phase with neff = ng. The testbenches in `src/tb/` print and trace but contain **no assertions**.

---

## 2. Device catalogue

Abbreviations: o = optical port (`spx::oa_*`, `OpticalSignal`); e = electrical port (`spx::ea_*`, `double`). "tb" is the key in `tb/alltestbenches.cpp`. "test" means a gtest in `src/test`.

| Device (class) | Files | Netlist prefix / keywords | Ports | Key params (units, default) | Model | tb? | test? |
|---|---|---|---|---|---|---|---|
| `WaveguideUni` (=`Waveguide`), `WaveguideBi` | waveguide.h/.cpp | `WG*` (bidir) `L\|LENGTH`(m→×100 cm), `NEFF`, `NG`, `ATT`(dB/cm), `D`(s/m²); positional L neff ng att D (parse_element.cpp:94-125) | Uni: o in→out. Bi: o p0_in/p0_out, p1_in/p1_out | `m_length_cm` (cm, 1e-2 = 100 µm), `m_attenuation_dB_cm` (0), `m_neff` 2.2111, `m_ng` 2.2637, `m_D` (s/m², 0); λ0 = 1.55 µm **hard-coded** | t = 10^(−α·L/20); φ = 2π·neff(λ)·L/λ; neff(λ) Taylor to 1st order (D=0) or 2nd order; τg = L·ng(λ)/c | wg, ring, lambda, ac_* | **yes** (Bi, phase only) |
| `DirectionalCouplerUni` (=`DirectionalCoupler`), `…Bi` | directional_coupler.h/.cpp | `COUPLER*` (bidir) `K\|KF\|KFIELD` (field: Pthr = 1−k²), `KP\|KPOW\|KPOWER` (power), `T` (field through), `LOSS` (dB) (parse_element.cpp:278-332) | Uni: o in1,in2→out1,out2. Bi: o p0..p3 | `m_dc_through_coupling_power` (power fraction, 0.5), `m_dc_loss` (dB, 0), `m_delay_ns` 0, through phase 0, cross phase π/2 | S13=S24=√Pthr·10^(−IL/20); S14=S23=√(1−Pthr)·10^(−IL/20)·e^{jπ/2} | dc, ring, lambda, ac_add_drop | no |
| `Merger` | merger.h/.cpp | `MERGER*` `IL` (dB) | o in1,in2→out | `m_attenuation_dB` (0) | E = (E1+E2)/√2 · 10^(−IL/20), per-λ memory, 0 delay | merger, lambda, ps | no |
| `Splitter` | splitter.h/.cpp | `SPLITTER*` `RATIO`, `IL` | o in→out1,out2 | `m_split_ratio` (power to out1, 0.5), `m_attenuation_dB` | E1 = t√r·E, E2 = t√(1−r)·E (real), 0 delay | splitter | no |
| `CrossingUni` (=`Crossing`), `CrossingBi` | crossing.h/.cpp | `CROSSING*` `ATT` (dB, +loss), `XTALK` (dB, signed); **only Uni is ever instantiated** (parse_element.cpp:590) | Uni: o in1,in2→out1,out2. Bi: o p0..p3 | `m_attenuation_power_dB` (0), `m_crosstalk_power_dB` (NaN → 0) | S13=S24=10^(−att/20); S14=S23=10^(+xt/20), all phase 0, 0 delay | **none** | no |
| `PhaseShifterUni` (=`PhaseShifter`), `…Bi` | phaseshifter.h/.cpp | `PSHIFT*` (bidir optical) `SENSITIVITY\|GAIN\|G` (rad/V), `ATT` (dB) + electrical net | o in→out, e `p_vin` (V) | `m_sensitivity` 1 rad/V, `m_attenuation_dB` 0 | E·10^(−att/20)·e^{j·sens·V}; a change on vin re-emits every stored λ; 0 delay | ps | no |
| `MZIActiveUni` (=`MZIActive`), `…Bi` | mzi_active.h/.cpp | `MZI*` (5 nets) `SENSITIVITY`, `LENGTH`(m), `LENGTH_REF`(m), `NEFF`, `NG`, `ATT`(dB/cm), `IL_DC`, `IL_PHASESHIFTER` | o in1,in2→out1,out2, e vin | sens 1 rad/V, L 0, Lref 0, neff **2.2**, ng **2.6**, losses 0 | Composite: DC(50/50) → [WG(L/2)·PS·WG(L/2)] ∥ WG(Lref) → DC(50/50) | **none** | no |
| `MZI` (2 phase shifters, Clements cell) | mzi.h/.cpp | not exposed (parse_element.h:82 commented out) | o in1,in2→out1,out2; e vphi, vtheta | `length_cm` 1e-2, att dB/cm, coupler dB, ps dB, neff/ng 2.2111/2.2637 | PS(φ) on in1 → DC → [PS(θ)+WG] ∥ WG → DC | mzi | no |
| `MeshCol` | mesh_col.h/.cpp | — | o N in/out; e ⌊N/2⌋ (or N/2−1) vphi/vtheta | as MZI | One Clements column: MZIs plus bypass WGs on idle rows | (via mesh) | no |
| `Clements` | clements.h/.cpp | — | o N in/out; e N(N−1)/2 vphi/vtheta | as MZI | N columns of `MeshCol`; **no output phase screen D** | mesh | no |
| `CROW` | crow.h/.cpp | — | o in, add → out_t, out_d | N 3, `m_ring_length` (m), neff 2.2, ng 4.3, 2 dB/cm, Pthr 0.85 | N rings, each 2 × WG(L/2), plus N+1 DCs with alternating direction | ac_crow | no |
| `Ring` | ring.h/.cpp | — | o in→out | `length_cm` default **30e-6 cm** | DC+WG loop; **dead code** (thread function never defined) | no (ring tb hand-builds DC+WG) | no |
| `PCMElement` | pcm_device.h/.cpp | `PCMCELL*` `EMELT`(J), `NSTATES`, `K\|INITIAL_STATE`, `TC`, `TA`, `TANH_COEF` | o in→out | Emelt (J, **required**), N (**required**), Tc/Ta (power transmission, **ctor default 0/0**), influence window 1 ns, speed 3 | T = Tc+(Ta−Tc)·tanh(speed·k/N); E_out = √T·E; k++ if ∫P dt > Emelt | pcm | no |
| `Detector` | detector.h/.cpp | `PDET*` positional Ts R; `TS`, `R\|RESPONSIVITY`, `NB`, `FOP` | o in → e `p_readout` (**never written**) | R 1 A/W, Id 0 A, noiseBypass true, B 1 GHz, T 300 K, Req 400 Ω, i_TIA 10 pA/√Hz, Ts 1 ps | I = R·\|Σ_k E_k e^{j2πf_k t}\|² + noise + Id; σ² = B(i_TIA² + 2q(I+Id) + 4kT/Req) | det | no |
| `PowerMeter` | power_meter.h/.cpp | `PWR*` | o in | — | Σ_k \|E_k\|² (incoherent), VCD trace only | no | no |
| `Probe`, `PowerProbe`, `PhaseProbe` | probe.h/.cpp | `PROBE*` flags POWER/MAG/PHASE/WL | o in | — | Traces P, \|E\|, arg E, λ of the **last event**, whatever its λ | most tbs | no |
| `MLambdaProbe` | probe.h/.cpp | `MLPROBE*` `WL=[…]` | o in | λ set | Per-λ traces, keyed by exact `double ==` on λ | lambda, ps | no |
| `CWSource` | cw_source.h/.cpp | `CWSRC*` `WL`(m), `POWER`(W), `PHI`(rad) | o out | — | E = √P·e^{jφ}; emits on enable, re-emits on reset (DC sweep) | lambda | no |
| `VLSource` | value_list_source.h/.cpp | `VLSRC*` `FILE`, `VALUES`, `SCALE` | o out | CSV `t[s],P[W],λ[m]` | E = √P (phase 0) at time t | no (netlists) | no |
| `EVLSource` | electrical_value_list_source.h/.cpp | `EVLSRC*` | e out | CSV `t[s],V[V]` | Writes V directly (no output-port object) | no | no |
| `BitstreamSource` | bitstream_source.h/.cpp | — | clk, enable → o out | 8 bits/value (static), on/off power | One bit per clk edge (pos and neg), LSB first | no | no |
| `OctaneCell` / `OctaneSegment` / `OctaneMatrix` | octane_*.h/.cpp | — | o row/col in(/out); e PD readouts | Emelt 10 pJ, N 63, neff = ng = 2.2, all losses 0 | Cell: [WG]·Merger·[WG]·PCM·[WG]·PD. Segment: tap DCs with Pthr = 1−1/(N−j). Matrix: segments plus crossings | **none** | no |
| `GenericTransmissionDevice` + `TransmissionMatrix` | generic_transmission_device.h/.cpp | — | N ports, each in+out | Taylor coeffs of α, φ, τ around λ0 | One `sc_spawn` process per active (i,j); emits *deltas* (`m_use_deltas`) | no | no |
| `Generic2x2Coupler` | generic_2x2_coupler.h | — | 4 | k_power [0,1] (asserted), IL dB | Unitary 2×2 through TM (order 0 only) | commented out in dc tb | no |
| `GenericWaveguide` | generic_waveguide.h | — | 2 | L (m), loss (dB/m), neff, ng, D, λ0 | **Disabled**: `exit(1)` in ctor (:18-19) | commented out | no |
| `TimeMonitor` | time_monitor.h/.cpp | CLI `add_time_monitor` (main.cpp:374-380) | — | poll period, wall-clock period | Progress print, then **`exit(1)`** (:108) | — | — |
| `SubcircuitInstance` | subcircuit_instance.h | `X*` | — | — | Container that holds signals and modules | — | — |
| `spx_module` | spx_module.h | — | — | `flags` (unused), virtual `init()`, `describe()` | Base class | — | — |

---

## 3. Class hierarchy and shared patterns

**Hierarchy**
- `sc_module` → `spx_module` (spx_module.h:12-35): virtual `init()` is called on *every* spx_module by `SPECSConfig::prepareSimulation()` (specs.cpp:359-361); `describe()` is a stub.
  - Base/Uni/Bi triples:
    - `WaveguideBase`/`Uni`/`Bi`
    - `DirectionalCouplerBase`/`Uni`/`Bi`
    - `CrossingBase`/`Uni`/`Bi`
    - `PhaseShifterBase`/`Uni`/`Bi`
    - `MZIActiveBase`/`Uni`/`Bi` (Base is abstract: `init`/`connect_submodules` are pure)
  - Single classes: `Merger`, `Splitter`, `PCMElement`, `Detector`, `PowerMeter`, `Probe` (← `PowerProbe`, `PhaseProbe`), `MLambdaProbe`, `CWSource`, `VLSource`, `EVLSource`, `MZI`, `MeshCol`, `Clements`, `SubcircuitInstance`.
  - `GenericTransmissionDevice` (abstract `prepareTM()`) ← `Generic2x2Coupler`, `GenericWaveguide`.
- Plain `sc_module` (not spx, so `prepareSimulation` does not auto-init them): `CROW`, `Ring`, `OctaneCell`, `OctaneSegment`, `OctaneMatrix`, `BitstreamSource`, `TimeMonitor`.

**Parser binding.** `DECLARE_BIDIR_ELEMENT(..., Waveguide, 2)` token-pastes `Waveguide##Base/Uni/Bi` (parse_element.h:52-75, 80-97). The bidir variant is chosen when any connected net is bidirectional (`&` specifier, parser.l:10), e.g. parse_element.cpp:78-84. Prefixes are case-insensitive (parser.l:27). They are `WG`, `COUPLER`, `MERGER`, `SPLITTER`, `PSHIFT`, `MZI`, `CROSSING`, `CWSRC`, `VLSRC`, `EVLSRC`, `PROBE`, `MLPROBE`, `PDET`, `PCMCELL`, `PWR`, `X` (parser.l:362-456).

**Per-device runtime recipe** (seen in WG, DC, Merger, Splitter, PS, PCM):
1. Constructor: `SC_THREAD(on_<port>_changed); sensitive << <port>;`, with one thread per input port. Crossing is the exception: one thread sensitive to all ports.
2. Thread prologue computes S-parameters from member variables. This runs at simulation start, after the parser has set parameters. Changing a parameter after start has **no effect**.
3. Loop: `wait()` → `read()` → store `m_memory_inX[wavelength_id] = field` (a `std::map<uint32_t, complex>`) → combine with the other ports' memory **at the same λ** → `OpticalOutputPort::delayedWrite(sig, delay)`.
4. `OpticalOutputPort` (optical_output_port.cpp):
   - It schedules the event at `t+delay`, snapped up to the resolution (:326-358).
   - It merges or replaces events with the same (t, λ) (:340-357).
   - At emission it emits only if `|desired−emitted| > abstol && |Δ|/|emitted| > reltol` (:60-73, :161-165).
   - It snaps |E| < 10·abstol to 0 (:169-170).
   - In `m_use_deltas` mode it accumulates deltas instead of replacing values (:152-158). Only `GenericTransmissionDevice` uses that mode (generic_transmission_device.cpp:76).
5. Composites (`MZI`, `MZIActive`, `MeshCol`, `Clements`, `CROW`, `Octane*`) create submodules and wires in `init()` and have no processes of their own.

**Delays.** Only the waveguide (and the disabled GenericTransmissionDevice) has a non-zero delay. Every other device emits at `SC_ZERO_TIME`, i.e. after a delta cycle. A loop made only of zero-delay devices spins in delta cycles forever. The parser guards only the trivial case of a waveguide connected to itself (parse_element.cpp:72-77).

**"NaN wavelength" sentinel does not exist.** `OpticalSignal(0)` maps to `m_wavelength_id = 0`, which is the *first registered* wavelength (optical_signal.h:35-38). `getIDFromWavelength` never stores NaN (optical_signal.cpp:111-137). Consequences:
- The `m_memory_inX[0] = 0; // initializing for nan wavelength` lines (e.g. directional_coupler.cpp:11) and the `isnan(s.getWavelength())` checks (probe.cpp:35, generic_transmission_device.cpp:177) are no-ops or dead code.
- detector.cpp:68 indexes `wavelengths_vector[0]` without a bounds check.

---

## 4. Physics review per device

### Waveguide (`devices/waveguide.cpp`)
- Loss: `m_attenuation_dB = att_dB_cm · L_cm` (:11); field t = 10^(−dB/20) (:14). Units are consistent (dB/cm × cm). The loss is wavelength-independent.
- Group delay: `τg[ns] = 1e9 · L_cm·1e-2 · ng / c` (:18). With D ≠ 0, `ng(λ) = ng0 + c·D·(λ−λ0)` (:29, :86). Since dng/dλ = −λ·d²neff/dλ² = c·D, this is correct.
- Phase: `φ = 2π·L·neff(λ)/λ` (:21, :62); `neff(λ) = neff0 + (neff0−ng0)/λ0·Δλ` (:25, :60). This follows from ng = neff − λ·dneff/dλ and is correct. With D: `+ (−c·D/(2λ0))·Δλ²` (:28, :84-85). Since d²neff/dλ² = −c·D/λ, this is correct. The field is multiplied by `polar(t, +φ)` (:65), so the sign convention is **e^{+jβL}**, the same as Photontorch.
- λ0 is fixed at **1.55e-6** (:25, :28, :60, :84) and cannot be set. neff/ng values measured at another reference wavelength are silently misinterpreted.
- `WaveguideBi` duplicates all of this twice (:104-201, :205-280).
- **gtest** `test/devices/waveguide.cpp:85-88,155-166` uses neff = ng, so the slope term is zero. It only checks the OP phase at 6 wavelengths. Loss (3.0103 dB/cm × 1 cm, i.e. 0.5 power), delay and D are never asserted.

### Directional coupler (`devices/directional_coupler.cpp`)
- `Pthr_dB = 10·log10(p) − IL`, `Pcross_dB = 10·log10(1−p) − IL` (:8-9), converted to field via 10^(dB/20) (:13-14). Through phase 0, cross phase +π/2 (directional_coupler.h:14-15).
- **Energy check:** |t|²+|κ|² = (p + 1−p)·10^(−IL/10) ≤ 1. The cross terms are in quadrature, so the matrix is unitary for IL = 0. Correct.
- Bidirectional reciprocity: forward S02 = S13 = through, S03 = S12 = cross; reverse p2→p0 through, p2→p1 cross (:263-269). Consistent.
- **No validation.** p ∉ [0,1] gives log10(<0) = NaN fields that propagate silently. The parser `K` maps to `1−k²`, so k > 1 produces this case (parse_element.cpp:305, 316). Compare `Generic2x2Coupler`, which asserts.
- **Netlist semantics vs paper.** `K` is a *field* coupling (parse_element.cpp:281 even prints "coupling power for DC currently set to field"). The paper's Fig. 1 netlist uses `k=0.15`, and the text calls κ = 0.15 a "power cross-coupling ratio". `circuits/add_drop_pyspecs.cir:10-11` copies `k=0.15`, which means P_cross = 2.25%. The C++ CROW benchmark uses power 0.15 correctly (crow.h:30).
- Every input event re-emits *both* outputs, even if the second output did not change. Delay is 0.

### Merger / Splitter
- Merger: `t = 10^(−IL/20)/√2`, `E = t·(E1+E2)` (merger.cpp:8, :34-38). This is the ideal symmetric Y-combiner. Power is not conserved, which is physical (antisymmetric mode radiates).
- Splitter: `S12 = t√r`, `S13 = t√(1−r)` (splitter.cpp:7-8). Lossless when IL = 0. There is no validation of r (r ∉ [0,1] gives NaN). The verbose print hard-codes "0.5" (:14).

### Crossing (`devices/crossing.cpp`)
- `x = 10^(+XT_dB/20)` (:9) and `t = 10^(−ATT_dB/20)` (:10). The two parameters therefore use **opposite sign conventions** in one device.
- S = [[t, x],[x, t]] with both entries real and in phase (:16-19, :72-75). This is not unitary whenever x > 0: S†S has off-diagonal 2tx ≠ 0. With att = 0 and xt = −20 dB, |t|²+|x|² = 1.01, which is **gain**. Coherent inputs can then interfere constructively above the input power.
  - Expected for a passive device: |t|²+|x|² ≤ 1, with the cross term ±π/2 relative to through, e.g. `x → j·x`, or t derived as √(10^(−att/10) − x²).
- **No per-wavelength memory.** The thread reads both ports' last events (:42-43) and sums them. Different wavelengths on in1 and in2 then hit `OpticalSignal::operator+=`, which prints an error in TD mode and keeps the larger λ in FD mode (optical_signal.cpp:17-32).
- Parser: Uni outputs are connected `AS_READER` (parse_element.cpp:539-540), and only Uni is ever instantiated (:590).

### Phase shifter (`devices/phaseshifter.cpp`)
- `E_out = 10^(−att/20) · e^{j·sens·V} · E_in` (:41, :59). Linear in V. There is no Vπ, no V-dependent loss, and no electrical bandwidth (the response is instantaneous). Sign: positive V·sens advances phase like +Δneff in the waveguide convention, which is consistent.
- A change on vin re-emits every stored wavelength (:62-77). Correct.

### MZI (2-PS) / MZIActive / MeshCol / Clements
- `MZI::init` (mzi.cpp:8-55): PS(φ) on in1 → DC 50/50 → upper [PS(θ) → WG(L)], lower WG(L) → DC 50/50. This is the standard Clements/Reck cell. The voltage equals the phase in rad when sens = 1.
- `MZIActiveUni` (mzi_active.cpp:3-84): upper arm WG(L/2)+PS+WG(L/2), lower WG(Lref). Correct.
- **`MZIActiveBi` is miswired** (mzi_active.cpp):
  - Forward: `wg3->p1_out(*m_ps_p1_out)` (:172) and `dc2->p1_in(*m_ps_p1_out)` (:175). It should be `m_dc2_p1_in`, which is created at :92 but never used. The lower arm writes onto the phase shifter's output net and dc2 port 1 reads the upper arm.
  - Backward has the same error: `wg3->p1_in(*m_ps_p1_in)` (:199) and `dc2->p1_out(*m_ps_p1_in)` (:202).
  - `ps->p_vin(p_vin)` is bound twice (:165, :192). The vendored SystemC rejects this at `complete_binding` ("interface already bound to port", thirdparty/systemc/src/sysc/communication/sc_port.h:550-558). Any `MZI*` netlist element on a bidirectional net should fail at elaboration.
  - Wire name typo: `"/DC1_in2_1"` for a DC2 wire (:100).
- `MeshCol`: comments say "Zero neff so it doesn't add any phase" (mesh_col.cpp:123 and similar), but the code passes `m_neff`. Bypass WGs match the arm WG length, so delay and loss are matched. The phase does not include the MZI's DC factors (two `j` terms), so the programmed phases must absorb that.
- `Clements`: N columns and N(N−1)/2 MZIs, which is the correct count. There is no final diagonal phase screen, so not every unitary is reachable without external phase shifters.

### CROW (`devices/crow.{h,cpp}`)
- Pthr = 0.85 (κ_power = 0.15). This matches the paper. Defaults are neff 2.2, ng 4.3, 2 dB/cm (crow.h:27-30).
- Each ring is 2 × WG(L/2) (crow.cpp:28-47). The alternating direction for even/odd rings is correct (counter-rotating coupled rings), and the DC through path keeps light inside a ring.
- **Paper benchmark not reproducible:** the paper samples L_i ~ N(33.2, 5.0) µm per waveguide, but the code applies the same length to every waveguide. `crow_tb.cpp:179` uses 60 µm per ring. (The testbench was removed in v0.4.0; `tests/bench/crow_gen.py` draws the paper's lengths.)
- Debug `cout` spam: crow.h:62,74 and crow.cpp:33,39,45.

### Ring (`devices/ring.h`) — dead code
- `SC_THREAD(on_port_in_changed)` (:87) refers to a member declared at :59 that is never defined. It compiles only because nobody constructs a `Ring`. `SC_HAS_PROCESS(Waveguide)` (:85) is a no-op in SystemC 3.
- Default `length_cm = 30e-6` (:66) is a 0.3 µm circumference, almost certainly meant as 30 µm.
- `m_through_phase_rad = π/2` (:45) should be 0.
- The `m_*` parameters are never pushed to `dc`/`wg`.

### PCM (`devices/pcm_device.{h,cpp}`)
- T(k) = Tc + (Ta−Tc)·tanh(speed·k/N) (:144); field = √T (:148), real (no phase change).
- At k = N, T = Tc + 0.995·(Ta−Tc) with speed 3, so it never reaches Ta. N = 0 gives NaN; the constructor default is N = 0 (pcm_device.h:51), but the parser requires a non-zero value.
- **Constructor defaults Tc = 0, Ta = 0** (pcm_device.h:53-54) override the header defaults (`m_Ta = 1`, :27). With them T ≡ 0 for all k. `OctaneCell` relies on these defaults (octane_cell.cpp:32, :77), so every OCTANE cell is opaque. The PCM tb passes 0.85/0.95 explicitly (pcm_device_tb.cpp:92).
- Energy: Σ P_in·Δt over the recorded samples (:121) is compared to Emelt.
  - It uses *incident* power, not absorbed power ((1−T)·P or an absorption coefficient).
  - It is evaluated only at the **next rising edge** once ≥ 1 ns has passed (:110), so the last pulse of a simulation never programs the cell.
  - It allows +1 state per window regardless of how far the energy exceeds Emelt.
  - These are model choices that need a human decision.
- The power sum over wavelengths ignores heterodyne terms (:46-51). That is acceptable for thermal effects.
- Parser: `assert(args.size() <= 2)` (parse_element.cpp:1202) makes positional Tc/Ta (:1216-1218) unreachable.

### Detector (`devices/detector.cpp`)
- I = R·|Σ_k E_k·e^{j2πf_k·t}|² (:66-77). The noise is
  σ² = B·(i_TIA² + 2q(I+Id) + 4kT/Req) (:102-111).
  - These are the standard TIA-input, shot and thermal terms. B is the parameter `m_opFreq_Hz`, which is called "operating frequency", not noise bandwidth.
  - The variable named `inoise_rms` actually holds the variance; `sqrt` is applied correctly.
- **`p_readout->write(...)` is commented out** (:84). The `PDET` output net is never driven. The value only reaches the VCD through `Detector::trace` (specs.cpp:301-305).
- **Sign convention.** The beat model uses e^{+jωt}. Under that convention a pure delay τ multiplies the field by e^{−jωτ}, but the waveguide applies e^{+jβL}. With two or more wavelengths the beat-note phase is mirrored. Single-wavelength intensity is not affected.
- Precision: 2π·f·t with f ≈ 2e14 Hz grows to ~1e15 rad at t ≈ 1 s, where double precision gives ~0.1 rad error. Using (f_k − f_ref) would fix this.
- The RNG is seeded from `std::random_device` (:135), and a 1e-20·N(0,1) field is injected whenever the total field is exactly 0 (:74-75). Runs are not bit-reproducible even with noise bypassed.
- Sampling: `wait(Ts, event)` with Ts = 1 ps (:51) wakes every ps while anything is pending, which costs performance.

### Sources / probes / meters
- CW: |E| = √P (cw_source.h:49-52). Correct (|E|² = W). Parser positional arguments are off by one: `args[1]` is λ, `args[2]` is P, `args[3]` is unreachable, and `args[0]` is ignored (parse_element.cpp:637-652).
- VL/EVL: the first line of the file is always discarded *before* the header check (value_list_source.h:65, electrical_value_list_source.h:62), so a header-less file loses its first data row.
- Probe: an event that arrives while the probe is disabled is dropped, and after enable it waits for the *next* change (probe.cpp:27-31). Single-λ probes trace whichever λ came last.
- PowerMeter: incoherent Σ|E_k|². That is the right definition of a DC power meter.
- BitstreamSource: saturation check uses `m_current_value` instead of `val` (bitstream_source.cpp:29). This affects the display only.

### OCTANE (`octane_cell/segment/matrix`)
- `length_for_2pi = 100·λ/neff` is passed as cm, i.e. λ/neff in m, so φ = 2π. Correct. `length_for_pi_2 = length_for_2pi/4` compensates the missing DC +π/2 (octane_segment.cpp:78-79). This sign matches the waveguide e^{+jβL} convention.
- Tap ratios: Pthr = 1 − 1/(N−j) (octane_matrix.cpp:117,126,133-134) give equal power per cell. Correct.
- Fatal issue: PCM T ≡ 0 (see PCM above).

### GenericTransmissionDevice / GenericWaveguide / Generic2x2Coupler
- `TransmissionMatrix::operator()` does a Taylor expansion with 1/n! (generic_transmission_device.cpp:9-63). Coefficients are therefore derivatives.
- `GenericWaveguide::prepareTM` (generic_waveguide.h:69-111), compared with φ(λ) = 2π·neff(λ)·L/λ:
  - `dphi_0 = fmod(2πL·neff, 2π)` (:78) is **missing /λ0**.
  - `dphi_1 = −2πL·ng/λ0²` (:81) is **correct**, even though the comment on that line says it is wrong.
  - `dphi_2 = 2πL·2ng/λ0³` (:82) omits the dispersion term; it should be 2πL·(2ng/λ0³ − c·D/λ0²).
  - `group_delay_1 = L·D` (:85) is correct.
  - The class is disabled (`exit(1)`, :19).
- `input_on_i_output_on_j` latches Sij at the first λ, calls `sc_stop()` on any other λ (:200-208), and prints "i, j is active" for every pair (:157). `input_on_i` is unused.
- `Generic2x2Coupler`: k is asserted in [0,1] and the matrix is unitary with π/2 on the cross path (generic_2x2_coupler.h:18, :30-82). Fine, but nothing reaches it from a netlist.

---

## 5. Duplication and inconsistency

**Copy-paste**
- WaveguideUni/Bi: 3 near-identical blocks of about 75 lines (waveguide.cpp:6-101, 104-201, 205-280).
- DirectionalCoupler: 6 threads repeat the S-matrix setup (directional_coupler.cpp:6-318).
- PhaseShifter: 5 threads.
- Crossing Uni/Bi.
- `EVLSource` is a sed-clone of `VLSource` (diff shows only V vs P,λ).
- OctaneCell `init_full`/`init_compact` and OctaneSegment full/compact repeat the parameter block.
- The `__modname` macro is redefined in 6 .cpp files.
- `port_in_type` typedefs are repeated at global scope in octane_segment.h:11-14 and octane_matrix.h:10-13.

**Length units**
- cm internally: Waveguide, MZI, MZIActive, MeshCol, Clements, Ring.
- m: CROW, GenericWaveguide, all netlist keywords (×100 in the parser).

**Loss units**
- dB/cm: WG, MZIActive.
- dB/m: GenericWaveguide.
- dB as positive loss: DC, Merger, Splitter, PS, crossing `att`.
- dB as a signed ratio: crossing `xtalk`.

**Time units**
- ns: WG group delay, `m_delay_ns`, PCM `m_influence_time_ns`.
- s: Detector Ts, VL times, TM τ.

**Default neff/ng**

| Device | neff | ng | Source |
|---|---|---|---|
| WG / MZI / Mesh / Clements | 2.2111 | 2.2637 | |
| MZIActive | 2.2 | 2.6 | |
| CROW | 2.2 | 4.3 | |
| Octane | 2.2 | 2.2 | |
| GenericWaveguide | 1 | 1 | |
| add_drop.cir | 2.3 | 4.3 | circuits/add_drop.cir |

**Multi-wavelength handling**
- Per-λ memory: DC, Merger, PS, PCM, Detector, PowerMeter.
- None (mixes wavelengths): Crossing.
- Single wavelength only (`sc_stop`): GenericTransmissionDevice.
- Single-input devices (WG, Splitter) are fine without it.

**Base class.** `spx_module` is used inconsistently: CROW, Octane* and Bitstream derive from `sc_module`, so `prepareSimulation()` will not `init()` them and testbenches must call `init()` manually.

**Logging.** Unconditional `cout` in hot or elaboration paths: cw_source.cpp:33, value_list_source.cpp:62, crow.*, generic_transmission_device.cpp:157, mesh_col.cpp ("odd-even").

---

## 6. Unknowns (need build/run or author input)

1. Whether `MZIActiveBi` really aborts at elaboration. Static reading of the vendored SystemC 3.0.1 says yes; it needs a run with a `&` net on an MZI.
2. Whether `mesh_tb` survives a double `init()`: `mesh1.init()` at mesh_tb.cpp:91, then `prepareSimulation()` re-calls `init()` on Clements, every MeshCol and every MZI (specs.cpp:359-361), recreating submodules after ports were bound. Possible double-binding or use-after-free.
3. Which coupling semantics the paper's Fig. 1 add-drop curve actually used (field k = 0.15 or power).
4. What the intended global phase convention is (e^{+jωt} or e^{−jωt}). Waveguide, DC, PS and Octane are self-consistent with e^{+jβL}; only the detector's beat term disagrees.
5. Whether the PCM energy model (incident vs absorbed power, one step per window, lazy update) is intentional for the OCTANE work. The paper does not describe it.
6. Whether `Ring`, `GenericWaveguide`, `Generic2x2Coupler`, `BitstreamSource` and `TimeMonitor` are meant to be kept. Most look abandoned.
7. Clements MZI phase convention versus the Clements et al. 2016 decomposition. The output phase screen and the DC `j` factors were not checked numerically.
8. The OOP rule that snaps |E| < 10·abstol to zero (optical_output_port.cpp:169) makes the effective floor 10 fW, not the paper's 0.1 fW. This belongs to the core probe; it is noted here because every device relies on it.

---

## 7. Seeds for backlog

- [severity: high] [physics] devices/pcm_device.h:53-54 + devices/octane_cell.cpp:32,77 — The PCMElement constructor defaults `Tc=0, Ta=0` override the header default `m_Ta=1` (pcm_device.h:27). OctaneCell builds PCMs without Tc/Ta, so T(k) ≡ 0: every OCTANE cell is opaque and its photodetector reads 0. A `PCMCELL` netlist element without `tc`/`ta` is also opaque. Expected: physical defaults (e.g. Tc≈0.85, Ta≈0.95 as in pcm_device_tb.cpp:92) or an error when they are not given.
- [severity: high] [code] devices/detector.cpp:84 — `p_readout->write(m_cur_readout)` is commented out, so the `PDET` electrical output net is never driven. Electrical consumers (OCTANE `p_readout`, netlist nets) always see 0, and the readout exists only in the VCD via `trace()`.
- [severity: high] [code] devices/mzi_active.cpp:172,175,199,202 — `MZIActiveBi` lower arm is bound to the phase-shifter nets (`m_ps_p1_out`/`m_ps_p1_in`) instead of `m_dc2_p1_in`/`m_dc2_p1_out`, which are created but unused. This mixes the arms and gives one net two writers. Expected: `wg3->p1_out(*m_dc2_p1_in); dc2->p1_in(*m_dc2_p1_in);` and the symmetric fix for backward.
- [severity: high] [code] devices/mzi_active.cpp:165,192 — `ps->p_vin(p_vin)` is bound twice, so SystemC reports "interface already bound to port" (sc_port.h:550-558). Any bidirectional `MZI*` netlist element should fail. Remove the second bind.
- [severity: high] [physics] parser/parse_element.cpp:305,316 (vs paper Fig. 1 and §Results) — netlist `k` is the **field** cross-coupling (P_cross = k²), but the paper calls κ = 0.15 a power ratio and its Fig. 1 netlist uses `k=0.15`. Either the docs/paper example is off by a square or the semantics changed. Decide on one meaning, document it, and consider making `k` unambiguous (keep only `kf`/`kp`).
- [severity: high] [physics] devices/crossing.cpp:9-19,62-75 — the crossing S-matrix `[[t,x],[x,t]]` is real and in-phase, which is non-unitary and can exceed 1. Example: att = 0, xtalk = −20 dB gives |t|²+|x|² = 1.01, and coherent inputs can show gain. Expected passive model: |t|²+|x|² ≤ 1 (e.g. t = √(10^(−att/10) − |x|²)) and a cross term in quadrature (`x·e^{jπ/2}`). Also unify the dB sign convention: att is positive loss but xtalk is a signed ratio.
- [severity: high] [code] parser/parse_element.cpp:539-540 — Crossing Uni connects `p_out1`/`p_out2` `AS_READER` instead of `AS_WRITER`, which corrupts net reader/writer counts. Separately, `CrossingBi` is never instantiated (:590 always passes `false`).
- [severity: med] [physics] devices/crossing.cpp:37-56 — no per-wavelength memory: the device reads the last event on each port and sums them, so different wavelengths on in1 and in2 collide in `OpticalSignal::operator+` (error in TD mode, larger λ wins in FD mode). Expected: a per-λ `map` like DirectionalCoupler.
- [severity: med] [physics] devices/detector.cpp:70 vs devices/waveguide.cpp:65 — the detector beat term uses e^{+j2πft}, under which a delay τ should multiply the field by e^{−jωτ}, but the waveguide applies e^{+j2πneffL/λ}. Multi-λ beat-note phase or time is mirrored. Pick one convention globally and document it. Also use (f_k − f_ref)·t to avoid precision loss at large t.
- [severity: med] [physics] devices/waveguide.cpp:25,28,60,84 (and Bi copies at :123,126,…) — reference wavelength λ0 = 1.55 µm is hard-coded for the neff/ng/D Taylor expansion. Add a `lambda0` parameter, as GenericWaveguide has.
- [severity: med] [physics] devices/pcm_device.cpp:110-131 — (a) the energy integrates *incident* power, not absorbed power ((1−T)·P or an absorption coefficient); (b) at most +1 state per window regardless of E/Emelt; (c) the update happens only at the next rising edge after 1 ns, so the final pulse never programs. Needs a human decision on the intended model.
- [severity: med] [physics] devices/pcm_device.cpp:144 — T(N) = Tc + (Ta−Tc)·tanh(speed), only 99.5% of the span at speed = 3, so Ta is never reached. N = 0 (the constructor default, pcm_device.h:51) gives NaN. Consider normalising by tanh(speed) and asserting N > 0.
- [severity: med] [physics] devices/generic_waveguide.h:78,82 — `dphi_0 = fmod(2πL·neff, 2π)` is missing `/λ0`. `dphi_2` should be `2πL·(2ng/λ0³ − c·D/λ0²)`. The comment at :81 wrongly flags `dphi_1`, which is correct. The class is disabled now; fix before re-enabling.
- [severity: med] [physics] devices/directional_coupler.cpp:8-9, devices/splitter.cpp:7-8, parser/parse_element.cpp:305-321 — no range check on the coupling or split ratio. `k > 1`, `kp ∉ [0,1]` or `ratio ∉ [0,1]` silently produce NaN fields. Assert or clamp as Generic2x2Coupler does (generic_2x2_coupler.h:18).
- [severity: med] [code] devices/time_monitor.cpp:108 — unconditional `exit(1)` once the wall-clock period elapses. The `add_time_monitor` CLI option (main.cpp:374-380) kills any simulation longer than 0.5 s.
- [severity: med] [code] specs.cpp:359-361 + tb/mesh_tb.cpp:91 — `prepareSimulation()` calls `init()` on every spx_module, but Clements/MeshCol/MZI are also init()'d manually, so submodules are recreated after binding. Make `init()` idempotent, or stop auto-calling it for children.
- [severity: med] [code] devices/detector.cpp:74-75,135 — RNG is seeded from `random_device` and a 1e-20 random field is injected when dark, so runs are not reproducible even with noise bypass. Add a seed option and drop the dark-field injection.
- [severity: med] [code] devices/value_list_source.h:65, devices/electrical_value_list_source.h:62 — the first line is always discarded before the header check, so the first sample of a header-less file is lost.
- [severity: med] [code] parser/parse_element.cpp:637-652 (CWSRC) and :1202 vs 1216-1218 (PCMCELL) — positional arguments are off by one or unreachable: CW ignores `args[0]` and `args[3]` can never be reached; PCM Tc/Ta positional values are blocked by `assert(args.size() <= 2)`.
- [severity: low] [physics] optical_signal.h:26 (and paper "10^-8 V m^-1") — the field is labelled V/m but `power() = |E|²` is in W, so the field is in √W. Fix the unit labels in code, print-outs (optical_signal.h:234) and docs.
- [severity: low] [physics] devices/mesh_col.cpp:123-126 (and similar) — comment says "Zero neff so it doesn't add any phase" but `m_neff` is passed. Bypass rows also lack the MZI's DC `j` factors, and Clements has no output phase screen. Document the phase convention needed to program arbitrary unitaries.
- [severity: low] [physics] devices/crow.cpp:28 — CROW uses identical waveguide lengths, so the paper benchmark (L_i ~ N(33.2, 5.0) µm) cannot be reproduced from the repo. Add per-waveguide lengths or variation.
- [severity: low] [physics] cross-device defaults — neff/ng defaults differ per device (2.2111/2.2637, 2.2/2.6, 2.2/4.3, 2.2/2.2, 1/1). Centralise them in a technology or default config.
- [severity: low] [code] devices/ring.h:45,59,66,85 — `Ring` is dead: undefined thread function, 30e-6 cm default length, through phase π/2, parameters never applied. Delete it or finish it.
- [severity: low] [code] devices/probe.cpp:27-31 — an event arriving while disabled is dropped, so the OP→TRAN steady-state value is not traced until the next change. Trace the current value on enable.
- [severity: low] [code] devices/splitter.cpp:14, devices/bitstream_source.cpp:29 — cosmetic: hard-coded "0.5" in the verbose print; saturation check uses the wrong variable.
- [severity: low] [code] devices/crow.h:62,74, devices/crow.cpp:33-45, devices/generic_transmission_device.cpp:157, devices/cw_source.cpp:33, devices/value_list_source.cpp:62 — unconditional `cout` spam. Gate it on `verbose_component_initialization`.
- [severity: low] [code] devices/waveguide.cpp, devices/directional_coupler.cpp, devices/phaseshifter.cpp, devices/crossing.cpp — 3–6× copy-pasted thread bodies for Uni/Bi. Factor S-parameter computation into a Base method (this also makes it unit-testable).
- [severity: low] [code] test/devices/waveguide.cpp:85-88,155-166 — the only device unit test uses neff = ng (zero slope) and checks phase only. Add loss, group delay, dispersion (D ≠ 0), DC unitarity and crossing/merger tests. No tb in `src/tb/` asserts anything.
- [severity: low] [code] optical_signal.h:35-38, devices/*:`m_memory_*[0]=0` — the "NaN wavelength" sentinel does not exist (id 0 is the first registered λ). Remove the dead `isnan` checks or introduce an explicit invalid id. detector.cpp:68 indexes `wavelengths_vector[0]` without a bounds check.
