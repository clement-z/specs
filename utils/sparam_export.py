#!/usr/bin/env python3
"""Export the S-matrix of a SPECS subcircuit to an S-parameter file (doc/sparam.md, "Export").

    python3 -I utils/sparam_export.py --specs build/specs --netlist ring.cir --subckt ring \\
        --wl 1.535e-6 1.565e-6 1e-9 --format sparam -o ring.sparam

For each port k of the subcircuit, a netlist is written in a temporary directory:

    .include <absolute path of --netlist>
    X1 sp_<port1> ... sp_<portN> <SUBCKT>
    cwsrc1 sp_<port k> wl=START power=1
    .save sp_<port1> ... sp_<portN>
    .options <--options>
    .dc /cwsrc1/WL START STOP STEP

and run with `specs --format csv`. S[j][k] is the field leaving port j divided by the source field (1 + 0j,
power 1 W, phase 0), at each sweep point. The --netlist file must only define things (.subckt, .param): a
top-level element, source or analysis in it would be part of every run.

Choosing the probe of each port (the .save probes are PROBE{ROOT/net} on a unidirectional net, PROBE{ROOT/net:0}
and PROBE{ROOT/net:1} on a bidirectional one; a signal that nobody writes has no samples):
- j != k: the probe of net j that has samples (more than one: error); none: S[j][k] = 0.
- j == k: the probe whose field is the source field at every point is the source's signal; the other probe of
  the net, if it has samples, is the reflection, else the reflection is 0 (unidirectional net: one probe). No
  probe or two probes equal to the source field: error.

Values are in the SPECS (physics) convention and are converted to the convention of the output format
(Touchstone: complex conjugate; .sparam and CSV: as is).

The module can be imported: read_trace_csv(), sparam_from_traces(), write_sparam(), write_touchstone(),
write_csv(), export().
"""

import argparse
import cmath
import concurrent.futures
import csv
import os
import re
import shutil
import subprocess
import sys
import tempfile

C0 = 299792458.0

# Tight tolerances by default: SPECS's defaults (abstol 1e-8 sqrt(W), reltol 1e-4) stop each .dc fixed point
# once the changes are below them, so the exported values would only be accurate to about 1e-4 (relative).
DEFAULT_OPTIONS = 'abstol=1e-12 reltol=1e-9'

NET_PREFIX = 'sp_'

# Tolerance on the source field when telling the source's signal from the reflection on the excited net.
SOURCE_TOL = 1e-12

_NAME_RE = re.compile(r'^[A-Za-z0-9_.+\-]+$')


class ExportError(Exception):
    pass


# ---------------------------------------------------------------------------------------------------------
# Netlist side


def subckt_ports(netlist, name):
    """Port names of `.subckt NAME p1 p2 ... [param=default ...]` in the file (name case-insensitive)."""
    with open(netlist) as f:
        lines = f.read().splitlines()
    logical = []
    for ln in lines:
        ln = ln.split(';', 1)[0].rstrip()
        if ln.startswith('+') and logical:
            logical[-1] += ' ' + ln[1:]
        else:
            logical.append(ln)
    found = []
    for ln in logical:
        tok = ln.split()
        if len(tok) >= 2 and tok[0].lower() == '.subckt' and tok[1].lower() == name.lower():
            ports = []
            for t in tok[2:]:
                if '=' in t:
                    break
                ports.append(t)
            found.append(ports)
    if not found:
        raise ExportError('%s: no `.subckt %s` line' % (netlist, name))
    if len(found) > 1:
        raise ExportError('%s: %d `.subckt %s` lines' % (netlist, len(found), name))
    if not found[0]:
        raise ExportError('%s: subcircuit %s has no ports' % (netlist, name))
    return found[0]


def port_netlist(netlist_abs, subckt, ports, k, wl, options):
    """Text of the netlist that excites port k (0-based)."""
    nets = [NET_PREFIX + p for p in ports]
    start, stop, step = (repr(float(x)) for x in wl)
    lines = ['* utils/sparam_export.py: subcircuit %s, source on port %d (%s)' % (subckt, k + 1, ports[k]),
             '.include ' + netlist_abs,
             'X1 %s %s' % (' '.join(nets), subckt),
             'cwsrc1 %s wl=%s power=1' % (nets[k], start),
             '.save ' + ' '.join(nets)]
    if options.strip():
        lines.append('.options ' + options.strip())
    lines.append('.dc /cwsrc1/WL %s %s %s' % (start, stop, step))
    return '\n'.join(lines) + '\n'


