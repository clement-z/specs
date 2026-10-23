#!/usr/bin/env python3
"""End-to-end tests of the S-parameter device SPAR (F-03, doc/sparam.md). Standard library only.

    python3 -I tests/sparam/test_sparam.py --specs build/specs [--keep DIR] [--only REGEX]

Checks (each prints PASS/FAIL with its measured numbers; exit 1 if one fails):
- fixtures: tests/sparam/gen_fixtures.py rewrites tests/sparam/fixtures byte for byte, and exporting the
  ring again gives tests/sparam/data/add_drop_ring.sparam (1e-12);
- phase convention, .dc: each wg.* fixture (a 100 um waveguide written in each format's own convention)
  through SPAR gives the field of a WG of the same length, at the grid points and between them; read
  with the opposite convention, the Touchstone fixtures do not;
- phase convention, .tran: the same pulses leave SPAR and WG at the same times with the same fields
  (group delay L*ng/c, rounded to the time step like the waveguide's);
- bidirectional: two sources on both sides of SPAR (nets with two writers) as on both sides of a WG;
- per-path delays, .tran: two lit inputs whose paths into each output have different delays (two waveguides
  into a coupler) give the transient of the explicit circuit;
- port order: the asymmetric Touchstone 1 and 2 fixtures give S21 and, with ports= reversed, S12;
- passivity: check warns, ignore does not, enforce scales the matrix;
- range: a wavelength outside the data is an error with the range;
- round trip: circuits/add_drop.cir's ring exported with utils/sparam_export.py, loaded as one SPAR, swept
  again: the same fields at the grid points (1e-6), and between the points of a finer export.
"""

import argparse
import cmath
import filecmp
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
FIXTURES = os.path.join(HERE, 'fixtures')
sys.path.insert(0, os.path.join(ROOT, 'utils'))
sys.path.insert(0, HERE)
import sparam_export  # noqa: E402
import sparam_io  # noqa: E402

C0 = 299792458.0
WG_LINE = 'wg1 {a} {b} length=100e-6 neff=2.2 ng=4.2 att=10'
WG_FIXTURES = ['wg.sparam', 'wg.s2p', 'wg_v2.ts', 'wg.csv', 'wg.taylor']


class Run:
    def __init__(self, status, stdout, stderr, rows):
        self.status = status
        self.stdout = stdout
        self.stderr = stderr
        self.rows = rows  # [(time_s, signal, wavelength_m, field)]

    def signal(self, name):
        return [(t, wl, e) for t, s, wl, e in self.rows if s == name]


class Tester:
    def __init__(self, specs, keep):
        self.specs = os.path.abspath(specs)
        self.keep = keep
        self.work = os.path.abspath(keep) if keep else tempfile.mkdtemp(prefix='specs_sparam_')
        os.makedirs(self.work, exist_ok=True)
        self.count = 0
        self.failures = []

    def cleanup(self):
        if not self.keep:
            shutil.rmtree(self.work, ignore_errors=True)

    def run(self, text, name=None):
        """Runs a netlist (fixture paths are absolute) in a fresh directory; returns a Run."""
        self.count += 1
        d = os.path.join(self.work, '%03d_%s' % (self.count, name or 'run'))
        os.makedirs(os.path.join(d, 'traces'))
        with open(os.path.join(d, 'x.cir'), 'w') as f:
            f.write(text)
        env = dict(os.environ, SC_COPYRIGHT_MESSAGE='DISABLE', LC_ALL='C')
        p = subprocess.run([self.specs, '-f', 'x.cir', '-o', 'traces/x', '--format', 'csv'], cwd=d,
                           capture_output=True, text=True, env=env, timeout=300)
        rows = []
        csv_path = os.path.join(d, 'traces', 'x.csv')
        if os.path.exists(csv_path):
            with open(csv_path) as f:
                next(f, None)
                for line in f:
                    t, sig, wl, _, re_, im = line.rstrip('\n').split(',')
                    rows.append((float(t), sig, float(wl), complex(float(re_), float(im))))
        return Run(p.returncode, p.stdout, p.stderr, rows)

    def check(self, name, ok, detail):
        print('%s %-45s %s' % ('PASS' if ok else 'FAIL', name, detail))
        if not ok:
            self.failures.append(name)


def fixture(name):
    return os.path.join(FIXTURES, name)


