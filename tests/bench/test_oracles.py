#!/usr/bin/env python3
"""test_oracles.py: SPECS against closed forms (oracles.py), and the CROW generator against the legacy
CROW testbench. Fails on a physics regression; stdlib only (Meson suite `oracle`).

Checks (each one runs specs in a fresh temporary directory and reads the CSV trace table):
    add-drop     circuits/add_drop.cir (.dc, 31 points): through/in and drop/in, complex, vs the
                 closed form (Yariv 2002); the closed form also equals the N = 1 transfer matrix
    crow-dc      paper-style CROW, 3 rings with random lengths (crow_gen.py, seed 1), .dc over 10 nm
                 around 1550 nm: through/in and drop/in vs the transfer matrices (Poon 2004)
    crow-tran    paper-style CROW, 5 rings with random lengths (seed 2), CW source at 1552 nm, .tran: the field at
                 the end of the run vs the transfer-matrix steady state at the source wavelength
    crow-tb      (--legacy-tb) `specs -t ac_crow [--nrings_crow N]` vs the generated crow_tb preset, N = 3
                 and 2: same event times on through and drop, powers within 1e-9 W
Every oracle check also runs its comparison against mutated models (k read as a power ratio, no
dispersion slope, cross coupling without its pi/2 phase) and requires each of them to miss SPECS by more than 10x
the tolerance, so the tolerance is known to catch such a regression.

Usage: python3 -I test_oracles.py --specs build/specs --repo . [--legacy-tb] [--only crow-dc] [-v]
"""
import argparse
import csv
import io
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import crow_gen  # noqa: E402
import oracles  # noqa: E402

MUTANTS = {
    "k read as power": oracles.Model(k_as_power=True),
    "no dispersion slope": oracles.Model(no_dispersion_slope=True),
    "real cross coupling": oracles.Model(cross_phase_rad=0.0),
}
MUTANT_MARGIN = 10.0


class Failure(Exception):
    pass


def run_specs(specs, cwd, args):
    env = dict(os.environ, SC_COPYRIGHT_MESSAGE="DISABLE", LC_ALL="C")
    r = subprocess.run([specs] + args, cwd=cwd, env=env, capture_output=True, text=True, timeout=600)
    if r.returncode != 0:
        raise Failure("specs %s exited with %d:\n%s%s" % (" ".join(args), r.returncode, r.stdout[-2000:],
                                                          r.stderr[-2000:]))
    return r


def run_netlist(specs, repo, netlist_text=None, netlist_path=None):
    """Run a netlist (text, or a path relative to the repo) and return the CSV rows of its probes."""
    with tempfile.TemporaryDirectory(prefix="specs-oracle-") as td:
        os.mkdir(os.path.join(td, "traces"))
        if netlist_path:
            shutil.copytree(os.path.join(repo, "circuits"), os.path.join(td, "circuits"))
            net = netlist_path
        else:
            net = "net.cir"
            with open(os.path.join(td, net), "w") as f:
                f.write(netlist_text)
        run_specs(specs, td, ["-f", net, "-o", "traces/t", "--format", "csv"])
        with open(os.path.join(td, "traces", "t.csv")) as f:
            return list(csv.DictReader(io.StringIO(f.read())))


def last_fields(rows):
    """{signal: {wavelength: complex field}}, the last row of each signal and wavelength."""
    out = {}
    for r in rows:
        out.setdefault(r["signal"], {})[float(r["wavelength_m"])] = complex(float(r["field_re"]),
                                                                             float(r["field_im"]))
    return out


def compare(points, model_fn, tol):
    """points: [(lam, (H_through, H_drop) from SPECS)]; returns the max |H_specs - H_model| over both."""
    err = 0.0
    for lam, got in points:
        want = model_fn(lam)
        for g, w in zip(got, want):
            err = max(err, abs(g - w))
    return err


def check_with_mutants(name, points, model_of, tol, verbose):
    err = compare(points, lambda lam: model_of(lam, oracles.SPECS), tol)
    line = "%s: %d points, max |H_specs - H_model| = %.3e (tolerance %.1e)" % (name, len(points), err, tol)
    if err > tol:
        raise Failure(line)
    print("PASS " + line)
    for mname, m in MUTANTS.items():
        merr = compare(points, lambda lam: model_of(lam, m), tol)
        if verbose:
            print("     mutant '%s': max error %.3e" % (mname, merr))
        if merr <= MUTANT_MARGIN * tol:
            raise Failure("%s: the mutant model '%s' is within %gx the tolerance (%.3e): the check would not "
                          "catch that regression" % (name, mname, MUTANT_MARGIN, merr))


def check_add_drop(a):
    # circuits/add_drop.cir: k = 0.85, half rings 101 * 1.55 um / 2.3, neff 2.3, ng 4.3, 2 dB/cm
    rows = run_netlist(a.specs, a.repo, netlist_path="circuits/add_drop.cir")
    f = last_fields(rows)
    e_in, e_out, e_drop = f["PROBE{ROOT/in}"], f["PROBE{ROOT/out}"], f["PROBE{ROOT/drop}"]
    points = [(lam, (e_out[lam] / e_in[lam], e_drop[lam] / e_in[lam])) for lam in sorted(e_in)]
    if len(points) != 31:
        raise Failure("add-drop: expected 31 sweep points, got %d" % len(points))
    half = 101 * 1.55e-6 / 2.3
    ad = lambda lam, m: oracles.add_drop(lam, 0.85, 0.85, half, 2.3, 4.3, 2.0, model=m)
    tm = lambda lam, m: oracles.crow(lam, [0.85, 0.85], [(half, half)], 2.3, 4.3, 2.0, model=m)
    for lam, _ in points:
        if max(abs(x - y) for x, y in zip(ad(lam, oracles.SPECS), tm(lam, oracles.SPECS))) > 1e-12:
            raise Failure("add-drop: closed form and N = 1 transfer matrix differ at %g m" % lam)
    check_with_mutants("add-drop", points, ad, a.tol_dc, a.verbose)


