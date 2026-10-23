# SPAR interpolation and delay options: discussion summary 2 (sprint 2026-10c)

Discussion between the maintainer and a discussion agent on 2026-10-23 (brief:
`discover/orchestration/2026-10c/PSPAR2-discussion.md`, branch `ws/spar-discuss2`) about the choices phase PF03c
took in the `SPAR` device (`backlog/sprint-2026-10c/PF03c-REPORT.md`, "Decisions I had to take", items 1, 3, 4
and 5) and about whether the per-output delay option should exist. Labels: (a) what the code does, (b) sources,
(c) measurements and derivations made for this discussion, (d) opinion. Nothing in the product code was changed.

Method (c): a standard-library Python copy of the tabulated interpolant (`SParamData::buildTables`,
`removeDelay`, `realImaginaryInterval`, `interpolate`, `leaveOneOutErrors`, `pchipSlopes`), checked bit for
bit against the C++ through a small driver linked with `src/io/sparam_file.cpp` (3001 evaluations per entry on
the ring file, the SiEPIC BDC and the `wg` fixtures, linear and PCHIP, and the same leave-one-out values); the
variants below are switches of that copy. Reference devices are closed forms. The grid-check change was also
built into a patched copy of `sparam_file.cpp` and run on every fixture. Timings use the gcc release build
(`pixi run test gcc`: 16 OK, 1 skipped, B-193). The prototype and scripts are scratch files outside the
repository and are not durable; the algorithms and the expected values are given here so that the follow-up can
reproduce them.

Metric: "error" is the worst field error |S_interp - S_exact| at the 7 interior points of every interval of the
grid; "LOO" is the grid check's leave-one-out value (`GRID_CHECK_THRESHOLD` 0.02). Reference devices:

| device | grid | notes |
| --- | --- | --- |
| unbalanced MZI (two 50/50 couplers, arms 300/400 um, ng 4.3, 2 dB/cm) | 1.54-1.56 um by 0.2 nm, 101 points, 28 per FSR | arm delays 4.30 / 5.74 ps, mean 5.020 ps; variants with arm amplitudes 1/0.9 and 1/0.8 (imperfect nulls, |S|min/max 0.053 and 0.11) |
| multi-path sums a_i exp(j 2 pi f tau_i) | 0.2 nm and 0.05 nm | 3 paths 0.5/0.4/0.3 at 3/5/8 ps; 2 paths 0.5/0.5 at 3/8 ps |
| add-drop ring of `circuits/add_drop.cir` (closed forms of `SPAR-discussion.md`) | 1.535-1.565 um by 0.05 nm (601) and by 1 nm (31) | k = 0.85; k = 0.4 as an under-resolved case |
| delay line (the `wg` fixtures, 1.40097 ps) | 1.5-1.6 um by 0.5 nm, 201 | exact for every scheme |
| dispersive line, group delay 5 +- 1.5 ps across the band, |S| 0.9 | same | no null, no delay removal interval |
| SiEPIC BDC (`tests/sparam/data/siepic`) | 1.5-1.6 um by 2 nm, 51 | no reference: LOO only |

## Decisions (maintainer, 2026-10-23)

