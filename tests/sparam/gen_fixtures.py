#!/usr/bin/env python3
"""Writes the S-parameter test fixtures of tests/sparam/README.md (standard library only).

    python3 -I tests/sparam/gen_fixtures.py [--out DIR]       (default DIR: fixtures/ next to this script)

The output is deterministic (floats written with repr(), the shortest string that reads back to the same
double): running it again gives byte-identical files. After writing, every file is read back with the
independent readers of sparam_io.py and compared with the reference model (exit status 1 on a mismatch).

Reference model: a 2-port waveguide section computed exactly like SPECS's WG (src/devices/waveguide.cpp,
non-dispersive branch), L = 100 um, neff = 2.2, ng = 4.2, att = 10 dB/cm:
    S21 = S12 = polar(10**(-att*L_cm/20), 2*pi*L_cm*1e-2 * neff_l/lam),
    neff_l = neff + (neff - ng)/1.55e-6 * (lam - 1.55e-6),     S11 = S22 = 0.
Its phase is +beta*L (SPECS convention) and is linear in frequency: phase = omega*L*ng/c + constant, so the
group delay dphi/domega is exactly L*ng/c.

Wavelength grid: lam_k = (3000 + k) / 2e9 (= 1.5e-6 + k*0.5e-9), k = 0..200. The numerator is an exact
integer and the division by 2e9 (exact) is one correctly rounded operation, so every lam_k is the double
nearest to its decimal value (1.53e-6 is the double that float('1.53e-6') gives). Frequencies are
f_k = c/lam_k (c = 299792458 m/s), written in ascending frequency in every frequency-based format.
"""

import argparse
import cmath
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import sparam_io  # noqa: E402  (test oracle, independent of the writers below)

C0 = 299792458.0

# Waveguide parameters (SI, as in a netlist)
L_M = 100e-6
NEFF = 2.2
NG = 4.2
ATT_DB_CM = 10.0
LAM0 = 1.55e-6          # SPECS's reference wavelength of the neff expansion

GRID = [(3000 + k) / 2e9 for k in range(201)]           # 1.5e-6 .. 1.6e-6, step 0.5 nm
ASYM_GRID = [(3000 + 50 * k) / 2e9 for k in range(5)]   # 1.5e-6 .. 1.6e-6, step 25 nm

# Constant entries of the asymmetric 2-port, SPECS convention: {(out, in): S}
ASYM = {(1, 1): 0.1 + 0j, (2, 1): 0.8j, (1, 2): 0.3 + 0j, (2, 2): -0.2 + 0j}

GAIN_MAG = 1.05         # |S21| = |S12|: power 1.1025 > 1.03 (passivity check)

TAYLOR_ORDER = 9
TAYLOR_RANGE = (1.5e-6, 1.6e-6)


def r(x):
    return repr(float(x) + 0.0)  # + 0.0: -0.0 (conjugate of 0) is written 0.0


def wg_phase(lam):
    """SPECS WG transmission phase (rad, not wrapped) at lam (m), same operations as waveguide.cpp."""
    length_cm = L_M * 100
    phase_factor = 2.0 * math.pi * length_cm * 1e-2
    dneff = (NEFF - NG) / 1.55e-6
    neff = NEFF + dneff * (lam - 1.55e-6)
    return phase_factor * neff / lam


def wg_transmission(lam):
    """SPECS WG field transmission at lam (m)."""
    t = math.pow(10.0, -(ATT_DB_CM * (L_M * 100)) / 20.0)
    return cmath.rect(t, wg_phase(lam))


def wg_matrix(lam):
    s = wg_transmission(lam)
    return {(1, 1): 0j, (2, 1): s, (1, 2): s, (2, 2): 0j}


def gain_matrix(lam):
    s = wg_transmission(lam)
    return {(1, 1): 0j, (2, 1): cmath.rect(GAIN_MAG, cmath.phase(s)), (1, 2): cmath.rect(GAIN_MAG, cmath.phase(s)),
            (2, 2): 0j}


def wrap_rad(phi):
    """phi wrapped into (-pi, pi]."""
    w = math.remainder(phi, 2 * math.pi)
    return math.pi if w <= -math.pi else w


def wrap_deg(deg):
    w = math.remainder(deg, 360.0)
    return 180.0 if w <= -180.0 else w


def by_freq(grid):
    """[(f_Hz, lam)] in ascending frequency."""
    return [(C0 / lam, lam) for lam in sorted(grid, reverse=True)]


# ---------------------------------------------------------------------------------------------------------
# Writers (one per fixture; they do not share code with utils/sparam_export.py)


