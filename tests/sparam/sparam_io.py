"""Minimal readers for the S-parameter file formats of doc/sparam.md (test oracle, standard library only).

Each reader returns ``(ports, data)``: ``ports`` is the list of port names (port 1 first) and ``data`` is
``{(out, in): [(f_Hz, S), ...]}`` with 1-based port numbers, ``S`` complex **in the SPECS convention**
(``exp(+j beta L)`` for a delay; Touchstone values are conjugated), samples sorted by increasing frequency.
Only the pairs present in the file are in ``data`` (a missing pair is 0); for Touchstone every pair is.
``read(path)`` picks the reader from the extension and returns ``data`` only.

These readers are written independently of tests/sparam/gen_fixtures.py and utils/sparam_export.py, from the
text of doc/sparam.md only, so that they can check both. They do not validate much: a malformed file can raise
any exception. ``read_taylor`` returns the polynomials and ``eval_taylor`` evaluates them.
"""

import ast
import cmath
import csv
import math
import os
import re

C0 = 299792458.0


def _strip_comment(line, char):
    i = line.find(char)
    return line if i < 0 else line[:i]


# ---------------------------------------------------------------------------------------------------------
# Lumerical/SiEPIC .sparam


def read_sparam(path):
    ports = []
    declared = False
    data = {}
    with open(path) as f:
        lines = [ln.strip() for ln in f]
    i = 0
    while i < len(lines):
        ln = lines[i]
        i += 1
        if not ln:
            continue
        if ln.startswith('['):
            # port declaration ["name","position"]
            decl = ast.literal_eval(ln)
            ports.append(str(decl[0]))
            declared = True
            continue
        if ln.startswith('('):
            hdr = ast.literal_eval(ln)
            out_name, in_name = str(hdr[0]), str(hdr[3])
            if len(hdr) != 6 or hdr[5] != 'transmission':
                raise ValueError('%s: unsupported block header %s' % (path, ln))
            for name in (out_name, in_name):
                if name not in ports:
                    if declared:
                        raise ValueError('%s: port %s not declared' % (path, name))
                    ports.append(name)
            shape = ast.literal_eval(lines[i])
            i += 1
            nrows, ncols = int(shape[0]), int(shape[1])
            if ncols != 3:
                raise ValueError('%s: %d columns' % (path, ncols))
            rows = []
            for _ in range(nrows):
                fhz, mag, ph = (float(x) for x in lines[i].split())
                i += 1
                rows.append((fhz, cmath.rect(mag, ph)))
            rows.sort(key=lambda r: r[0])
            data[(ports.index(out_name) + 1, ports.index(in_name) + 1)] = rows
            continue
        raise ValueError('%s: unexpected line %r' % (path, ln))
    return ports, data


# ---------------------------------------------------------------------------------------------------------
# Touchstone 1.x / 2.x

_UNITS = {'HZ': 1.0, 'KHZ': 1e3, 'MHZ': 1e6, 'GHZ': 1e9}


def _pair(fmt, a, b):
    if fmt == 'RI':
        return complex(a, b)
    if fmt == 'MA':
        return cmath.rect(a, math.radians(b))
    if fmt == 'DB':
        return cmath.rect(10.0 ** (a / 20.0), math.radians(b))
    raise ValueError(fmt)