| Topic | Decision |
| --- | --- |
| 1. tau0 of the delay-removed interpolation (B-190) | **Pulse-pair estimator** replaces the median of the finite-difference delays: tau0 = arg(sum_k S(k+1) conj S(k)) / (2 pi mean step), refined by one or two Newton steps for non-uniform grids; the 1e-3 rad cut and f_ref unchanged. |
| 2. Segment test | **Kept.** The grid check takes its re/im decision on the two actual intervals around the point (not the doubled one). |
| 3. PCHIP | **Magnitude/phase slopes per run between the re/im intervals** (today's scheme otherwise). PCHIP on the detrended phase ("Er") not adopted. Threshold 0.07 unchanged. **Linear stays the default.** |
| 4. Per-output delays | **`delay_per` removed** with its two golden cases, its three `test_sparam.py` cases and its documentation. `delay="auto"|"none"|<s>` unchanged ("auto": one delay per path). |
| 5. Keyword | Moot. If a per-output mode were ever reintroduced, fold it into `delay="path"|"output"|"none"|<s>`. |
| Backlog | Read INTERCONNECT's `.sparam` "option 3" header (a group delay per entry, phase stored with the delay removed), rejected today. |

## Topic 1: the delay estimate tau0

(a) `src/io/sparam_file.cpp`, `removeDelay()` (lines 459-485 at ebcacf3): tau0 is the median of `tau_s`, the
3-point finite-difference group delays of the unwrapped phase (lines 312-328), set to 0 when it turns S by less
than `DELAY_REMOVAL_MIN_ROTATION` (1e-3 rad) over the largest grid step; `f_ref` is the first grid point. It is
computed once per entry when the file is loaded (`buildTables()`), not per wavelength.

(c) tau0 (ps) and the linear error, per estimator. "Pulse-pair": arg(sum_k S(k+1) conj S(k)) / (2 pi df).
"Fit": least-squares slope of the unwrapped phase (what PF03c rejected).

| case | median | pulse-pair | \|S\|^2-weighted mean of tau_s | fit |
| --- | --- | --- | --- | --- |
| MZI (mean delay 5.020) | 5.018, 0.0016 | 5.018, 0.0016 | 5.011, 0.0016 | 4.28, 0.0033 |
| ring through 0.05 nm | 0.762, 5.4e-4 | 0.749, 5.5e-4 | 0.747, 5.5e-4 | 0 (cut), 9.7e-4 |
| ring drop 0.05 nm (no re/im interval) | 0.806, 4.0e-4 | 1.124, 4.0e-4 | 1.123, 4.0e-4 | 0.978, 4.0e-4 |
| MZI, 5 points centred on a null / null between the middle points | 4.69, 1.5e-3 / 4.95, 5.2e-4 | 4.96, 3.1e-4 / 4.97, 5.2e-4 | 4.16, 3.9e-3 / 4.51, 2.3e-3 | -0.90, 0.026 / -0.89, 0.056 |
| MZI, 3 points, same two layouts | -4.8, 0.07 / -4.8, 0.056 | -0.4, 0.024 / 4.63, 0.0017 | -3.1, 0.07 / 1.5, 0.056 | -4.8, 0.07 / -4.8, 0.056 |
| MZI, 7 and 11 points | identical errors for all but the fit | | | |
| MZI with complex noise -50 / -40 / -30 dB | 0.0085 / 0.027 / 0.094 for every estimator (the noise of the grid values sets the error); estimates within 0.03 ps of 5.02 except the fit | | | |
| 3 paths 0.5/0.4/0.3, 0.05 nm (resolved) | 4.81, 0.0054 | 4.55, 0.0054 | 4.56, 0.0054 | 4.99, 0.0054 |
| narrow band, 6 points, no null | all 5.019, 0.0016 | | | |

- The MZI's floor of 0.0016 is the linear interpolation of |S| at its maxima (h^2/8 of a cosine sampled 56
  times per period), not the nulls: a brute-force scan of tau0 finds nothing better. For the ring the best tau0
  (1.05 ps) gives 3.9e-4, the floor where |S| >= 0.1.
- The fit is biased to one arm's delay (4.30 ps) by the pi steps of the unwrapped phase at the nulls, as PF03c
  found.
- The median only fails when the points polluted by a null (3 per null) are at least half of the grid: 3 to 5
  points around a null, where the grid check warns (N = 3) or nearly (N = 5).
- Pulse-pair (derivation): sum_k |S~(k+1) - S~(k)|^2, the total variation of the delay-removed entry, equals
  sum (|S(k+1)|^2 + |S(k)|^2) - 2 Re(exp(-j 2 pi df tau) sum_k S(k+1) conj S(k)) on a uniform grid, so its
  minimum over tau is at tau0 = arg(sum_k S(k+1) conj S(k)) / (2 pi df). In the continuum limit this is the
  |S|^2-weighted mean of the group delay, and for a sum of delayed paths the power-weighted mean of their delays
  over whole beat periods. It needs no unwrapping and no stencil; a null's products are small vectors that barely
  turn the sum. On a non-uniform grid the closed form with the mean step is exact for uniform grids, within
  1e-3 ps on wavelength-uniform grids (ring file: 0.7493 vs 0.7492 ps refined, a rotation of 4e-5 rad per step),
  and one Newton step on g(tau) = sum_k |S(k) S(k+1)| sin(dphi_k - 2 pi df_k tau) df_k = 0 makes it exact on
  alternating steps (0.2/1.0 nm: 1.40018 then 1.40097 ps) and log-spaced grids. Limit: a step whose phase
  advance exceeds pi aliases, the same limit as the unwrapping; a gap wider than half a cycle of the delay (two
  bands 60 nm apart with a 1.4 ps line) gives 0.40 ps instead of 1.40 because the gap's product aliases, where
  the median loses only the four points around the gap: skip intervals whose step exceeds about 3 times the
  median step, or document it.