def max_diff(a, b):
    """Max |field difference| of two sample lists of equal times and wavelengths; inf if they differ."""
    if len(a) != len(b) or not a:
        return math.inf
    worst = 0.0
    for (ta, wa, ea), (tb, wb, eb) in zip(a, b):
        if ta != tb or wa != wb:
            return math.inf
        worst = max(worst, abs(ea - eb))
    return worst


# ---------------------------------------------------------------------------------------------------------

def test_fixtures(t):
    out = os.path.join(t.work, 'fixtures')
    p = subprocess.run([sys.executable, '-I', os.path.join(HERE, 'gen_fixtures.py'), '--out', out],
                       capture_output=True, text=True)
    names = sorted(os.listdir(FIXTURES))
    same = p.returncode == 0 and sorted(os.listdir(out)) == names and all(
        filecmp.cmp(os.path.join(out, n), fixture(n), shallow=False) for n in names)
    t.check('fixtures/regenerated-identical', same, '%d files' % len(names))


def test_ring_data(t):
    # tests/sparam/data/add_drop_ring.sparam (golden case cir/spar-add-drop-ring) is the export of the ring
    # with the command of tests/sparam/README.md: exporting again gives the same values
    committed = os.path.join(HERE, 'data', 'add_drop_ring.sparam')
    again = os.path.join(t.work, 'add_drop_ring.sparam')
    sparam_export.export(t.specs, os.path.join(HERE, 'add_drop_ring.cir'), 'ring', (1.535e-6, 1.565e-6, 1e-9), again,
                         'sparam')
    pa, a = sparam_io.read_sparam(committed)
    pb, b = sparam_io.read_sparam(again)
    worst = 0.0
    same = pa == pb and sorted(a) == sorted(b)
    for key in a if same else []:
        if [f for f, _ in a[key]] != [f for f, _ in b[key]]:
            same = False
            break
        worst = max([worst] + [abs(x - y) for (_, x), (_, y) in zip(a[key], b[key])])
    t.check('data/add-drop-ring-reproduced', same and worst <= 1e-12, 'max |S difference| = %.3g (<= 1e-12)' % worst)


def dc_netlist(device, start, stop, step):
    return '\n'.join([
        '* SPAR phase convention, .dc',
        'cwsrc1 in wl=%r power=1' % start,
        device,
        '.save out',
        '.dc /cwsrc1/WL %r %r %r' % (start, stop, step),
        '']) + '\n'


def test_phase_dc(t):
    sweeps = {'grid': (1.53e-6, 1.57e-6, 1e-9),        # on the fixtures' grid (0.5 nm)
              'between': (1.5003e-6, 1.5997e-6, 0.7e-9)}  # between its points, over the whole range
    for label, (a, b, s) in sweeps.items():
        ref = t.run(dc_netlist(WG_LINE.format(a='in', b='out'), a, b, s), 'wg_dc_' + label)
        ref_out = ref.signal('PROBE{ROOT/out}')
        for fx in WG_FIXTURES:
            r = t.run(dc_netlist('spar1 in out file="%s"' % fixture(fx), a, b, s), 'spar_dc_' + label)
            d = max_diff(r.signal('PROBE{ROOT/out}'), ref_out)
            t.check('phase-dc/%s/%s' % (label, fx), r.status == 0 and d <= 1e-9,
                    'max |E_SPAR - E_WG| = %.3g over %d points (<= 1e-9)' % (d, len(ref_out)))
    # the conversion is needed: the Touchstone fixtures read as physics convention are wrong
    a, b, s = sweeps['grid']
    ref = t.run(dc_netlist(WG_LINE.format(a='in', b='out'), a, b, s), 'wg_dc_conj')
    for fx in ['wg.s2p', 'wg_v2.ts']:
        r = t.run(dc_netlist('spar1 in out file="%s" convention="physics"' % fixture(fx), a, b, s), 'spar_dc_conj')
        d = max_diff(r.signal('PROBE{ROOT/out}'), ref.signal('PROBE{ROOT/out}'))
        t.check('phase-dc/not-conjugated-differs/%s' % fx, r.status == 0 and d > 0.5,
                'max |E_SPAR - E_WG| = %.3g (> 0.5)' % d)


def tran_netlist(device):
    # pulses at 1.55 um: on at 1 ps, half power at 5 ps, off at 9 ps, a second wavelength at 3 ps
    values = '[[0, 0, 1.55e-6], [1e-12, 1, 1.55e-6], [3e-12, 0.3, 1.56e-6], [5e-12, 0.5, 1.55e-6], [9e-12, 0, 1.55e-6]]'
    return '\n'.join([
        '* SPAR phase convention and delay, .tran',
        'vlsrc1 in values=%s' % values,
        device,
        'mlprobe1 out wl=[1.55e-6, 1.56e-6]',
        '.tran 20e-12',
        '']) + '\n'


