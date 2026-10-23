# S-parameter tests (F-03)

Test data and helpers for the `SPAR` element and `utils/sparam_export.py` (formats and conventions:
[doc/sparam.md](../../doc/sparam.md)). Standard library only; every script runs as `python3 -I <script>`.

## Fixtures (`fixtures/`)

Written by `gen_fixtures.py`; regenerate them (byte-identical output, floats written with `repr()`) with

    python3 -I tests/sparam/gen_fixtures.py            # or --out DIR

The script then reads every file back with `sparam_io.py` and checks it against the model (exit status 1 on a
mismatch), and prints the truncation error of the Taylor fixture and the finite-difference group delay.

Reference model: a 2-port waveguide section computed with the operations of SPECS's `WG`
(`src/devices/waveguide.cpp`, non-dispersive): L = 100 um, neff = 2.2, ng = 4.2, att = 10 dB/cm, so
`S21 = S12 = polar(10^(-0.1/20), 2 pi L neff(lambda)/lambda)` (SPECS convention, phase +beta L) and
`S11 = S22 = 0`. The phase is linear in frequency: the group delay is exactly `L ng / c` = 1.40097 ps.
Grid: `lambda_k = (3000 + k) / 2e9` (1.5 to 1.6 um by 0.5 nm, 201 points, each the double nearest to its decimal
value); frequency formats are written in ascending frequency, `f_k = c / lambda_k`.

| File | Format | Convention in the file | Notes |
| --- | --- | --- | --- |
| `wg.sparam` | Lumerical `.sparam` | physics (as SPECS) | port declarations `in`, `out`; all four blocks (the reflections as modulus 0); phases wrapped into (-pi, pi] (exercise the unwrapping) |
| `wg.s2p` | Touchstone 1, `# GHz S DB R 50` | engineering (conjugate) | `! Port[1] = in`; zero reflections written as -400 dB (DB cannot express 0) |
| `wg_v2.ts` | Touchstone 2.0, `# Hz S RI R 50` | engineering | `[Two-Port Data Order] 12_21`, `[Number of Frequencies] 201` |
| `wg.csv` | CSV | physics | only the two transmissions (absent pairs are 0) |
| `wg.taylor` | Taylor form | SPECS | center 1.55e-6, range 1.5e-6 1.6e-6, amp constant, phase of order 9 (truncation error 2.2e-12 rad over the range), no `delay` line |
| `gain.s2p` | Touchstone 1, `# Hz S MA R 50` | engineering | modulus of S21 and S12 1.05 (power 1.1025 > 1.03: passivity warning), waveguide phase |
| `asym.s2p` | Touchstone 1, `# MHz S RI R 50` | engineering | constant, 5 points (1.5 to 1.6 um by 25 nm), v1 order S11 S21 S12 S22 |
| `asym_v2.ts` | Touchstone 2.0, `# Hz S RI R 50` | engineering | same values, `[Two-Port Data Order] 21_12` |

The `asym` files hold, in the SPECS convention, S11 = 0.1, S21 = 0.8j, S12 = 0.3, S22 = -0.2 (S21 is
stored as `0 -0.8`): reading them with the wrong port order or without the conjugation gives different values.
Their ports are named `left` and `right`.

## Tests that use them

`test_sparam.py` (with the `SPAR` element):

- Phase and delay: each `wg.*` fixture through `SPAR` gives the transmission (modulus and phase) of a `WG` of
  the same length at the grid points and between them (the phase is linear in frequency, so the linear
  interpolation is exact), and the group delay `L ng / c`. The Touchstone files only match after the
  conjugation, `wg.sparam` only after unwrapping; `wg.taylor` checks the derived delay.
- Port order: `asym.s2p` and `asym_v2.ts` give the four values above.
- Passivity: `gain.s2p` warns with `passivity=check`, scales with `passivity=enforce`.

`sparam_io.py` is a minimal reader per format (`read_sparam`, `read_touchstone`, `read_csv`, `read_taylor`
and `eval_taylor`), written from doc/sparam.md independently of the writers: it returns the port names and
`{(out, in): [(f_Hz, S), ...]}` (1-based ports, S in the SPECS convention), and serves as the test oracle.

## Ring data (`data/add_drop_ring.sparam`)

The S-matrix of `add_drop_ring.cir` (below), used by the golden cases `cir/spar-add-drop-ring` and
`regress/spar-ring-tran` and by `test_sparam.py`, exported with the tool's default settings
(`abstol=1e-12 reltol=1e-9`) on a 0.05 nm grid, 601 points from 1.535 to 1.565 um:

    python3 -I utils/sparam_export.py --specs build/specs --netlist tests/sparam/add_drop_ring.cir --subckt ring \
        --wl 1.535e-6 1.565e-6 0.05e-9 --format sparam -o tests/sparam/data/add_drop_ring.sparam -j 8

`test_sparam.py` (`data/add-drop-ring-reproduced`) exports it again and compares. Until PF03b (2026-10-23) the file
had a 1 nm grid (31 points), which resolves neither the drop resonance's group delay (23 % low at resonance) nor
the through port near its null (SPAR-discussion.md in backlog/sprint-2026-10c). On the 0.05 nm grid the finite-difference
delays of the drop path match the exact ones within 0.01 ps, those of the through path from 0.5 nm off the null
(1.35 instead of 1.44 ps at 0.1 nm); the through-port null itself (its phase turns within about 4 pm) stays
unresolved (-37 ps instead of -621 ps), see doc/sparam.md.

Grid-adequacy check (doc/sparam.md): the 0.05 nm file **passes** it (worst leave-one-out error 0.0021, on the
through path next to a null, threshold 0.02; 0.0039 before the delay removal of B-190), although its through-port
null is narrower than the grid. With the mixed interpolation (real and imaginary parts of the delay-removed S near
nulls) the field between its grid points is within 6e-4 of the circuit everywhere, also at the null; what the grid does not resolve there is the phase feature, hence the group
delay (-37 ps), which the check does not test. With magnitude/phase interpolation the null would fail the check
(0.067). The former 1 nm file fails it (0.71); `regress/spar-grid-warning` shows the warning on a 1 nm export.

## SiEPIC BDC (`data/siepic/`)

`EBeam_1550_TE_BDC.sparam`: the S-parameters of the SiEPIC EBeam PDK 50/50 broadband directional coupler
(Lumerical `.sparam`, 4 ports, 51 points from 1.5 to 1.6 um), a third-party file copied unchanged; source,
commit, copyright and MIT licence in `data/siepic/NOTICE`. Used by the golden case `cir/spar-siepic-bdc` (`.dc`
sweep of the split) and by `test_sparam.py` (`siepic-bdc/split`: 0.502 and 0.490 at 1.55 um, within 0.02 of 0.5
over 1.54-1.56 um, passive). It passes the grid-adequacy check (worst 0.0024 with linear interpolation).

`check_ring_export.py --specs build/specs` checks `utils/sparam_export.py` on `add_drop_ring.cir` (the ring of
`circuits/add_drop.cir` as a subcircuit `ring in add out drop`): the export in every format reads back to the
same matrix (2e-16), and S[out][in], S[drop][in] match `circuits/add_drop.cir` without its access subcircuit
`x1` within 1.4e-12 with the same tolerances (7.2e-7 with the netlist's own reltol 1e-6).