def run_port(specs, workdir, text):
    """Writes the netlist in workdir, runs specs there, returns the path of the CSV trace."""
    os.makedirs(os.path.join(workdir, 'traces'), exist_ok=True)
    with open(os.path.join(workdir, 'netlist.cir'), 'w') as f:
        f.write(text)
    log = os.path.join(workdir, 'specs.log')
    with open(log, 'w') as out:
        rc = subprocess.call([specs, '-f', 'netlist.cir', '-o', 'traces/port', '--format', 'csv'],
                             cwd=workdir, stdout=out, stderr=subprocess.STDOUT)
    if rc != 0:
        with open(log) as f:
            tail = ''.join(f.readlines()[-15:])
        raise ExportError('specs failed (exit status %d) on %s:\n%s' % (rc, os.path.join(workdir, 'netlist.cir'), tail))
    trace = os.path.join(workdir, 'traces', 'port.csv')
    if not os.path.exists(trace):
        raise ExportError('specs wrote no trace %s' % trace)
    return trace


# ---------------------------------------------------------------------------------------------------------
# Traces


_PROBE_RE = re.compile(r'^PROBE\{ROOT/([^}:]+)(?::(\d+))?\}$')


def read_trace_csv(path):
    """Reads a `specs --format csv` trace of a .dc sweep.

    Returns (wavelengths, probes): wavelengths[i] is the wavelength (m) of sweep point i (points are the
    time steps, in time order); probes is {signal name: [complex field or None per point]}."""
    with open(path, newline='') as f:
        rows = list(csv.reader(f))
    if not rows or rows[0][:6] != ['time_s', 'signal', 'wavelength_m', 'power_W', 'field_re', 'field_im']:
        raise ExportError('%s: not a SPECS CSV trace' % path)
    samples = []
    for r in rows[1:]:
        if r:
            samples.append((float(r[0]), r[1], float(r[2]), complex(float(r[4]), float(r[5]))))
    times = sorted(set(s[0] for s in samples))
    index = {t: i for i, t in enumerate(times)}
    wavelengths = [None] * len(times)
    probes = {}
    for t, sig, wl, e in samples:
        i = index[t]
        if wavelengths[i] is None:
            wavelengths[i] = wl
        elif wavelengths[i] != wl:
            raise ExportError('%s: two wavelengths (%r, %r) at time %r' % (path, wavelengths[i], wl, t))
        vals = probes.setdefault(sig, [None] * len(times))
        if vals[i] is not None:
            raise ExportError('%s: %s has two samples at time %r' % (path, sig, t))
        vals[i] = e
    return wavelengths, probes


def _net_probes(probes, net, path):
    """{signal name: values} of the probes of a net that have samples (all points, else error)."""
    out = {}
    for sig, vals in probes.items():
        m = _PROBE_RE.match(sig)
        if m and m.group(1) == net:
            if any(v is None for v in vals):
                raise ExportError('%s: %s has samples at only %d of %d sweep points'
                                  % (path, sig, sum(v is not None for v in vals), len(vals)))
            out[sig] = vals
    return out