def write_sparam(path):
    """Lumerical .sparam, physics convention (= SPECS), phases wrapped into (-pi, pi], all 4 blocks."""
    names = ['in', 'out']
    out = ['["in","LEFT"]', '["out","RIGHT"]']
    pts = by_freq(GRID)
    for (j, i) in [(1, 1), (2, 1), (1, 2), (2, 2)]:
        out.append('("%s","TE",1,"%s",1,"transmission")' % (names[j - 1], names[i - 1]))
        out.append('(%d,3)' % len(pts))
        for f, lam in pts:
            s = wg_matrix(lam)[(j, i)]
            out.append('%s %s %s' % (r(f), r(abs(s)), r(wrap_rad(cmath.phase(s)) if s != 0 else 0.0)))
    return out


def write_s2p_db(path):
    """Touchstone 1, GHz, DB/degrees, engineering convention (conjugate of SPECS)."""
    out = ['! SPECS test fixture (tests/sparam/gen_fixtures.py): 100 um waveguide, neff 2.2, ng 4.2, 10 dB/cm',
           '! Engineering convention: values are the complex conjugates of the SPECS ones',
           '! Zero reflections are written as -400 dB (|S| = 1e-20): DB cannot express 0',
           '! Port[1] = in',
           '! Port[2] = out',
           '# GHz S DB R 50']
    for f, lam in by_freq(GRID):
        m = wg_matrix(lam)
        vals = []
        for key in [(1, 1), (2, 1), (1, 2), (2, 2)]:
            s = m[key].conjugate()
            if s == 0:
                vals += [r(-400.0), r(0.0)]
            else:
                vals += [r(20 * math.log10(abs(s))), r(wrap_deg(math.degrees(cmath.phase(s))))]
        out.append(' '.join([r(f / 1e9)] + vals))
    return out


def write_ts_v2(path):
    """Touchstone 2.0, Hz, RI, [Two-Port Data Order] 12_21, engineering convention."""
    pts = by_freq(GRID)
    out = ['! SPECS test fixture (tests/sparam/gen_fixtures.py): 100 um waveguide, neff 2.2, ng 4.2, 10 dB/cm',
           '! Engineering convention: values are the complex conjugates of the SPECS ones',
           '! Port[1] = in',
           '! Port[2] = out',
           '[Version] 2.0',
           '# Hz S RI R 50',
           '[Number of Ports] 2',
           '[Two-Port Data Order] 12_21',
           '[Number of Frequencies] %d' % len(pts),
           '[Network Data]']
    for f, lam in pts:
        m = wg_matrix(lam)
        vals = []
        for key in [(1, 1), (1, 2), (2, 1), (2, 2)]:
            s = m[key].conjugate()
            vals += [r(s.real), r(s.imag)]
        out.append(' '.join([r(f)] + vals))
    out.append('[End]')
    return out


def write_csv(path):
    """CSV of doc/sparam.md, physics convention; only the two non-zero pairs (the others are 0 by absence)."""
    out = ['wavelength_m,"in@0,out@0_re","in@0,out@0_im","out@0,in@0_re","out@0,in@0_im"']
    for lam in GRID:
        m = wg_matrix(lam)
        a, b = m[(2, 1)], m[(1, 2)]
        out.append(','.join([r(lam), r(a.real), r(a.imag), r(b.real), r(b.imag)]))
    return out


def taylor_coefficients():
    """Phase polynomial in d = lam - LAM0 of the exact phase 2*pi*L*(ng/lam + (neff - ng)/LAM0).

    ng/lam = (ng/LAM0) * sum_n (-d/LAM0)^n, so c0 = 2*pi*L*(ng + neff - ng)/LAM0 and
    c_n = 2*pi*L*(ng/LAM0)*(-1/LAM0)^n for n >= 1."""
    a = 2.0 * math.pi * (L_M * 100) * 1e-2
    c = [a * (NG / LAM0 + (NEFF - NG) / LAM0)]
    for n in range(1, TAYLOR_ORDER + 1):
        c.append(a * (NG / LAM0) * (-1.0 / LAM0) ** n)
    return c


def taylor_truncation():
    """(analytic bound, max float difference) over the range of the truncated phase vs the exact phase."""
    a = 2.0 * math.pi * (L_M * 100) * 1e-2
    bound = 0.0
    for lam in TAYLOR_RANGE:
        x = (lam - LAM0) / LAM0
        # exact remainder of the geometric series: (-x)^(N+1) / (1 + x)
        bound = max(bound, abs(a * NG / LAM0 * x ** (TAYLOR_ORDER + 1) / (1 + x)))
    c = taylor_coefficients()
    worst = 0.0
    for k in range(2001):
        lam = TAYLOR_RANGE[0] + k * (TAYLOR_RANGE[1] - TAYLOR_RANGE[0]) / 2000
        d = lam - LAM0
        poly = 0.0
        for ck in reversed(c):
            poly = poly * d + ck
        worst = max(worst, abs(poly - wg_phase(lam)))
    return bound, worst


