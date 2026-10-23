# SPAR delays: discussion summary (sprint 2026-10c)

Discussion between the maintainer and a discussion agent on 2026-10-23 (brief:
`discover/orchestration/2026-10c/PSPAR-discussion.md`, branch `ws/spar-discuss`) about two choices the
implementing agent of F-03 took without a maintainer decision (`backlog/sprint-2026-10c/PF03-REPORT.md`,
"Decisions I had to take", items 1 and 2). Labels: (a) what the code does, (b) sources, (c) measurements and
derivations made for this discussion, (d) opinion. Nothing in the product code was changed.

## Decisions (maintainer, 2026-10-23)

| Topic | Decision |
| --- | --- |
| One delay per output port | Replaced by **per-path delayed copies** (below). Steady state unchanged. `delay=none` and `delay=<s>` unchanged. |
| Negative group delay | **Keep clamping to 0**, now per path; make the warning more informative (below). Delay extraction (front delay + filter) belongs to phase 2. The whole-band phase slope and a smoothed slope are not adopted. |
| Group-delay estimate | Add a **grid-adequacy warning** at load time (criterion proposed below; the implementer fixes the details). |
| Sub-linewidth features (a ring's through-port null), broadband pulses | **Phase 2** of F-03, as in the brief. |
| Re-export of `tests/sparam/data/add_drop_ring.sparam` on a finer grid | Left open (its delays are wrong by up to 100 %, see below; three golden cases use it). |

## The test case

The add-drop ring of `circuits/add_drop.cir` (`tests/sparam/add_drop_ring.cir`, exported to
`tests/sparam/data/add_drop_ring.sparam` on a 1 nm grid, 31 points), with SPECS's own models (c):

| Quantity | Value |
| --- | --- |
| coupler `k=0.85` (field cross coupling): through field t = sqrt(1 - k^2) | 0.5268 |
| ring waveguides: 2 x 101 lambda0/neff, att 2 dB/cm, neff 2.3, ng 4.3 | round-trip field a = 0.99687, round trip tau_rt = 1.9526 ps |
| FSR | 4.104 nm |
| drop resonance FWHM | 1.98 nm (247 GHz) |
| through-port null: \|S\| at 1550 nm; width of its phase feature, 2(1-a)/(2 pi) FSR | 0.00228; 4.1 pm (0.51 GHz) |

Closed forms (h = field transmission of one half ring): S(out,in) = t (1 - h^2) / (1 - t^2 h^2);
S(out,add) = S(drop,in) = -k^2 h / (1 - t^2 h^2). The exact group delay is the derivative of their phase.
"FD" below is SPECS's estimate (`src/io/sparam_file.cpp`, `buildTables()`: unwrapped phase, 3-point finite
differences at the grid points, linear interpolation), on the committed 1 nm file and on a 0.05 nm export
(601 points) made with `utils/sparam_export.py` for this discussion.

| detuning from the null | \|S\| through | tau through exact | FD 1 nm | FD 0.05 nm | tau drop exact | FD 1 nm | FD 0.05 nm |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 0.002 | -621 ps | -0.68 ps | -37 ps | 1.72 ps | 1.32 ps | 1.72 ps |
| 0.1 nm | 0.11 | 1.44 ps | -0.61 ps | 1.35 ps | 1.70 ps | 1.29 ps | 1.70 ps |
| 0.5 nm | 0.48 | 1.32 ps | -0.35 ps | 1.32 ps | 1.33 ps | 1.16 ps | 1.33 ps |
| 1.0 nm | 0.71 | 0.85 ps | -0.014 ps | 0.85 ps | 0.86 ps | 0.99 ps | 0.86 ps |
| 2.05 nm | 0.82 | 0.55 ps | 0.65 ps | 0.55 ps | 0.55 ps | 0.65 ps | 0.55 ps |

Transients (c): `.tran` at 1 fs (`timescale=-15`, `abstol=1e-12 reltol=1e-6`), a 1 W step on `in` at 1 ps and
back to 0 at 20 ps, the explicit ring subcircuit against one `SPAR` on each file:

| case | explicit ring | SPAR, 1 nm file | SPAR, 0.05 nm file |
| --- | --- | --- | --- |
| 1550 nm, `out` | 0.527 (field) at the edge (direct coupler path), decays to 0.0023 in steps of one round trip, settled after ~10 ps | one step to 0.0023 at +1.32 ps | one step to 0.0023 at +1.72 ps |
| 1550 nm, `drop` | 0.72 at +0.98 ps (half round trip), 0.92 at +2.9 ps, 0.997 after ~10 ps; centroid of the rise ~1.70 ps | step to 0.997 at +1.32 ps | step at +1.72 ps |
| 1551 nm, both outputs | first arrival at +0 (`out`) and +0.98 ps (`drop`), settled after ~6 ps | both step at **+0 ps** (see below) | both step at +0.853 ps (exact 0.852/0.855) |
| 1550.1 nm, `in` and `add` both lit, `out` | builds up over ~10 ps to 0.995 | step at +1.29 ps to **0.896** (interpolation error of the 1 nm grid) | step at +1.70 ps to 0.995 |

Steady states of the SPAR agree with the explicit ring at every wavelength on the 0.05 nm file (1e-5) and at the
grid points of the 1 nm file.

## Question 1: one delay per output port, not per path

(a) `src/devices/sparam_device.cpp`, `computeEntry()`: per output and wavelength, the delay is the group delay
of the input with the largest |S| into that output, chosen among all inputs connected in the data, lit or not;
`onInput()` writes the full sum `sum_i S_ki a_i` to the output with that delay (absolute writes).
`src/optical_output_port.cpp`, `delayedWriteEventDriven()`: a write at a time and wavelength already pending
replaces the pending value, so writes to one output must be scheduled in time order; this is why the
implementing agent did not use a delay per path.

(b) Established practice is one delay per path with the sum formed at the output:

- HSPICE S-element, time domain: a constant delay matrix T(i,j) is extracted by finite differences of the
  phase of each S_ij at a target frequency, removed from each transfer function, and the port output is the sum
  over inputs n of the response of path (k,n) applied to the input n delayed by T(k,n). HSPICE Signal Integrity
  User Guide A-2007.12, chapter 2, "Group Delay Handler in Time Domain Analysis", pp. 42-43, eq. 7-9 (read).
- Photontorch: an instantaneous S per wavelength and one delay per port, applied as a buffer on that port
  (`photontorch/networks/network.py`, read): the closest published model to SPECS's phase 1.
- INTERCONNECT fits an FIR or IIR per S_ij for time domain and its `.sparam` format carries a group delay per
  data block; vendor pages (optics.ansys.com) were not reachable (HTTP 403), so this is second-hand.
- RF macromodelling separates a delay per path (or per group of paths) from a rational part: Lalgudi et al.,
  EPEP 2006 (minimum-phase part plus a per-path delay); Chinea, Triverio, Grivet-Talocia, "delayed vector
  fitting", IEEE TADVP 33(1), 2010; Gustavsen, IPST 2007 (delay from the phase minus the minimum-phase angle).

(c) Error of the per-output choice:

- Steady state: exact.
- One lit input: exact only when the lit path has the largest |S| into the output. Otherwise the signal is
  shifted by the delay difference, with no steady-state error. In the ring at 1550 nm the through signal leaves
  `out` 1.3 to 1.7 ps late (the unlit add-to-out path wins). At 1551 nm on the 1 nm file both outputs leave with
  zero delay: the unlit add-to-drop path wins on |S| (0.7096 vs 0.7027) and its finite-difference delay is the
  -14 fs artefact of Question 2.
- Two coherent lit inputs: the minor component is shifted by the delay difference. For an envelope with rise
  time T the field error during the edge is about |S_ki E_i| x (delay difference)/T; for pulses shorter than
  the delay difference the superposition is qualitatively wrong. In the ring the two paths into `out` differ
  by less than 50 fs wherever the through path carries more than half the field, by 0.26 ps at 0.1 nm from
  the null (through |S| = 0.11), and without bound at the null, where the through path's own delay is not
  meaningful (Question 2). The 1 nm file's interpolation error at 0.1 nm (0.896 instead of 0.995) is far larger
  than any delay effect.