def test_phase_tran(t):
    ref = t.run(tran_netlist(WG_LINE.format(a='in', b='out')), 'wg_tran')
    ref_rows = [(tt, wl, e) for tt, s, wl, e in ref.rows]
    tau = 100e-6 * 4.2 / C0
    first = min((tt for tt, wl, e in ref_rows if abs(e) > 0), default=None)
    t.check('phase-tran/wg-reference', ref.status == 0 and first is not None and abs(first - (1e-12 + 1.4e-12)) < 1e-18,
            'first output at %r s (1 ps + L*ng/c = %.6g s rounded to 100 fs)' % (first, tau))
    for fx in WG_FIXTURES:
        r = t.run(tran_netlist('spar1 in out file="%s"' % fixture(fx)), 'spar_tran')
        d = max_diff([(tt, wl, e) for tt, s, wl, e in r.rows], ref_rows)
        t.check('phase-tran/%s' % fx, r.status == 0 and d <= 1e-9,
                'same times, max |E_SPAR - E_WG| = %.3g over %d samples' % (d, len(ref_rows)))


def test_bidirectional(t):
    def netlist(device):
        return '\n'.join([
            'cwsrc1 a wl=1.55e-6 power=1',
            'cwsrc2 b wl=1.55e-6 power=0.25 phi=0.3',
            device,
            '.save a b',
            '.dc /cwsrc1/WL 1.55e-6 1.551e-6 1e-9',
            '']) + '\n'
    ref = t.run(netlist(WG_LINE.format(a='a', b='b')), 'wg_bi')
    # fixtures whose reflections are exactly 0 (wg.s2p writes them as -400 dB: active paths, so a PROBE on
    # the source's net latches the swept wavelength of that reflection)
    for fx in ['wg.sparam', 'wg_v2.ts', 'wg.csv']:
        r = t.run(netlist('spar1 a b file="%s"' % fixture(fx)), 'spar_bi')
        # the slots of a net with two writers depend on the creation order: compare the sets of fields
        got = sorted((round(e.real, 9), round(e.imag, 9)) for _, _, _, e in r.rows)
        exp = sorted((round(e.real, 9), round(e.imag, 9)) for _, _, _, e in ref.rows)
        t.check('bidirectional/%s' % fx, r.status == 0 and got == exp and len(exp) == 8,
                '%d samples, fields %s' % (len(got), 'equal' if got == exp else 'differ: %s vs %s' % (got, exp)))


SKEW_SUBCKT = '\n'.join([
    '* two waveguides of different lengths into a coupler: every path has its own delay',
    '.subckt skew a b o1 o2',
    'wg1 a x length=100e-6 neff=2.2 ng=4.2 att=10',
    'wg2 b y length=300e-6 neff=2.2 ng=4.2 att=10',
    'coupler1 x y o1 o2 k=0.5',
    '.ends skew',
    '']) + '\n'