def write_taylor(path):
    t = math.pow(10.0, -(ATT_DB_CM * L_M * 100) / 20.0)
    c = ' '.join(r(x) for x in taylor_coefficients())
    return ['specs-taylor 1',
            '# SPECS test fixture (tests/sparam/gen_fixtures.py): 100 um waveguide, neff 2.2, ng 4.2, 10 dB/cm',
            '# phase = 2*pi*L*(ng/lambda + (neff - ng)/1.55e-6) expanded to order %d in d = lambda - 1.55e-6;' % TAYLOR_ORDER,
            '# no delay line: the group delay (L*ng/c) is derived from the phase',
            'ports in out',
            'center ' + r(LAM0),
            'range %s %s' % (r(TAYLOR_RANGE[0]), r(TAYLOR_RANGE[1])),
            'S out in amp ' + r(t),
            'S out in phase ' + c,
            'S in out amp ' + r(t),
            'S in out phase ' + c]


def write_gain(path):
    """Touchstone 1, Hz, MA, |S21| = |S12| = 1.05 with the waveguide's phase, engineering convention."""
    out = ['! SPECS test fixture (tests/sparam/gen_fixtures.py): active 2-port, |S21| = |S12| = %s' % r(GAIN_MAG),
           '! (power %s > 1.03: fails the passivity check); phase of the 100 um waveguide, conjugated' % r(GAIN_MAG ** 2),
           '! Port[1] = in',
           '! Port[2] = out',
           '# Hz S MA R 50']
    for f, lam in by_freq(GRID):
        m = gain_matrix(lam)
        vals = []
        for key in [(1, 1), (2, 1), (1, 2), (2, 2)]:
            s = m[key].conjugate()
            vals += [r(abs(s)), r(wrap_deg(math.degrees(cmath.phase(s))) if s != 0 else 0.0)]
        out.append(' '.join([r(f)] + vals))
    return out


ASYM_COMMENT = ['! SPECS test fixture (tests/sparam/gen_fixtures.py): constant 2-port with 4 different entries.',
                '! SPECS-convention values: S11 = 0.1, S21 = 0.8j, S12 = 0.3, S22 = -0.2',
                '! (written conjugated, engineering convention: S21 is stored as 0 -0.8).']


def write_asym_v1(path):
    out = ASYM_COMMENT + ['! Version 1 order: S11 S21 S12 S22', '! Port[1] = left', '! Port[2] = right',
                          '# MHz S RI R 50']
    for f, _ in by_freq(ASYM_GRID):
        vals = []
        for key in [(1, 1), (2, 1), (1, 2), (2, 2)]:
            s = ASYM[key].conjugate()
            vals += [r(s.real), r(s.imag)]
        out.append(' '.join([r(f / 1e6)] + vals))
    return out


def write_asym_v2(path):
    pts = by_freq(ASYM_GRID)
    out = ASYM_COMMENT + ['! [Two-Port Data Order] 21_12: S11 S21 S12 S22', '! Port[1] = left', '! Port[2] = right',
                          '[Version] 2.0',
                          '# Hz S RI R 50',
                          '[Number of Ports] 2',
                          '[Two-Port Data Order] 21_12',
                          '[Number of Frequencies] %d' % len(pts),
                          '[Network Data]']
    for f, _ in pts:
        vals = []
        for key in [(1, 1), (2, 1), (1, 2), (2, 2)]:
            s = ASYM[key].conjugate()
            vals += [r(s.real), r(s.imag)]
        out.append(' '.join([r(f)] + vals))
    out.append('[End]')
    return out


FIXTURES = [
    ('wg.sparam', write_sparam),
    ('wg.s2p', write_s2p_db),
    ('wg_v2.ts', write_ts_v2),
    ('wg.csv', write_csv),
    ('wg.taylor', write_taylor),
    ('gain.s2p', write_gain),
    ('asym.s2p', write_asym_v1),
    ('asym_v2.ts', write_asym_v2),
]

# ---------------------------------------------------------------------------------------------------------
# Read-back check