(c, d) Per-path delayed copies, the adopted alternative: on an input event at time t with value a_i, for each
output k reached from i, schedule an internal arrival at t + tau_ki; on arrival set the delayed copy m_ki = a_i
and write `sum_i S_ki m_ki` to output k with zero delay. Writes to one output are then in time order, so no
stale value can remain. Steady state is identical (all copies reach their final values); only transients
change. Cost: one extra timed stage per path, about twice today's queue operations per input event, plus the
in-flight entries per path in memory. This is the structure SPECS already uses for an MZI (delaying arms, an
instantaneous combiner). Choosing the dominant path among lit inputs instead would make the per-output delay
vary in time and bring the stale-write hazard back.

## Question 2: negative group delays become 0

(a) `src/devices/sparam_device.cpp`, `autoDelay()`: a negative delay is replaced by 0, reported once per
device in `.tran` when its magnitude is at least half a time step. `src/io/sparam_file.cpp`,
`buildTables()` and `groupDelay()`: 3-point finite differences of the unwrapped phase at the grid points,
linearly interpolated.

(b) Physics. A passive, causal, linear device can have a negative group delay; a smooth pulse whose spectrum
fits inside the anomalous region leaves with its peak advanced, while its front is never advanced (Garrett &
McCumber, Phys. Rev. A 1, 305 (1970); Chu & Wong, Phys. Rev. Lett. 48, 738 (1982); Boyd & Gauthier, Science
326, 1074 (2009); Kitano, Nakanishi & Sugiyama, IEEE JSTQE 9, 43 (2003)). For rings, Heebner & Boyd, J. Mod.
Opt. 49, 2629 (2002) (read): the undercoupled through port "displays the reversal of sign near resonance", a
pulse "will emerge with its centre advanced in time", and "Causality is maintained because the discrete impulse
response of the resonator does not possess any advanced impulses." The lossy symmetric add-drop through port is
minimum phase (c): its zero and pole both lie inside the unit circle, the phase swings by about pi across the
null and returns, and the net phase per FSR is zero.

(c) Measurements.

- The exact delay at this null is -621 ps and describes only pulses longer than about 2 ns, which fit inside
  the 4 pm feature. For anything shorter the output is a reshaped spike (the explicit ring: 0.527 at the edge,
  decaying over ~10 ps), which no single delay represents; the carrier-only magnitude (0.0023) is wrong there
  too. So at the null the delay choice is moot: this is phase 2.
- The -14.5 fs quoted in the PF03 report is not physics. It is the 3-point stencil of the 1 nm grid straddling
  the null at 1549 and 1551 nm. The exact value there is +852 fs; the 0.05 nm file gives +853 fs. The 1 nm grid
  also puts the drop delay at resonance 23 % low. The file satisfies the documented rule "less than pi between
  points" and still gives delays wrong by 100 %.

(c, d) The substitute for a negative delay: a steep phase feature of spectral width df has a slope of about
1/(2 df) in magnitude for a pi swing. A signal sees that slope as a clean delay only if its spectrum fits
inside df, so its duration is at least about 1/df, about twice the delay. Whenever the first-order model is
valid, the advance is therefore at most a fraction of the pulse duration, and replacing it by 0 makes the output
late by less than the pulse duration; any larger substitute is later still. Hence:

| option | what it does | verdict |
| --- | --- | --- |
| clamp to 0 (current) | emits at the input edge; for a signal the model describes, the smallest possible timing error; for a bare ring through port it is the physical front (direct coupler path) | **kept** |
| whole-band phase slope as fallback | a positive "typical" delay from the overall phase trend; only helps for signals broader than the feature, whose amplitude the carrier-only model already gets wrong; depends on the exported range | not adopted |
| minimum-phase delay extraction | a front delay plus a filter that does the reshaping; meaningful only with the filter | phase 2 |
| smoothed or aperture-averaged slope | a tool against measurement noise; cannot resolve a feature the grid does not resolve (-0.68 ps, -37 ps, -621 ps at three resolutions) | not adopted; the grid warning covers the need |