def test_per_path_tran(t):
    # Each output receives a through 1.40 ps and b through 4.20 ps: with one delay per output, b's light
    # would leave with a's delay. Steps on both inputs at the same wavelength (on the grid, then between grid
    # points), while earlier values are still on their way: the outputs are coherent sums of both
    netlist = os.path.join(t.work, 'skew.cir')
    with open(netlist, 'w') as f:
        f.write(SKEW_SUBCKT)
    path = os.path.join(t.work, 'skew.sparam')
    sparam_export.export(t.specs, netlist, 'skew', (1.54e-6, 1.56e-6, 0.5e-9), path, 'sparam')
    def netlist_with(device):
        return '\n'.join([
            '* per-path delays, .tran',
            'vlsrc1 a values=[[0, 0, 1.55e-6], [1e-12, 1, 1.55e-6], [3e-12, 0.4, 1.55e-6], [8e-12, 0, 1.55e-6], '
            '[14e-12, 1, 1.5503e-6], [16e-12, 0.3, 1.5503e-6], [21e-12, 0, 1.5503e-6]]',
            'vlsrc2 b values=[[0, 0, 1.55e-6], [2e-12, 0.5, 1.55e-6], [2.5e-12, 0.2, 1.55e-6], [6e-12, 0, 1.55e-6], '
            '[13e-12, 0.6, 1.5503e-6], [15e-12, 0.1, 1.5503e-6], [19e-12, 0, 1.5503e-6]]',
            device,
            'mlprobe1 o1 wl=[1.55e-6, 1.5503e-6]',
            'mlprobe2 o2 wl=[1.55e-6, 1.5503e-6]',
            '.tran 40e-12',
            '']) + '\n'
    ref = t.run(SKEW_SUBCKT + netlist_with('x1 a b o1 o2 skew'), 'skew_explicit')
    r = t.run(netlist_with('spar1 a b o1 o2 file="%s"' % path), 'skew_spar')
    for probe in ('MLPROBE1', 'MLPROBE2'):
        name = 'ROOT/' + probe
        d = max_diff(sorted(r.signal(name)), sorted(ref.signal(name)))
        t.check('per-path-tran/%s' % probe.lower(), ref.status == 0 and r.status == 0 and d <= 1e-9,
                'same times, max |E_SPAR - E_explicit| = %.3g over %d samples' % (d, len(ref.signal(name))))
    # CW sources: the operating point before the transient (no delays) gives the steady state at t = 0
    def cw_netlist(device):
        return '\n'.join(['* per-path delays, .tran from an operating point',
                          'cwsrc1 a wl=1.55e-6 power=1', 'cwsrc2 b wl=1.55e-6 power=0.5 phi=1', device,
                          '.save o1 o2', '.tran 10e-12', '']) + '\n'
    ref = t.run(SKEW_SUBCKT + cw_netlist('x1 a b o1 o2 skew'), 'skew_cw_explicit')
    r = t.run(cw_netlist('spar1 a b o1 o2 file="%s"' % path), 'skew_cw_spar')
    d = max(max_diff(r.signal('PROBE{ROOT/%s}' % s), ref.signal('PROBE{ROOT/%s}' % s)) for s in ('o1', 'o2'))
    t0 = sorted(set(tt for tt, _, _, _ in r.rows))
    t.check('per-path-tran/operating-point', ref.status == 0 and r.status == 0 and d <= 1e-9 and t0 == [0.0],
            'max |E_SPAR - E_explicit| = %.3g, sample times %s (only 0)' % (d, t0))


def test_port_order(t):
    for fx in ['asym.s2p', 'asym_v2.ts']:
        for ports, expected in [('', 0.8j), (' ports="right,left"', 0.3)]:
            r = t.run(dc_netlist('spar1 in out file="%s"%s' % (fixture(fx), ports), 1.55e-6, 1.56e-6, 1e-8), 'asym')
            out = r.signal('PROBE{ROOT/out}')
            d = max((abs(e - expected) for _, _, e in out), default=math.inf)
            t.check('port-order/%s%s' % (fx, ports.replace(' ', '/') or ''), r.status == 0 and d <= 1e-12,
                    'S = %s, expected %s' % (out[0][2] if out else None, expected))


def test_passivity(t):
    base = 'spar1 in out file="%s"' % fixture('gain.s2p')
    for mode, warn, mag in [('', True, 1.05), (' passivity="ignore"', False, 1.05), (' passivity="enforce"', True, 1.0)]:
        r = t.run(dc_netlist(base + mode, 1.55e-6, 1.56e-6, 1e-8), 'gain')
        warned = 'are not passive' in r.stderr
        out = r.signal('PROBE{ROOT/out}')
        d = max((abs(abs(e) - mag) for _, _, e in out), default=math.inf)
        t.check('passivity/%s' % (mode.strip() or 'check'), r.status == 0 and warned == warn and d <= 1e-12,
                'warning: %s, |S21| = %s (expected %s)' % (warned, abs(out[0][2]) if out else None, mag))


def test_range(t):
    r = t.run(dc_netlist('spar1 in out file="%s"' % fixture('wg.sparam'), 1.59e-6, 1.61e-6, 1e-8), 'range')
    ok = r.status == 1 and re.search(r'wavelength 1\.6\d*e-06 m is outside the range \[1\.\d+e-06, 1\.\d+e-06\] m', r.stderr)
    t.check('range/outside-is-an-error', bool(ok), (r.stderr.strip().splitlines() or [''])[-1])


def add_drop_with_spar(spar_line):
    """circuits/add_drop.cir with its couplers and ring waveguides replaced by spar_line (False: kept), and
    .save out drop."""
    with open(os.path.join(ROOT, 'circuits', 'add_drop.cir')) as f:
        lines = f.read().split('\n')
    ring = re.compile(r'^(coupler1|coupler2|wg_ring_l|wg_ring_r)\b', re.I)
    out = []
    for line in lines:
        if spar_line is not False and ring.match(line):
            if spar_line:
                out.append(spar_line)
                spar_line = None
            continue
        out.append(line)
    return '\n'.join(out) + '\n.save out drop\n'