def check_tabulated(name, path, model, grid, ports, tol=1e-12):
    names, data = {'.sparam': sparam_io.read_sparam, '.csv': sparam_io.read_csv}.get(
        os.path.splitext(path)[1], sparam_io.read_touchstone)(path)
    errors = []
    if names != ports:
        errors.append('%s: ports %s, expected %s' % (name, names, ports))
    pts = by_freq(grid)
    worst = 0.0
    for key in [(1, 1), (2, 1), (1, 2), (2, 2)]:
        rows = data.get(key)
        if rows is None:
            rows = [(f, 0j) for f, _ in pts]   # absent pair = 0
        if len(rows) != len(pts):
            errors.append('%s: S%d%d has %d points' % (name, key[0], key[1], len(rows)))
            continue
        for (f, s), (fref, lam) in zip(rows, pts):
            if abs(f - fref) > 1e-15 * fref:
                errors.append('%s: S%d%d frequency %r, expected %r' % (name, key[0], key[1], f, fref))
                break
            worst = max(worst, abs(s - model(lam)[key]))
    if worst > tol:
        errors.append('%s: max |S - reference| = %g > %g' % (name, worst, tol))
    return worst, errors


def check_taylor(path, tol):
    t = sparam_io.read_taylor(path)
    errors = []
    worst = 0.0
    if t[0] != ['in', 'out'] or t[1] != LAM0 or t[2] != TAYLOR_RANGE:
        errors.append('wg.taylor: header %r' % (t[:3],))
    for lam in GRID:
        got = sparam_io.eval_taylor(t, lam)
        ref = wg_matrix(lam)
        for key in [(1, 1), (2, 1), (1, 2), (2, 2)]:
            worst = max(worst, abs(got.get(key, 0j) - ref[key]))
    if worst > tol:
        errors.append('wg.taylor: max |S - reference| = %g > %g' % (worst, tol))
    return worst, errors


def group_delay_check(path):
    """Max relative deviation from L*ng/c of the finite-difference group delay of the unwrapped S21 phase."""
    _, data = sparam_io.read_sparam(path)
    rows = data[(2, 1)]
    phases = [cmath.phase(s) for _, s in rows]
    unwrapped = [phases[0]]
    for p in phases[1:]:
        d = math.remainder(p - unwrapped[-1], 2 * math.pi)
        unwrapped.append(unwrapped[-1] + d)
    tau = L_M * NG / C0
    worst = 0.0
    for k in range(1, len(rows)):
        dw = 2 * math.pi * (rows[k][0] - rows[k - 1][0])
        worst = max(worst, abs((unwrapped[k] - unwrapped[k - 1]) / dw - tau) / tau)
    return tau, worst


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--out', default=os.path.join(HERE, 'fixtures'), help='output directory (default: %(default)s)')
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)
    for name, writer in FIXTURES:
        path = os.path.join(args.out, name)
        lines = writer(path)
        with open(path, 'w', newline='\n') as f:
            f.write('\n'.join(lines) + '\n')
        print('wrote', path)

    bound, worst_float = taylor_truncation()
    print('taylor: order %d, truncation error over [%g, %g] m: %.3g rad (analytic), %.3g rad (float evaluation)'
          % (TAYLOR_ORDER, TAYLOR_RANGE[0], TAYLOR_RANGE[1], bound, worst_float))

    errors = []
    if max(bound, worst_float) >= 1e-9:
        errors.append('taylor: truncation error >= 1e-9 rad')
    p = lambda n: os.path.join(args.out, n)  # noqa: E731
    for name, model, grid, ports in [('wg.sparam', wg_matrix, GRID, ['in', 'out']),
                                     ('wg.s2p', wg_matrix, GRID, ['in', 'out']),
                                     ('wg_v2.ts', wg_matrix, GRID, ['in', 'out']),
                                     ('wg.csv', wg_matrix, GRID, ['in', 'out']),
                                     ('gain.s2p', gain_matrix, GRID, ['in', 'out']),
                                     ('asym.s2p', lambda lam: ASYM, ASYM_GRID, ['left', 'right']),
                                     ('asym_v2.ts', lambda lam: ASYM, ASYM_GRID, ['left', 'right'])]:
        w, e = check_tabulated(name, p(name), model, grid, ports)
        errors += e
        print('read back %-11s max |S - reference| = %.3g' % (name, w))
    w, e = check_taylor(p('wg.taylor'), 1e-9)
    errors += e
    print('read back %-11s max |S - reference| = %.3g (tolerance 1e-9: truncation)' % ('wg.taylor', w))
    tau, dev = group_delay_check(p('wg.sparam'))
    print('group delay of wg.sparam (finite differences): L*ng/c = %.6g s, max relative deviation %.3g' % (tau, dev))
    if dev > 1e-6:
        errors.append('group delay deviates by %g' % dev)
    for e in errors:
        print('ERROR:', e, file=sys.stderr)
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