def sparam_from_traces(ports, traces):
    """S-matrix from the traces of the N runs (traces[k]: CSV trace with the source on port k).

    Returns (wavelengths, smat): smat[p][j][k] is S[j][k] (0-based ports) at sweep point p."""
    n = len(ports)
    nets = [NET_PREFIX + p for p in ports]
    wavelengths = None
    columns = []  # columns[k][j] = list over points
    for k, path in enumerate(traces):
        wls, probes = read_trace_csv(path)
        if not wls:
            raise ExportError('%s: no samples' % path)
        if wavelengths is None:
            wavelengths = wls
        elif wls != wavelengths:
            raise ExportError('%s: sweep wavelengths differ from those of port 1 (%d and %d points)'
                              % (path, len(wls), len(wavelengths)))
        npts = len(wls)
        col = []
        for j in range(n):
            sampled = _net_probes(probes, nets[j], path)
            if j != k:
                if len(sampled) > 1:
                    raise ExportError('%s: port %s: more than one probe with samples (%s)'
                                      % (path, ports[j], ', '.join(sorted(sampled))))
                col.append(list(next(iter(sampled.values()))) if sampled else [0j] * npts)
                continue
            src = [s for s, v in sampled.items() if all(abs(e - 1) <= SOURCE_TOL for e in v)]
            if not src:
                raise ExportError('%s: port %s: no probe of net %s carries the source field (1, 0) at every point'
                                  ' (probes with samples: %s)' % (path, ports[j], nets[j], ', '.join(sorted(sampled)) or 'none'))
            if len(src) > 1:
                raise ExportError('%s: port %s: ambiguous reflection: both signals of net %s equal the source field'
                                  ' (1, 0) at every point (%s)' % (path, ports[j], nets[j], ', '.join(sorted(src))))
            others = [v for s, v in sampled.items() if s != src[0]]
            if len(others) > 1:
                raise ExportError('%s: port %s: more than two probes on net %s' % (path, ports[j], nets[j]))
            col.append(list(others[0]) if others else [0j] * npts)
        columns.append(col)
    smat = [[[columns[k][j][p] for k in range(n)] for j in range(n)] for p in range(len(wavelengths))]
    return wavelengths, smat


# ---------------------------------------------------------------------------------------------------------
# Writers. smat[p][j][k] = S[j][k] at wavelengths[p], SPECS (physics) convention.


def _r(x):
    return repr(float(x) + 0.0)  # + 0.0: no -0.0


def _check_names(ports):
    for p in ports:
        if not _NAME_RE.match(p):
            raise ExportError('port name %r: use letters, digits and _ . + - only' % p)


def _ascending_freq(wavelengths):
    """Point indices in ascending frequency (descending wavelength)."""
    return sorted(range(len(wavelengths)), key=lambda p: C0 / wavelengths[p])