def test_round_trip(t):
    netlist = os.path.join(HERE, 'add_drop_ring.cir')
    original = t.run(add_drop_with_spar(False), 'add_drop')
    ref = {s: original.signal('PROBE{ROOT/%s}' % s) for s in ('out', 'drop')}
    # same grid as the sweep of add_drop.cir (1.535 to 1.565 um by 1 nm)
    for fmt, ext in [('sparam', 'sparam'), ('touchstone', 's4p'), ('csv', 'csv')]:
        path = os.path.join(t.work, 'ring.' + ext)
        sparam_export.export(t.specs, netlist, 'ring', (1.535e-6, 1.565e-6, 1e-9), path, fmt)
        r = t.run(add_drop_with_spar('spar1 in add out drop file="%s"' % path), 'add_drop_spar')
        d = max(max_diff(r.signal('PROBE{ROOT/%s}' % s), ref[s]) for s in ('out', 'drop'))
        # add_drop.cir converges to its reltol 1e-6; the export used tighter tolerances
        t.check('round-trip/same-grid/%s' % fmt, r.status == 0 and d <= 1e-6,
                'max |E_SPAR - E_add_drop| = %.3g at out and drop (<= 1e-6, the netlist reltol)' % d)
    # interpolation: export on a 0.05 nm grid, compare at the 1 nm points shifted by 0.025 nm, against
    # add_drop.cir swept at the same points
    shifted = (1.535025e-6, 1.564025e-6, 1e-9)
    path = os.path.join(t.work, 'ring_fine.sparam')
    sparam_export.export(t.specs, netlist, 'ring', (1.535e-6, 1.565e-6, 0.05e-9), path, 'sparam', jobs=4)
    # Magnitude and phase are interpolated separately (doc/sparam.md): near the through-port null at
    # 1.55 um (|S| = 0.03) the phase turns by almost pi between two grid points and the error is large;
    # away from it (|E| >= 0.1) it is the interpolation error of a smooth function
    for interp, tol in [('linear', 1e-3), ('pchip', 2e-4)]:
        orig = t.run(add_drop_with_spar(False).replace('.dc /cwsrc1/WL {lambda_min} {lambda_max} {dlambda}',
                                                       '.dc /cwsrc1/WL %r %r %r' % shifted), 'add_drop_shift')
        r = t.run(add_drop_with_spar('spar1 in add out drop file="%s" interp="%s"' % (path, interp)).replace(
            '.dc /cwsrc1/WL {lambda_min} {lambda_max} {dlambda}', '.dc /cwsrc1/WL %r %r %r' % shifted), 'spar_shift')
        n = len(orig.signal('PROBE{ROOT/out}'))
        smooth, anywhere = 0.0, 0.0
        for sig in ('out', 'drop'):
            a, b = r.signal('PROBE{ROOT/%s}' % sig), orig.signal('PROBE{ROOT/%s}' % sig)
            if max_diff(a, b) == math.inf:
                smooth = anywhere = math.inf
                break
            for (_, _, ea), (_, _, eb) in zip(a, b):
                anywhere = max(anywhere, abs(ea - eb))
                if abs(eb) >= 0.1:
                    smooth = max(smooth, abs(ea - eb))
        t.check('round-trip/between-points/%s' % interp, r.status == 0 and n == 30 and smooth <= tol and anywhere <= 0.03,
                'max |E_SPAR - E_add_drop| over %d points between grid points: %.3g where |E| >= 0.1 (<= %g), '
                '%.3g anywhere (<= 0.03)' % (n, smooth, tol, anywhere))


TESTS = [test_fixtures, test_ring_data, test_phase_dc, test_phase_tran, test_bidirectional, test_per_path_tran, test_port_order, test_passivity,
         test_range, test_round_trip]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--specs', required=True)
    ap.add_argument('--keep', help='keep the runs in this directory')
    ap.add_argument('--only', help='run the tests whose function name matches this regex')
    args = ap.parse_args()
    t = Tester(args.specs, args.keep)
    try:
        for test in TESTS:
            if args.only and not re.search(args.only, test.__name__):
                continue
            test(t)
    finally:
        t.cleanup()
    print('%d failed' % len(t.failures))
    return 1 if t.failures else 0


if __name__ == '__main__':
    sys.exit(main())