- Golden impact of the switch: the ring's `out` at the sweep points of `cir/spar-add-drop-ring`,
  `regress/spar-ring-tran` and `regress/spar-grid-warning` moves by about 1.7e-14 (the points are within 1e-9 of
  a grid point); between grid points the two interpolants differ by up to 7.2e-6 (ring through). The grid
  warning's printed value on the 1 nm file (0.707522 at 1.546 um) is unchanged to its printed digits (median
  tau0 -0.0145 ps there, pulse-pair 0.6235 ps). The gtest MZI numbers (0.0035, 0.0089) may change in their last
  digits.

(b) Kay, "A fast and accurate single frequency estimator", IEEE Trans. ASSP 37(12), 1989, doi
10.1109/29.45547 (weighted phase increments arg(x_i conj x_{i-1}), no unwrapping); the pulse-pair estimator of
Doppler radar (Miller and Rochwarger, IEEE Trans. Inf. Theory 18(5), 1972). No precedent was found for a median
of finite-difference delays (research agent and own check). Delay removal before interpolation or fitting:
INTERCONNECT's `.sparam` "option 3" header carries a group delay per entry "evaluated at the center frequency of
the S-parameter frequency data" and stores "the total phase angle minus the group delay phase change relative to
the center frequency", f_c = (f_1 + f_N)/2, else "the group delay will be double-counted in the simulation"
(optics.ansys.com help-center article 360036618513, read through its API); the HSPICE S-element with
`DELAYHANDLE=1` "accepts a constant group delay matrix", T(i,j) = d theta_Sij / d omega by finite differences at
`DELAYFREQ` (default the maximum frequency), removes exp(j omega T) from the transfer function and convolves with
v_n(t - T_kn) per path (HSPICE Signal Integrity User Guide A-2007.12, chapter 2, pp. 42-43). Keysight's port
extension fits a least-squares line to the unwrapped phase (US patent 7088087), the estimator that fails here.

(d) The pulse-pair is the better-defined estimator and never worse on the cases measured; the gain is confined
to files with 3 to 5 points around a null. Adopted by the maintainer.

## Topic 2: the segment test

(a) `realImaginaryInterval()` (lines 494-507): an interval is re/im when |S| at either end is below
`MIXED_INTERP_THRESHOLD` (0.07) times the entry's largest |S| (line 497), or when the chord between the two
delay-removed values passes within that distance of 0 with its foot inside the interval (lines 501-506).
`leaveOneOutErrors()` applies the same test to the doubled interval [j-1, j+1] (line 558).

(c)
- Between grid points it changes nothing on any resolved device (MZI, ring, delay line, dispersive line, BDC:
  identical with and without). It changes the LOO (MZI 0.114 without, 0.0064 with: the grid check then
  describes the interpolant) and under-resolved data (ring k = 0.4: 0.21 to 0.044 between points).
- Firing condition on a constant-|S| entry: the chord of a unit circle subtending r rad passes at distance
  cos(r/2) from the centre, so the test fires exactly when the delay-removed rotation per step exceeds
  pi - 2 asin(0.07) = 3.0015 rad (checked: 3.00 rad fires on no interval, 3.01 on all). Below that only an
  interval straddling a null between two larger points fires. On the actual grids it selects the same intervals
  as the endpoint rule (MZI 6-8 of 100, ring through 23 of 600, BDC and delay lines 0).
