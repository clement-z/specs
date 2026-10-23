#!/usr/bin/env python3
"""Checks utils/sparam_export.py on the add-drop ring of circuits/add_drop.cir (standard library only).

    python3 -I tests/sparam/check_ring_export.py --specs build/specs [--tol 1e-9]

1. Exports tests/sparam/add_drop_ring.cir (subcircuit `ring in add out drop`: the couplers and ring
   waveguides of circuits/add_drop.cir) at the add_drop sweep grid, in every format (.sparam, CSV, Touchstone
   1 .s4p and 2 .ts), and reads each file back with the independent readers of sparam_io.py: they must give
   the same matrix (within 1e-12).
2. Runs circuits/add_drop.cir without its 1 mm access subcircuit x1 (the source drives net `in` directly),
   with `.save in out drop`, and compares its fields at `out` and `drop` with S[out][in] and S[drop][in]:
   once with the netlist's own tolerances (reltol 1e-6, reported only) and once with the export's tolerances
   (must agree within --tol). Also reported: add_drop.cir as is, its fields divided by the analytic
   transmission of x1.
"""

import argparse
import cmath
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, 'utils'))
import sparam_export  # noqa: E402
import sparam_io  # noqa: E402

C0 = 299792458.0
WL = (1.535e-6, 1.565e-6, 1e-9)
PORTS = ['in', 'add', 'out', 'drop']


def access_transmission(lam):
    """Field transmission of x1 in circuits/add_drop.cir: two 1 mm waveguides, neff 1, ng 4, att 0 (the WG
    model of src/devices/waveguide.cpp)."""
    length_cm = 1e-3 * 100
    neff = 1.0 + (1.0 - 4.0) / 1.55e-6 * (lam - 1.55e-6)
    return cmath.rect(1.0, 2.0 * math.pi * length_cm * 1e-2 * neff / lam) ** 2


def direct_netlist(options_line, keep_x1=False):
    """circuits/add_drop.cir without x1, the source on `in`, `.save in out drop`; options_line replaces its
    .options line if given. keep_x1: the netlist as is, plus the .save line."""
    with open(os.path.join(ROOT, 'circuits', 'add_drop.cir')) as f:
        lines = f.read().splitlines()
    out = []
    for ln in lines:
        low = ln.lower()
        if not keep_x1 and low.startswith('x1 '):
            continue
        if not keep_x1 and low.startswith('cwsrc1 '):
            ln = re.sub(r'^(cwsrc1\s+)in_\b', r'\1in', ln, flags=re.IGNORECASE)
            assert ln.split()[1] == 'in', ln
        if low.startswith('.options') and options_line:
            ln = options_line
        if low.startswith('.dc '):
            out.append('.save in out drop')
        out.append(ln)
    return '\n'.join(out) + '\n'


def run_direct(specs, work, name, text):
    d = os.path.join(work, name)
    os.makedirs(os.path.join(d, 'traces'))
    with open(os.path.join(d, 'netlist.cir'), 'w') as f:
        f.write(text)
    with open(os.path.join(d, 'specs.log'), 'w') as log:
        rc = subprocess.call([specs, '-f', 'netlist.cir', '-o', 'traces/t', '--format', 'csv'], cwd=d,
                             stdout=log, stderr=subprocess.STDOUT)
    if rc:
        sys.exit('specs failed on %s' % os.path.join(d, 'netlist.cir'))
    return sparam_export.read_trace_csv(os.path.join(d, 'traces', 't.csv'))


def compare_direct(wls, smat, trace, divide_x1=False):
    twls, probes = trace
    if len(twls) != len(wls):
        sys.exit('direct run: %d points, export: %d' % (len(twls), len(wls)))
    worst_wl = max(abs(a - b) / a for a, b in zip(wls, twls))
    worst = 0.0
    for net, j in (('out', 2), ('drop', 3)):
        vals = probes['PROBE{ROOT/%s}' % net]
        for p, e in enumerate(vals):
            if divide_x1:
                e /= access_transmission(twls[p])
            worst = max(worst, abs(e - smat[p][j][0]))
    return worst, worst_wl


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--specs', required=True)
    ap.add_argument('--tol', type=float, default=1e-9, help='max |difference| with matched tolerances')
    ap.add_argument('--keep', help='keep the work files in this directory')
    args = ap.parse_args()
    specs = os.path.abspath(args.specs)
    work = os.path.abspath(args.keep) if args.keep else tempfile.mkdtemp(prefix='check_ring_export_')
    os.makedirs(work, exist_ok=True)
    netlist = os.path.join(HERE, 'add_drop_ring.cir')
    status = 0
    try:
        names, wls, smat = sparam_export.export(specs, netlist, 'ring', WL, os.path.join(work, 'ring.csv'), 'csv',
                                                jobs=4, keep=os.path.join(work, 'runs'))
        print(sparam_export.summary(names, wls, smat))
        sparam_export.write_sparam(os.path.join(work, 'ring.sparam'), names, wls, smat)
        sparam_export.write_touchstone(os.path.join(work, 'ring.s4p'), names, wls, smat, 1)
        sparam_export.write_touchstone(os.path.join(work, 'ring.ts'), names, wls, smat, 2)

        # 1. read back every format
        for fname, reader in (('ring.csv', sparam_io.read_csv), ('ring.sparam', sparam_io.read_sparam),
                              ('ring.s4p', sparam_io.read_touchstone), ('ring.ts', sparam_io.read_touchstone)):
            ports, data = reader(os.path.join(work, fname))
            worst = 0.0
            order = sorted(range(len(wls)), key=lambda p: C0 / wls[p])
            for j in range(4):
                for k in range(4):
                    rows = data[(j + 1, k + 1)]
                    for (f, s), p in zip(rows, order):
                        worst = max(worst, abs(s - smat[p][j][k]), abs(f - C0 / wls[p]) / f)
            ok = ports == PORTS and worst <= 1e-12
            status |= not ok
            print('read back %-11s ports %s, max difference %.3g %s' % (fname, ports, worst, '' if ok else 'FAIL'))

        # 2. against add_drop.cir without x1
        orig = compare_direct(wls, smat, run_direct(specs, work, 'original', direct_netlist(None, True)), True)
        print('add_drop.cir as is, x1 divided out analytically: max |S - E| = %.3g (wavelengths: %.3g rel.)' % orig)
        own = compare_direct(wls, smat, run_direct(specs, work, 'direct_own', direct_netlist(None)))
        opts = '.options %s timescale=-15' % sparam_export.DEFAULT_OPTIONS
        matched = compare_direct(wls, smat, run_direct(specs, work, 'direct_matched', direct_netlist(opts)))
        print('add_drop.cir without x1, its own .options (reltol 1e-6): max |S - E| = %.3g (wavelengths: %.3g rel.)'
              % own)
        print('add_drop.cir without x1, %s: max |S - E| = %.3g (wavelengths: %.3g rel.)'
              % ((opts,) + matched))
        if matched[0] > args.tol or matched[1] > 1e-12:
            print('FAIL: difference above %g' % args.tol)
            status = 1
    finally:
        if not args.keep:
            shutil.rmtree(work, ignore_errors=True)
    return status


if __name__ == '__main__':
    sys.exit(main())
