#!/usr/bin/env python3
"""table_vs_vcd.py REPO BASE: F-05 oracle. BASE.vcd and BASE.csv and/or BASE.npz from one run
(--format vcd --format csv --format npz). Checks, for every probe of the table:
- rows are in time order, at most one per (time step, probe, wavelength);
- at every time step of its rows, the VCD value at that time equals the row of the wavelength the VCD
  shows (or, for an MLPROBE variable, of its wavelength): power and wavelength exactly once rounded to the
  16 digits of the VCD, abs = |E| and phase = arg(E) within 8 ulp of that (recomputed here);
- every VCD record of the probe (after the initial $dumpvars) is at a time where the table has a row;
- the CSV and the .npz hold the same rows, bit for bit.
"""
import ast, cmath, csv, math, os, struct, sys, zipfile
sys.path.insert(0, sys.argv[1] + "/tests/golden")
import run_golden as rg

base = sys.argv[2]
vcd = rg.parse_vcd(rg.normalise_vcd(open(base + ".vcd").read()))
ts = vcd.timescale.split()
unit = {"s": 1, "ms": 1e-3, "us": 1e-6, "ns": 1e-9, "ps": 1e-12, "fs": 1e-15}
tick = float(ts[0]) * unit[ts[1]]
tl = rg.timelines(vcd)

def read_csv(p):
    rows = []
    with open(p) as f:
        r = csv.reader(f)
        assert next(r) == ["time_s", "signal", "wavelength_m", "power_W", "field_re", "field_im"]
        for t, sig, wl, pw, re_, im in r:
            rows.append((float(t), sig, float(wl), float(pw), float(re_), float(im)))
    return rows

def read_npz(p):
    arrs = {}
    with zipfile.ZipFile(p) as z:
        for info in z.infolist():
            d = z.read(info)
            assert d[:8] == b"\x93NUMPY\x01\x00"
            hl = struct.unpack("<H", d[8:10])[0]
            h = ast.literal_eval(d[10:10 + hl].decode())
            data = d[10 + hl:]
            n = h["shape"][0]
            if h["descr"] == "<f8":
                v = struct.unpack("<%dd" % n, data)
            elif h["descr"] == "<i4":
                v = struct.unpack("<%di" % n, data)
            else:
                w = int(h["descr"][2:])
                v = [data[i * 4 * w:(i + 1) * 4 * w].decode("utf-32-le").rstrip("\0") for i in range(n)]
            arrs[info.filename[:-4]] = list(v)
    names = arrs["signal_names"]
    return [(arrs["time_s"][i], names[arrs["signal"][i]], arrs["wavelength_m"][i], arrs["power_W"][i],
             arrs["field_re"][i], arrs["field_im"][i]) for i in range(len(arrs["time_s"]))]

tables = {}
for ext, reader in ((".csv", read_csv), (".npz", read_npz)):
    if os.path.exists(base + ext):
        tables[ext] = reader(base + ext)
assert tables, "no table"
if len(tables) == 2:
    assert [tuple(map(repr, r)) for r in tables[".csv"]] == [tuple(map(repr, r)) for r in tables[".npz"]], "CSV != npz"
rows = next(iter(tables.values()))

def close(a, b, ulps=4):
    return a == b or abs(a - b) <= ulps * math.ulp(max(abs(a), abs(b)))

errs, nchecked = [], 0
by_sig = {}  # signal -> tick -> {wavelength: (power, re, im)}
for t, sig, wl, pw, re_, im in rows:
    d = by_sig.setdefault(sig, {}).setdefault(round(t / tick), {})
    if wl in d:
        errs.append("%s: two rows at #%s for %r" % (sig, round(t / tick), wl))
    d[wl] = (pw, re_, im)
prev = None
for r in rows:
    if prev is not None and r[0] < prev:
        errs.append("rows not in time order at %r" % r[0])
    prev = r[0]

def expected(q, wl, pw, re_, im):
    return {"power": pw, "wavelength": wl, "abs": abs(complex(re_, im)), "phase": cmath.phase(complex(re_, im))}[q]

def matches(q, cur, exp):
    # SystemC writes VCD reals with 16 significant digits (%.16g); the table keeps every bit
    exp16 = float("%.16g" % exp)
    return cur == exp16 if q in ("power", "wavelength") else close(cur, exp16, 8)

for sig, pts in by_sig.items():
    pref = "SystemC." + sig + "."
    names = [n for n in vcd.vars if n.startswith(pref)]
    cur = {n: None for n in names}
    for n in names:
        for t in tl.get(n, {}):
            if t != 0 and t not in pts:
                errs.append("%s: VCD record at #%s has no table row" % (n, t))
    for k in sorted(set(pts) | {t for n in names for t in tl.get(n, {})}):
        for n in names:
            if k in tl.get(n, {}):
                cur[n] = float(tl[n][k][1:])
        if k not in pts:
            continue
        cands = pts[k]
        wl_var = pref + "wavelength"
        if wl_var in cur and cur[wl_var] is not None:
            sel = {wl: v for wl, v in cands.items() if float("%.16g" % wl) == cur[wl_var]}
        else:
            sel = cands
        for n in names:
            q = n[len(pref):]
            if "@" in q:
                q, wls = q.split("@")
                sel_n = {wl: v for wl, v in cands.items() if abs(float(wls.replace("_", ".")) * 1e-9 - wl) < 1e-18}
            else:
                sel_n = sel
            if not sel_n:
                if "@" in n[len(pref):]:
                    continue
                errs.append("%s at #%s: no row with the VCD wavelength" % (n, k))
                continue
            nchecked += 1
            if not any(matches(q, cur[n], expected(q, wl, *v)) for wl, v in sel_n.items()):
                errs.append("%s at #%s: VCD %r, table %r" % (n, k, cur[n], sel_n))
print("%s: %d rows, %d signals, %d (time, variable) points checked, %s" % (
    os.path.basename(base), len(rows), len(by_sig), nchecked, "OK" if not errs else "ERRORS: " + "; ".join(errs[:5])))
sys.exit(1 if errs else 0)