def crow_points(f, power):
    e_in = power ** 0.5
    thr, drp = f["ROOT/PROBE1"], f["ROOT/PROBE2"]
    return [(lam, (thr[lam] / e_in, drp[lam] / e_in)) for lam in sorted(thr)]


def crow_model(p):
    kappas = [p.kp ** 0.5] * (p.N + 1)
    halves = crow_gen.ring_lengths(p)
    return lambda lam, m: oracles.crow(lam, kappas, halves, p.neff, p.ng, p.att, model=m)


def check_crow_dc(a):
    p = crow_gen.parse_args(["3", "--seed", "1", "--analysis", "dc", "--power", "1", "--dc-start", "1545e-9",
                             "--dc-stop", "1555e-9", "--dc-step", "0.1e-9", "--abstol", "1e-12",
                             "--reltol", "1e-8"])
    points = crow_points(last_fields(run_netlist(a.specs, a.repo, netlist_text=crow_gen.generate(p))), p.power)
    if len(points) != 101:
        raise Failure("crow-dc: expected 101 sweep points, got %d" % len(points))
    check_with_mutants("crow-dc", points, crow_model(p), a.tol_dc, a.verbose)


def check_crow_tran(a):
    # Off the 1.55 um reference wavelength, so that the dispersion slope matters
    p = crow_gen.parse_args(["5", "--seed", "2", "--source", "cw", "--wl", "1552e-9", "--power", "1",
                             "--duration", "2e-9",
                             "--abstol", "1e-12", "--reltol", "1e-8"])
    points = crow_points(last_fields(run_netlist(a.specs, a.repo, netlist_text=crow_gen.generate(p))), p.power)
    if len(points) != 1:
        raise Failure("crow-tran: expected one wavelength, got %d" % len(points))
    check_with_mutants("crow-tran", points, crow_model(p), a.tol_tran, a.verbose)


def vcd_module(repo):
    import importlib.util
    spec = importlib.util.spec_from_file_location("run_golden", os.path.join(repo, "tests", "golden",
                                                                             "run_golden.py"))
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def check_crow_tb(a):
    g = vcd_module(a.repo)
    for n in (3, 2):
        with tempfile.TemporaryDirectory(prefix="specs-crowtb-") as td:
            os.mkdir(os.path.join(td, "traces"))
            run_specs(a.specs, td, ["-t", "ac_crow", "--nrings_crow", str(n)])
            with open(os.path.join(td, "traces", "crow_tb.vcd")) as f:
                vcd = g.parse_vcd(f.read())
        tl = g.timelines(vcd)
        # The testbench's VCD counts time in units of the engine timescale (100 fs here)
        tb = {}
        for probe in ("pthrough", "pcross"):
            tb[probe] = {round(t): float(v[1:] if v[:1] in "rR" else v)
                         for t, v in tl["SystemC.%s.power" % probe].items()}
        p = crow_gen.parse_args([str(n), "--preset", "crow_tb"])
        rows = run_netlist(a.specs, a.repo, netlist_text=crow_gen.generate(p))
        gen = {"ROOT/PROBE1": {}, "ROOT/PROBE2": {}}
        for r in rows:
            gen[r["signal"]][round(float(r["time_s"]) * 1e13)] = float(r["power_W"])
        for probe, sig in (("pthrough", "ROOT/PROBE1"), ("pcross", "ROOT/PROBE2")):
            t_tb, t_gen = tb[probe], gen[sig]
            # The testbench traces the initial 0 of the drop port at t = 0; the netlist probe has no event there
            extra = sorted(set(t_tb) - set(t_gen))
            if extra and not (extra == [0] and t_tb[0] == 0.0):
                raise Failure("crow-tb N=%d %s: event times only in the testbench: %s" % (n, probe, extra[:5]))
            if set(t_gen) - set(t_tb):
                raise Failure("crow-tb N=%d %s: event times only in the netlist: %s"
                              % (n, probe, sorted(set(t_gen) - set(t_tb))[:5]))
            err = max(abs(t_gen[t] - t_tb[t]) for t in t_gen)
            line = "crow-tb N=%d %s: %d events, same times, max |dP| = %.3e W (tolerance 1e-9)" % (
                n, probe, len(t_gen), err)
            if err > 1e-9:
                raise Failure(line)
            print("PASS " + line)


CHECKS = [("add-drop", check_add_drop), ("crow-dc", check_crow_dc), ("crow-tran", check_crow_tran),
          ("crow-tb", check_crow_tb)]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--specs", required=True)
    ap.add_argument("--repo", required=True)
    ap.add_argument("--legacy-tb", action="store_true", help="also compare with `specs -t ac_crow`")
    ap.add_argument("--only", action="append", choices=[c for c, _ in CHECKS])
    ap.add_argument("--tol-dc", type=float, default=1e-5, help="max |H_specs - H_model| of .dc checks")
    ap.add_argument("--tol-tran", type=float, default=1e-5, help="same for the end of a .tran")
    ap.add_argument("-v", "--verbose", action="store_true", help="print the mutants' errors")
    a = ap.parse_args()
    a.specs = os.path.abspath(a.specs)
    a.repo = os.path.abspath(a.repo)
    failed = 0
    for name, fn in CHECKS:
        if a.only and name not in a.only:
            continue
        if name == "crow-tb" and not a.legacy_tb and not a.only:
            continue
        try:
            fn(a)
        except Failure as e:
            failed += 1
            print("FAIL %s" % e)
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