def read_touchstone(path):
    unit, fmt = 1e9, 'MA'
    names = {}
    nports = None
    v2 = False
    order_21_12 = True  # version 1 2-port order: S11 S21 S12 S22
    nfreq = None
    in_data = True  # version 1: every non-option line is data
    values = []
    m = re.search(r'\.s(\d+)p$', path, re.IGNORECASE)
    if m:
        nports = int(m.group(1))
    with open(path) as f:
        for raw in f:
            cm = re.match(r'\s*!\s*Port\[(\d+)\]\s*=\s*(.*)$', raw)
            if cm:
                name = cm.group(2).strip()
                q = re.search(r'"([^"]*)"|\'([^\']*)\'', name)
                if q:
                    name = q.group(1) if q.group(1) is not None else q.group(2)
                names[int(cm.group(1))] = name
                continue
            ln = _strip_comment(raw, '!').strip()
            if not ln:
                continue
            if ln.startswith('#'):
                tok = ln[1:].upper().split()
                j = 0
                while j < len(tok):
                    t = tok[j]
                    if t in _UNITS:
                        unit = _UNITS[t]
                    elif t in ('RI', 'MA', 'DB'):
                        fmt = t
                    elif t == 'R':
                        j += 1
                    elif t != 'S':
                        raise ValueError('%s: unsupported option %s' % (path, t))
                    j += 1
                continue
            if ln.startswith('['):
                key, _, val = ln[1:].partition(']')
                key = key.strip().lower()
                val = val.strip()
                if key == 'version':
                    v2 = True
                    in_data = False
                elif key == 'number of ports':
                    nports = int(val)
                elif key == 'two-port data order':
                    order_21_12 = (val == '21_12')
                elif key == 'number of frequencies':
                    nfreq = int(val)
                elif key == 'matrix format':
                    if val.lower() != 'full':
                        raise ValueError('%s: matrix format %s not supported here' % (path, val))
                elif key == 'network data':
                    in_data = True
                elif key == 'end':
                    in_data = False
                continue
            if in_data:
                values.extend(float(x) for x in ln.split())
    n = nports
    per = 1 + 2 * n * n
    if v2 and nfreq is not None and len(values) != nfreq * per:
        raise ValueError('%s: %d values, expected %d' % (path, len(values), nfreq * per))
    if len(values) % per:
        raise ValueError('%s: %d values is not a multiple of %d' % (path, len(values), per))
    data = {}
    for r in range(len(values) // per):
        rec = values[r * per:(r + 1) * per]
        fhz = rec[0] * unit
        for e in range(n * n):
            if n == 2 and order_21_12:
                out, inp = [(1, 1), (2, 1), (1, 2), (2, 2)][e]
            else:
                out, inp = e // n + 1, e % n + 1
            s = _pair(fmt, rec[1 + 2 * e], rec[2 + 2 * e]).conjugate()
            data.setdefault((out, inp), []).append((fhz, s))
    for rows in data.values():
        rows.sort(key=lambda x: x[0])
    ports = [names.get(i, str(i)) for i in range(1, n + 1)]
    return ports, data


# ---------------------------------------------------------------------------------------------------------
# CSV


def read_csv(path):
    with open(path, newline='') as f:
        rows = list(csv.reader(f))
    header = rows[0]
    if header[0] != 'wavelength_m':
        raise ValueError('%s: first column %r' % (path, header[0]))
    ports = []
    cols = {}
    for ci, name in enumerate(header[1:], start=1):
        m = re.match(r'^(.+)@0,(.+)@0_(re|im)$', name)
        if not m:
            raise ValueError('%s: column %r' % (path, name))
        inp, out, part = m.groups()
        for p in (inp, out):
            if p not in ports:
                ports.append(p)
        cols.setdefault((out, inp), {})[part] = ci
    data = {}
    for (out, inp), c in cols.items():
        lst = []
        for r in rows[1:]:
            if not r:
                continue
            lam = float(r[0])
            lst.append((C0 / lam, complex(float(r[c['re']]), float(r[c['im']]))))
        lst.sort(key=lambda x: x[0])
        data[(ports.index(out) + 1, ports.index(inp) + 1)] = lst
    return ports, data


# ---------------------------------------------------------------------------------------------------------
# Taylor form


def read_taylor(path):
    """Returns (ports, center_m, (lam_min, lam_max), {(out, in): {'amp': [c0, ...], 'phase': [...], 'delay': [...]}})."""
    ports, center, rng, terms = None, None, None, {}
    with open(path) as f:
        first = None
        for raw in f:
            ln = _strip_comment(raw, '#').split()
            if not ln:
                continue
            if first is None:
                first = ln
                if [t.lower() for t in ln] != ['specs-taylor', '1']:
                    raise ValueError('%s: first line %r' % (path, raw))
                continue
            kw = ln[0].lower()
            if kw == 'ports':
                ports = [str(i) for i in range(1, int(ln[1]) + 1)] if len(ln) == 2 and ln[1].isdigit() else ln[1:]
            elif kw == 'center':
                center = float(ln[1])
            elif kw == 'range':
                rng = (float(ln[1]), float(ln[2]))
            elif kw == 's':
                idx = []
                for p in ln[1:3]:
                    idx.append(ports.index(p) + 1 if p in ports else int(p))
                terms.setdefault(tuple(idx), {})[ln[3].lower()] = [float(x) for x in ln[4:]]
            else:
                raise ValueError('%s: keyword %r' % (path, ln[0]))
    return ports, center, rng, terms


def _poly(c, d):
    v = 0.0
    for a in reversed(c):
        v = v * d + a
    return v


def eval_taylor(taylor, lam):
    """{(out, in): S} at wavelength lam (m) from the result of read_taylor."""
    _, center, _, terms = taylor
    d = lam - center
    return {k: cmath.rect(_poly(t.get('amp', [0.0]), d), _poly(t.get('phase', [0.0]), d)) for k, t in terms.items()}


def read(path):
    ext = os.path.splitext(path)[1].lower()
    if ext in ('.sparam', '.dat'):
        return read_sparam(path)[1]
    if ext == '.ts' or re.match(r'^\.s\d+p$', ext):
        return read_touchstone(path)[1]
    if ext == '.csv':
        return read_csv(path)[1]
    raise ValueError('%s: unknown extension' % path)