def write_sparam(path, ports, wavelengths, smat):
    """Lumerical/SiEPIC .sparam: a block per (out, in) pair, rows `f |S| phase`, ascending frequency."""
    _check_names(ports)
    n = len(ports)
    order = _ascending_freq(wavelengths)
    lines = []
    for i, p in enumerate(ports):
        lines.append("['%s','%s']" % (p, 'LEFT' if i < (n + 1) // 2 else 'RIGHT'))
    for j in range(n):
        for k in range(n):
            lines.append("('%s','TE',1,'%s',1,'transmission')" % (ports[j], ports[k]))
            lines.append('(%d,3)' % len(order))
            for p in order:
                s = smat[p][j][k]
                lines.append('%s %s %s' % (_r(C0 / wavelengths[p]), _r(abs(s)), _r(cmath.phase(s))))
    with open(path, 'w', newline='\n') as f:
        f.write('\n'.join(lines) + '\n')


def write_touchstone(path, ports, wavelengths, smat, version=1):
    """Touchstone 1.x or 2.0, `# Hz S RI R 50`, engineering convention (conjugate), ascending frequency."""
    n = len(ports)
    order = _ascending_freq(wavelengths)
    ext = os.path.splitext(path)[1].lower()
    snp = '.s%dp' % n
    if version == 1 and ext != snp:
        print('Warning: %s: a Touchstone 1 file with %d ports should end in %s' % (path, n, snp), file=sys.stderr)
    if version == 2 and ext not in ('.ts', snp):
        print('Warning: %s: a Touchstone 2 file should end in .ts (or %s)' % (path, snp), file=sys.stderr)
    lines = ['! S-matrix exported by SPECS utils/sparam_export.py',
             '! Engineering convention exp(+j omega t): values are the complex conjugates of the SPECS ones']
    for i, p in enumerate(ports):
        lines.append('! Port[%d] = %s' % (i + 1, p))
    if version == 2:
        lines.append('[Version] 2.0')
    lines.append('# Hz S RI R 50')
    if version == 2:
        lines.append('[Number of Ports] %d' % n)
        if n == 2:
            lines.append('[Two-Port Data Order] 12_21')
        lines.append('[Number of Frequencies] %d' % len(order))
        lines.append('[Matrix Format] Full')
        lines.append('[Network Data]')
    for p in order:
        m = smat[p]
        f = _r(C0 / wavelengths[p])

        def pair(j, k):
            s = m[j][k].conjugate()
            return '%s %s' % (_r(s.real), _r(s.imag))

        if n == 2 and version == 1:     # S11 S21 S12 S22
            lines.append(' '.join([f, pair(0, 0), pair(1, 0), pair(0, 1), pair(1, 1)]))
        elif n <= 2:                    # 1-port, or 2-port 12_21: S11 S12 S21 S22
            lines.append(' '.join([f] + [pair(j, k) for j in range(n) for k in range(n)]))
        else:                           # row by row, each row on new lines, at most 4 pairs per line
            for j in range(n):
                row = [pair(j, k) for k in range(n)]
                for c in range(0, n, 4):
                    head = f if (j == 0 and c == 0) else ' ' * len(f)
                    lines.append(' '.join([head] + row[c:c + 4]))
    if version == 2:
        lines.append('[End]')
    with open(path, 'w', newline='\n') as fh:
        fh.write('\n'.join(lines) + '\n')


def write_csv(path, ports, wavelengths, smat):
    """CSV of doc/sparam.md (physics convention), every pair, input-major columns so that the ports appear
    in port order; rows in sweep order."""
    _check_names(ports)
    n = len(ports)
    keys = [(j, k) for k in range(n) for j in range(n)]
    head = ['wavelength_m']
    for j, k in keys:
        head += ['"%s@0,%s@0_re"' % (ports[k], ports[j]), '"%s@0,%s@0_im"' % (ports[k], ports[j])]
    lines = [','.join(head)]
    for p, wl in enumerate(wavelengths):
        row = [_r(wl)]
        for j, k in keys:
            s = smat[p][j][k]
            row += [_r(s.real), _r(s.imag)]
        lines.append(','.join(row))
    with open(path, 'w', newline='\n') as f:
        f.write('\n'.join(lines) + '\n')


# ---------------------------------------------------------------------------------------------------------


def summary(ports, wavelengths, smat):
    n = len(ports)
    maxs, maxs_at = 0.0, None
    maxp, maxp_at = 0.0, None
    for p, m in enumerate(smat):
        for k in range(n):
            col = sum(abs(m[j][k]) ** 2 for j in range(n))
            if col > maxp:
                maxp, maxp_at = col, (ports[k], wavelengths[p])
            for j in range(n):
                if abs(m[j][k]) > maxs:
                    maxs, maxs_at = abs(m[j][k]), ('%s<-%s' % (ports[j], ports[k]), wavelengths[p])
    lines = ['ports: %s' % ', '.join(ports),
             'points: %d, %r .. %r m' % (len(wavelengths), min(wavelengths), max(wavelengths))]
    if maxs_at:
        lines.append('max |S|: %.6g (%s at %r m)' % (maxs, maxs_at[0], maxs_at[1]))
        lines.append('max column power sum_j |S[j][k]|^2: %.6g (input %s at %r m)' % (maxp, maxp_at[0], maxp_at[1]))
    else:
        lines.append('max |S|: 0')
    return '\n'.join(lines)


def export(specs, netlist, subckt, wl, out, fmt, ports=None, options=DEFAULT_OPTIONS, touchstone_version=1,
           keep=None, jobs=1):
    """Runs the sweeps and writes `out`. Returns (port names, wavelengths, smat)."""
    netlist_abs = os.path.abspath(netlist)
    if re.search(r'\s', netlist_abs):
        raise ExportError('the netlist path must not contain white space: %s' % netlist_abs)
    sub_ports = subckt_ports(netlist_abs, subckt)
    names = list(ports) if ports else list(sub_ports)
    if len(names) != len(sub_ports):
        raise ExportError('--ports gives %d names, subcircuit %s has %d ports' % (len(names), subckt, len(sub_ports)))
    if len(set(names)) != len(names):
        raise ExportError('duplicate port names: %s' % ', '.join(names))
    specs = os.path.abspath(specs) if os.path.sep in specs else (shutil.which(specs) or specs)
    work = os.path.abspath(keep) if keep else tempfile.mkdtemp(prefix='sparam_export_')
    try:
        texts = [port_netlist(netlist_abs, subckt, sub_ports, k, wl, options) for k in range(len(sub_ports))]
        dirs = [os.path.join(work, 'port%d' % (k + 1)) for k in range(len(sub_ports))]
        with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, jobs)) as ex:
            traces = list(ex.map(lambda a: run_port(specs, *a), zip(dirs, texts)))
        wavelengths, smat = sparam_from_traces(sub_ports, traces)
    finally:
        if not keep:
            shutil.rmtree(work, ignore_errors=True)
    if fmt == 'sparam':
        write_sparam(out, names, wavelengths, smat)
    elif fmt == 'touchstone':
        write_touchstone(out, names, wavelengths, smat, touchstone_version)
    elif fmt == 'csv':
        write_csv(out, names, wavelengths, smat)
    else:
        raise ExportError('unknown format %r' % fmt)
    return names, wavelengths, smat


