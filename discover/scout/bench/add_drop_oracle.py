#!/usr/bin/env python3
"""add_drop_oracle.py: check SPECS's `.dc` wavelength sweep of circuits/add_drop.cir against the
closed-form add-drop ring transfer function (Yariv 2002). Reuses the VCD parser of
tests/golden/run_golden.py.

Usage: python3 -I add_drop_oracle.py --specs /path/to/specs --repo /path/to/repo

Model (two identical lossless couplers with field cross-coupling kappa, through t = sqrt(1-kappa^2);
two half-ring waveguides of length L each, field transmission a_h and phase phi_h each):
    through/in = t (1 - a e^{j phi}) / (1 - t^2 a e^{j phi}),   drop/in = -kappa^2 a_h e^{j phi_h} / (1 - t^2 a e^{j phi})
with a = a_h^2, phi = 2 phi_h, phi_h = 2 pi neff(lambda) L / lambda and
neff(lambda) = neff0 + (neff0 - ng0)/lambda_ref (lambda - lambda_ref) (SPECS waveguide model, lambda_ref = 1.55 um).
"""
import argparse
import importlib.util
import math
import os
import shutil
import subprocess
import sys
import tempfile


def load_golden(repo):
    p = os.path.join(repo, "tests", "golden", "run_golden.py")
    spec = importlib.util.spec_from_file_location("run_golden", p)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def fval(s):
    return float(s[1:]) if s[:1] in "rR" else float(s)


def model(lam, kappa=0.85, neff0=2.3, ng0=4.3, att_db_cm=2.0, lam_ref=1.55e-6, n_2pi=101):
    L = n_2pi * lam_ref / neff0  # half-ring length in m (add_drop.cir: 101*{length_2pi})
    t = math.sqrt(1 - kappa * kappa)
    a_h = 10 ** (-(att_db_cm * L * 100) / 20)
    neff = neff0 + (neff0 - ng0) / lam_ref * (lam - lam_ref)
    phi_h = 2 * math.pi * neff * L / lam
    e_h = a_h * complex(math.cos(phi_h), math.sin(phi_h))
    e = e_h * e_h
    den = 1 - t * t * e
    thr = t * (1 - e) / den
    drp = -(kappa * kappa) * e_h / den
    return abs(thr) ** 2, abs(drp) ** 2


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--specs", required=True)
    ap.add_argument("--repo", required=True)
    ap.add_argument("--netlist", default="circuits/add_drop.cir")
    a = ap.parse_args()
    g = load_golden(a.repo)
    env = dict(os.environ, SC_COPYRIGHT_MESSAGE="DISABLE", LC_ALL="C")
    with tempfile.TemporaryDirectory(prefix="ws-scout-oracle-") as td:
        shutil.copytree(os.path.join(a.repo, "circuits"), os.path.join(td, "circuits"))
        os.mkdir(os.path.join(td, "traces"))
        r = subprocess.run([a.specs, "-f", a.netlist, "-o", "traces/oracle"], cwd=td, env=env,
                           capture_output=True, text=True, timeout=600)
        if r.returncode != 0:
            sys.stderr.write(r.stdout[-2000:] + r.stderr[-2000:])
            sys.exit("specs failed with %d" % r.returncode)
        with open(os.path.join(td, "traces", "oracle.vcd")) as f:
            vcd = g.parse_vcd(f.read())
    tl = g.timelines(vcd)
    def pick(suffix):
        for n in tl:
            if n.endswith(suffix):
                return tl[n]
        sys.exit("signal %s not found in VCD; have: %s" % (suffix, sorted(tl)[:20]))
    wl = pick("PROBE{ROOT/out}.wavelength")
    pout = pick("PROBE{ROOT/out}.power")
    pdrop = pick("PROBE{ROOT/drop}.power")
    times = sorted(set(wl) | set(pout) | set(pdrop))
    cur = {"wl": None, "out": None, "drop": None}
    rows = []
    for t in times:
        if t in wl:
            cur["wl"] = fval(wl[t])
        if t in pout:
            cur["out"] = fval(pout[t])
        if t in pdrop:
            cur["drop"] = fval(pdrop[t])
        if None not in cur.values() and cur["wl"] > 0:
            rows.append((t, cur["wl"], cur["out"], cur["drop"]))
    # keep the last sample per wavelength (the converged fixed point)
    last = {}
    for t, lam, po, pd in rows:
        last[lam] = (po, pd)
    print("%-6s %-12s %-12s %-12s %-10s %-12s %-12s %-10s" % ("pt", "lambda_nm", "Pout_specs", "Pout_model", "rel_err", "Pdrop_specs", "Pdrop_model", "rel_err"))
    max_rel_out = max_rel_drop = 0.0
    max_abs_out = max_abs_drop = 0.0
    for i, lam in enumerate(sorted(last)):
        po, pd = last[lam]
        mo, md = model(lam)
        ro = abs(po - mo) / max(mo, 1e-12)
        rd = abs(pd - md) / max(md, 1e-12)
        max_rel_out = max(max_rel_out, ro)
        max_rel_drop = max(max_rel_drop, rd)
        max_abs_out = max(max_abs_out, abs(po - mo))
        max_abs_drop = max(max_abs_drop, abs(pd - md))
        print("%-6d %-12.6f %-12.6g %-12.6g %-10.2e %-12.6g %-12.6g %-10.2e" % (i, lam * 1e9, po, mo, ro, pd, md, rd))
    print("points: %d" % len(last))
    print("through: max abs err %.3e W, max rel err %.3e" % (max_abs_out, max_rel_out))
    print("drop:    max abs err %.3e W, max rel err %.3e" % (max_abs_drop, max_rel_drop))
    print("energy check (max of Pout+Pdrop over points, model): %.6f" % max(sum(model(l)) for l in last))


if __name__ == "__main__":
    main()