Refinements of the warning (d): name the path (port names) and the wavelength; say when the value lies next to
a grid point flagged by the adequacy check, so the user knows it is probably a grid artefact; say that a
clamped path behaves like a zero-length waveguide inside a feedback loop (the rounding check treats a
non-positive delay as fine, so nothing else warns). With per-path delays the clamp affects only the negative
path, not the whole output.

## Grid-adequacy warning (proposed criterion, d)

A threshold on the phase step alone would trip on a legitimate 1.4 ps waveguide at 1 nm spacing (1.1 rad per
point), and the "less than pi" rule passes the 1 nm ring file. A leave-one-out check measures what matters: at
each interior grid point, interpolate S from its two neighbours exactly as the device does between points
(magnitude and unwrapped phase, linear or PCHIP), and compare with the stored value. A smooth delay line passes
exactly. On the 1 nm ring file the null point errs by 0.71 in field, its neighbours by about 0.3, and the drop
resonance by 0.29 (the point whose delay is 23 % low). On the 0.05 nm file the null still errs by 0.05, which
is honest: the feature is unresolved there too. One pass at load time, one warning per device naming the worst
path and wavelength, threshold around 0.01 to 0.02 in field; not applicable to the Taylor form.

## Background: interpolation near nulls

Magnitude and phase are interpolated separately (`doc/sparam.md`, "Interpolation"). On the 1 nm file the
steady-state field at 0.1 nm from resonance is 0.896 instead of 0.995; the PF03 report measured 0.02 at the
null on a 0.05 nm grid. Not discussed further; the adequacy warning gives users a signal, and the choice of
interpolant remains the open numerics item of the PF03 report ("Found, not done").

## Notes for the follow-up item

- Per-path delayed copies: one internal timed queue per device (as the output port's pending mechanism) or one
  internal delayed port per path; no emission gate on the internal stage (the output port gates); same-time
  arrivals coalesce at the output port. The dominant-path selection and the per-output `delay` vector go away.
  `delay=<s>` applies per path; rounding warnings stay once per device.
- Expected golden impact: `regress/spar-ring-tran` (the through signal leaves at +0, the drop at its own
  delay); `cir/spar-touchstone-tran` unchanged (one path per output); `.dc` cases unchanged;
  `regress/spar-3port-taylor` to be checked (explicit delays).
- Tests: a two-input transient against an explicit circuit with paths of different delays (an MZI or the
  ring), and a loader test for the adequacy warning (a null narrower than the grid; a delay line that must pass).
- Documentation: `doc/sparam.md` ("Behaviour in the simulation", delay) and `doc/netlist.md` (SPAR `delay`),
  `CHANGELOG.md`.

## Sources

Read for this discussion: HSPICE Signal Integrity User Guide A-2007.12 (ch. 2, pp. 42-43); Photontorch
`photontorch/networks/network.py` (github.com/flaport/photontorch); Heebner & Boyd, J. Mod. Opt. 49, 2629
(2002), https://doi.org/10.1080/0950034021000011527. Cited from abstracts or secondary reading: Garrett &
McCumber, https://doi.org/10.1103/PhysRevA.1.305; Chu & Wong, https://doi.org/10.1103/PhysRevLett.48.738; Boyd &
Gauthier, https://doi.org/10.1126/science.1170885; Kitano et al., https://arxiv.org/abs/quant-ph/0302166;
Gustavsen, IPST 2007, https://www.ipstconf.org/papers/Proc_IPST2007/07IPST025.pdf; Lalgudi et al., EPEP 2006,
https://epsilon.ece.gatech.edu/publications/2006/04115367.pdf; Chinea, Triverio & Grivet-Talocia, IEEE TADVP
33(1), 2010; Keysight, "Comparing the PNA delay functions" (group-delay aperture),
https://helpfiles.keysight.com/csg/N52xxB/Tutorials/Comparing_the_PNA_Delay_Functions.htm; Laporte et al.,
Sci. Rep. 9, 5918 (2019), https://doi.org/10.1038/s41598-019-42408-2. Not reachable (HTTP 403): the
INTERCONNECT "Optical N Port S-Parameter" and "S-parameter file formats" pages on optics.ansys.com.