def _format_from_ext(path):
    ext = os.path.splitext(path)[1].lower()
    if ext in ('.sparam', '.dat'):
        return 'sparam'
    if ext == '.ts' or re.match(r'^\.s\d+p$', ext):
        return 'touchstone'
    if ext == '.csv':
        return 'csv'
    return None


def main(argv=None):
    ap = argparse.ArgumentParser(
        description='Export the S-matrix of a SPECS subcircuit (one .dc wavelength sweep per port) to a '
                    '.sparam, Touchstone or CSV file (doc/sparam.md).',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='The --netlist file must only define the subcircuit (no top-level element or analysis).\n'
               'Values are converted to the convention of the format (Touchstone: complex conjugate).')
    ap.add_argument('--specs', default='specs', help='specs executable (default: specs on the PATH)')
    ap.add_argument('--netlist', required=True, help='file that defines the subcircuit')
    ap.add_argument('--subckt', required=True, help='subcircuit name (case-insensitive)')
    ap.add_argument('--wl', nargs=3, type=float, required=True, metavar=('START', 'STOP', 'STEP'),
                    help='wavelength sweep (m), as `.dc /cwsrc1/WL START STOP STEP` (STOP included)')
    ap.add_argument('--format', choices=['sparam', 'touchstone', 'csv'],
                    help='output format (default: from the extension of -o)')
    ap.add_argument('-o', '--output', required=True, help='output file')
    ap.add_argument('--ports', help='comma-separated port names written in the file (default: the subcircuit\'s)')
    ap.add_argument('--options', default=DEFAULT_OPTIONS,
                    help='`.options` line added to each run (default: "%(default)s": tight tolerances, so that '
                         'every .dc point converges far below the default reltol 1e-4 and the exported values '
                         'are accurate; "" for none)')
    ap.add_argument('--touchstone-version', type=int, choices=[1, 2], default=1, help='default: %(default)s')
    ap.add_argument('--keep', metavar='DIR', help='keep the generated netlists, logs and traces in DIR')
    ap.add_argument('-j', '--jobs', type=int, default=1, help='parallel runs (default: %(default)s)')
    args = ap.parse_args(argv)
    fmt = args.format or _format_from_ext(args.output)
    if fmt is None:
        ap.error('cannot tell the format from %s: give --format' % args.output)
    ports = [p.strip() for p in args.ports.split(',')] if args.ports else None
    try:
        names, wls, smat = export(args.specs, args.netlist, args.subckt, args.wl, args.output, fmt, ports,
                                  args.options, args.touchstone_version, args.keep, args.jobs)
    except (ExportError, OSError) as e:
        print('Error: %s' % e, file=sys.stderr)
        return 1
    print(summary(names, wls, smat))
    print('wrote %s (%s)' % (args.output, fmt if fmt != 'touchstone' else 'touchstone %d' % args.touchstone_version))
    return 0


if __name__ == '__main__':
    sys.exit(main())