- Side effect: in the LOO the interval is doubled, so the test fires when the delay-removed rotation exceeds
  1.5 rad per grid step, i.e. when a path's group delay departs from tau0 by more than 1.5 / (2 pi 2 df): 3.8 ps
  at 0.5 nm, 0.95 ps at 2 nm (the BDC's finite-difference delays spread by 0.01 ps: safe), 19 ps at 0.1 nm. A
  dispersive line with group delay 1 +- 4 ps on a 0.5 nm grid (phase step 1.84 rad, error between points 0.0020:
  resolved) gets LOO 0.957 instead of 0.0080, a false grid warning; 1 +- 3.5 ps does not (0.0070).
- Fix measured: take the LOO's re/im decision on the two actual intervals, `realImaginaryInterval(t, j - 1, j)
  || realImaginaryInterval(t, j, j + 1)`. In the prototype: MZI 0.00636, ring through 0.00215, drop 0.00159,
  delay line 3.4e-13 unchanged; dispersive 1 +- 4 ps 0.957 to 0.0080; ring k = 0.4 0.429 to 0.153 (still a
  warning); ring 1 nm 0.708 unchanged. In a patched copy of the C++: identical worst values on every fixture,
  the ring file, the BDC and the golden 1 nm file with linear interpolation (`regress/spar-grid-warning` keeps
  0.707522 at 1.546 um); with PCHIP only the 1 nm file's worst value moves (0.7074 to 0.6657 at 1.546 um, still
  a warning); no golden case uses PCHIP. The check then asks whether the device would use re/im on either side
  of the point on the actual grid, rather than on the grid without the point; that is the question the warning
  is meant to answer.

(d) Sound between points; the one-line change in the grid check is worth making. Adopted.

## Topic 3: PCHIP, the threshold and the default interpolant

(a) `buildTables()` computes the PCHIP slopes of the magnitude and of the raw unwrapped phase over the whole
grid (lines 332-333) and of the delay-removed real and imaginary parts (334-335). Next to a pi step of the
phase the Fritsch-Carlson slope is the harmonic mean of the two secants, about twice the true slope when the
step's secant is large; a slope error e at one end of a cubic Hermite interval gives an error of
e h max|t^3 - 2 t^2 + t| = 0.148 e h; on the MZI (0.79 rad of phase per interval, |S| 0.1-0.2 next to the null)
that is about 0.017, PF03c's 0.0165.

(c) Error between points, then LOO. "Split": today's scheme with the magnitude/phase slope runs broken at the
re/im intervals, so bit-identical to today wherever an entry has none. "3-point": non-monotone cubic Hermite
with 3-point (parabolic) slopes, split the same way; exact for a quadratic phase.

| case | linear | PCHIP today ("Es") | PCHIP on the detrended phase ("Er") | split | 3-point, split |
| --- | --- | --- | --- | --- | --- |
| MZI | 0.0016 | 0.0165 | 0.0059 | 9.2e-4 | 1.5e-4 |
| MZI, arms 1/0.9 | 0.0076 | 0.0127 | 0.0057 | 0.0042 | 0.0039 |
| ring through | 5.4e-4 | 6.3e-4 | 3.0e-4 | 2.1e-4 | 1.2e-4 |
| ring drop | 4.0e-4 | 2.7e-4 | 2.7e-4 | 2.7e-4 | 7.3e-6 |
| 2 paths 3/8 ps, 0.05 nm | 1.2e-3 | 6.6e-3 | 5.6e-3 | 1.1e-3 | 6e-5 |
| 3 paths, 0.05 nm | 5.4e-3 | 3.8e-3 | 3.8e-3 | 3.8e-3 | 1.2e-3 |
| dispersive 5 +- 1.5 ps | 7.5e-4 | 6.1e-7 | 1.9e-4 | 6.1e-7 | 3e-13 |
| delay line | 3.8e-13 | 3.9e-13 | 3.9e-13 | 3.9e-13 | 3.9e-13 |
| BDC: LOO, and change against today between points | 2.2e-3 | 2.2e-3 | 2.2e-3, 2.7e-4 | 2.2e-3, 0 | 2.2e-3, 2e-5 |
| LOO: MZI / ring through / dispersive | 0.0064 / 0.0021 / 0.0030 | 0.042 / 0.0019 / 5.0e-4 | 0.011 / 0.0019 / 0.0026 | 0.0059 / 0.0019 / 5.0e-4 | 5.4e-4 / 3.7e-4 / 2.7e-13 |

- Er's loss on smooth dispersive data (6.1e-7 to 1.9e-4) comes from the monotonicity limiter flattening the
  extrema of the detrended phase; PCHIP is not invariant under adding a linear trend.
- With noise at -50 / -40 dB all schemes are within 10 percent of each other, except today's PCHIP on the MZI
  at -50 dB (0.024 against 0.0085).
- Overshoot: on these devices the split and 3-point magnitude cubics stay within the grid's range (the
  magnitude cubic on the mag/phase intervals never drops below the neighbouring grid values); the 3-point
  variant has no monotonicity guarantee on sharper features or noisy data, which is what PCHIP provides.
- Cost: the ring file (601 points, 16 entries, tables and grid check) loads in 10 ms with either interpolant;
  one evaluation is about ten flops per quantity instead of two, and S is computed once per wavelength per
  device. The split adds about 30 lines plus 15 to keep the grid check's reduced grid consistent (break its runs
  at the re/im intervals too).
- Threshold (error between points, linear then split PCHIP). With the delay removed, raising it from 0.07 helps
  imperfect nulls and costs PCHIP a factor 2 at the ring's null; the drop degrades only at 0.99 (4.0e-4 to
  5.5e-4) and the dispersive line only at 0.99 (0.046, re/im everywhere); the BDC's LOO is unchanged up to 0.5.
  A uniformly weak path (|S| 0.03 with a 5 ps delay) is unaffected (relative threshold). 0.07 was tuned before
  the delay removal (PF03b).

| threshold | 0.07 | 0.15 | 0.2 | 0.3 |
| --- | --- | --- | --- | --- |
| MZI, arms 1/0.9 | 0.0076, 0.0042 | 0.0038, 0.0018 | 0.0023, 9.1e-4 | 0.0015, 8.8e-4 |
| MZI, arms 1/0.8 | 0.013, 0.0084 | 0.0075, 0.0034 | 0.0039, 0.0015 | 0.0021, 8.3e-4 |
| ring through | 5.4e-4, 2.1e-4 | 5.4e-4, 4.2e-4 | 5.4e-4, 4.2e-4 | 5.4e-4, 4.2e-4 |
| 3 paths, 0.05 nm | 0.0054, 0.0038 | 0.0031, 0.0017 | 0.0020, 0.0017 | 0.0020, 8.5e-4 |
| LOO at 0.07 / 0.3, linear: MZI 1/0.9, 1/0.8, 3 paths | 0.020, 0.043, 0.021 / 0.0061, 0.0068, 0.0079 (the grids stop warning) | | | |

(b) Defaults elsewhere: HSPICE S-element `INTERPOLATION=LINEAR` (default; STEP, SPLINE, HYBRID) with
`INTDATTYP=MA` (default; RI, DBA) (SI User Guide A-2007.12); Keysight ADS `InterpMode` Linear with `InterpDom`
"Data Based", polar for S-parameters (ADS 2011.01 circuit components manual, via the research agent); scikit-rf
`Network.interpolate` linear in cartesian coordinates by default.

(d) Today's PCHIP is a trap on paths with nulls, ten times worse than linear on the MZI. The split removes the
trap and changes nothing on smooth data; Er is dominated by it. Linear as the default matches practice and is
safe; with the split, PCHIP is at least as good as linear on every case here, but changing the default would
move every SPAR golden value between grid points. Decision: split adopted, threshold unchanged, linear default.
The 3-point variant is recorded as the higher-accuracy, non-monotone alternative, not adopted.

## Topic 4: the per-output delay option

(a) `delay_per="path"|"output"` (commit 7e044eb, 333 insertions over 17 files): 76 lines of product code
(`src/devices/sparam_device.cpp` 47, `.h` 20, `src/parser/parse_element.cpp` 9; five branches on
`m_per_path_delays`, `sparam_device.cpp` lines 90, 135, 142, 187 and 241 at ebcacf3), three `test_sparam.py`
cases (per-output-tran, negative-delay/per-output, per-output-positive), the golden cases
`regress/spar-ring-tran-output` and `err/spar-bad-delay-per`, a `--vci` line, and documentation in
`doc/sparam.md`, `doc/netlist.md` and `CHANGELOG.md` (`[unreleased]`: `SPAR` has never been released).

(c) Cost re-measured: the ring file (0.05 nm), `in` and `add` lit with 200000 values each 0.25 ps apart (four
power levels, pseudo-random), `timescale=-15`, `.save out drop`, gcc release, median user time of 7 runs, at
1.55 um and 1.552 um:

| | per path | per output |
| --- | --- | --- |
| `delay=auto` | 1.24 / 1.34 s | 0.90 / 0.94 s |
| `delay=2e-12` | 0.92 / 0.94 s | 0.88 / 0.89 s |
| `delay="none"` | 0.85 / 0.87 s | same code path |

With automatic delays the per-path device emits 300074 / 300404 events on `out` against 187463 / 187524
(112611 / 112880 coalesced in the port's pending queue): two arrival times per input event, physically distinct,
which the per-output mode merges into one; the trace is 47 MB against 29 MB. With a fixed delay the events are
the same and the mechanism (device queue against port queue) costs 4-5 percent. Peak RSS 29-31 MB in every case.
PF03c measured 0.96-1.04 s against 0.91-0.94 s on a stimulus that repeated values more often; PF03b 1.02 against
1.63 s (unrecorded stimulus): the difference is the number of distinct arrival times, not the mechanism.

(b) HSPICE's S-element uses a delay matrix T(i,j), one per path; INTERCONNECT fits a digital filter per S entry
(`digital filter type` single tap | FIR | IIR, `fractional delay` true | false, `delay compensation` n; help-center
article 360036107914); Photontorch keeps one delay per port of a component (`Component.set_delays`,
"Tensor[#ports]"), the only per-port precedent found. No external results depend on the per-output behaviour.

(d) The speed argument is weak: `delay=<s>` already gives the single-arrival event count at the same cost. The
option's only unique feature is an automatic single delay per output taken from the dominant path, a cheaper and
less faithful transient (in the ring at its through null, `in`'s light leaves `out` with the drop path's delay).
Keeping it means every later `SPAR` change (phase 2 filters, warnings) must define and test two timing models.
Decision: removed.

## Topic 5: the keyword

(a) `SPAR` keywords are lowercase words with quoted string values (`format`, `convention`, `interp`,
`passivity`, `delay`); snake_case keywords exist elsewhere (`initial_state`, `insertion_loss`, `length_ref`);
0/1 flags only on `PROBE`. The code names auto/none/fixed the `DelayMode` (`src/devices/sparam_device.h`, line
33), so `delay_mode` would have named the other switch. The switch is void with `delay="none"` or a number
(same outputs, one delta cycle apart), yet `delay="none" delay_per="output"` was accepted. (b) HSPICE:
`DELAYHANDLE=1|0|ON|OFF`; INTERCONNECT: `fractional delay` true/false. (d) If a per-output mode were ever
reintroduced: fold it into `delay="path"|"output"|"none"|<s>` with `"auto"` as an alias of `"path"` (one keyword,
no void combination); against `delay_mode` (collides), `delays`, `delay_by="port"`, `timing`. Moot with the
removal: `delay="auto"|"none"|<s>` stays as it is, "auto" documented as one delay per path.

## Notes for the follow-up

Suggested order (each with its predicted golden impact):

1. **Remove `delay_per`**: device, header, parser, `--vci` line, the negative-delay warning's per-output text;
   delete `regress/spar-ring-tran-output` and `err/spar-bad-delay-per` (cases.json, baseline, netlists); remove
   the three `test_sparam.py` cases; `doc/sparam.md` "One delay per output", `doc/netlist.md` row and `.tran`
   sentence, `CHANGELOG.md`, `tests/golden/README.md` counts. Nothing else moves.
2. **Grid check on the actual intervals** (`leaveOneOutErrors()`, line 558, plus the comments at
   `src/io/sparam_file.h` lines 44-48 and `doc/sparam.md` "Grid check"): no golden change; re-run the
   `SParamFile` gtests.
3. **PCHIP slopes per run**: compute `dmag`/`dphase` per maximal run of consecutive magnitude/phase intervals
   (a run of two points gives the secant, a single point keeps no slope that is used); in the LOO, break the
   reduced grid's runs at its re/im intervals the same way. No golden case uses `interp=pchip`; the
   `test_sparam.py` PCHIP bound (2e-4 where |E| >= 0.1) still holds with a changed value; the gtest's PCHIP
   number (0.0127) changes. Expected: MZI 9.2e-4 between points, ring through 2.1e-4; dispersive line and BDC
   bit-identical.
4. **Pulse-pair tau0** in `removeDelay()`: closed form with the mean step, one or two Newton steps of
   g(tau) = sum_k |S(k) S(k+1)| sin(arg(S(k+1) conj S(k)) - 2 pi df_k tau) df_k, intervals whose step exceeds
   about 3 times the median step skipped; keep the 1e-3 rad cut and f_ref; the sign is positive for a delay in
   SPECS's convention (the `wg` fixtures give +1.40097 ps). Expected tau0: ring through 0.74919 ps (median
   0.7620), ring drop 1.1236 ps (0.8063), the MZI about 5.018 ps, the 1 nm ring file 0.6235 ps (through) and
   1.1406 ps (drop). Golden: `out` at the sweep points of `cir/spar-add-drop-ring`, `regress/spar-ring-tran` and
   `regress/spar-grid-warning` by about 1.7e-14 (strict re-record); the grid warning's text unchanged; gtest
   MZI numbers (0.0035, 0.0089) to update if their last digits move. Update `doc/sparam.md` "Interpolation", the
   comments at `src/io/sparam_file.h` lines 118-121 and in `removeDelay()`, and the "Fix" paragraph of
   `backlog/B-190-spar-interpolation-rotating-paths-with-nulls.md`.
5. Backlog item: accept the 7th header field of Lumerical `.sparam` blocks (group delay at the centre frequency
   f_c = (f_1 + f_N)/2) by adding 2 pi tau_g (f - f_c) back to the stored phase (rejected today as "not
   supported").

Validation: rebuild the reference prototype from this document (closed forms above; the devices' parameters are
in the table), check it bit for bit against the C++ before the changes, then predict each item's golden impact
before re-recording, as PF03c did. The thresholds are a second-order knob now: 0.15-0.3 would help imbalanced
interferometers (table above) at a factor 2 on the ring's null with PCHIP; left at 0.07.

## Sources

Read for this discussion: HSPICE Signal Integrity User Guide A-2007.12, chapter 2 (S-element syntax,
`DELAYHANDLE`, `INTERPOLATION`, `INTDATTYP`; "Group Delay Handler in Time Domain Analysis", pp. 42-43),
https://ece.iisc.ac.in/~dipanjan/E8_262/hspice_si.pdf; Lumerical INTERCONNECT "S-parameter file formats and
usage" and "Optical N Port S-Parameter" (optics.ansys.com help-center articles 360036618513 and 360036107914,
read through the API `https://optics.ansys.com/api/v2/help_center/en-us/articles/<id>.json`; the pages return
HTTP 403). Collected by a research agent and not re-read: Keysight ADS 2011.01 "Introduction to Circuit
Components" (`InterpMode`, `InterpDom`, `ExtrapMode`, `Imp*` options),
https://edadownload.software.keysight.com/eedl/ads/2011_01/pdf/ccsim.pdf; scikit-rf `Network.interpolate`,
https://scikit-rf.readthedocs.io/en/latest/api/generated/skrf.network.Network.interpolate.html; Photontorch
`photontorch/components/component.py` and `networks/network.py` (github.com/flaport/photontorch); Kay, IEEE
Trans. ASSP 37(12), 1989, https://doi.org/10.1109/29.45547; Miller and Rochwarger, IEEE Trans. Inf. Theory
18(5), 1972; De Tommasi and Gustavsen, IPST 2007, https://www.ipstconf.org/papers/Proc_IPST2007/07IPST025.pdf
(delay identified jointly with the rational fit); Keysight, "Comparing the PNA delay functions",
https://helpfiles.keysight.com/csg/e5080b/Tutorials/Comparing_the_PNA_Delay_Functions.htm; US patent 7088087
(port extension: least-squares line on the unwrapped phase). Not reachable or not verified: Cadence Spectre
`nport` reference (forum summaries only: `interp=linear|spline|bbspice`), Chinea/Triverio/Grivet-Talocia 2010 and
Lalgudi 2006 delay estimators, Kay's original weights (secondary source), ngspice/Xyce (no time-domain
S-parameter device: ngspice `xfer` is AC only, Xyce `YLIN` is harmonic-balance only).
