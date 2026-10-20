#!/usr/bin/env python3
"""bench_crow.py: time SPECS on generated CROW netlists (see crow_gen.py) and record wall time,
peak RSS and VCD size for a small matrix of cases. Writes JSON + a markdown table.

Usage: python3 -I bench_crow.py --specs /path/to/specs --out results.json [--quick]
"""
import argparse
import json
import os
import re
import resource
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))


def run_case(specs, gen_args, timeout, label):
    with tempfile.TemporaryDirectory(prefix="ws-scout-bench-") as td:
        os.mkdir(os.path.join(td, "traces"))
        net = os.path.join(td, "crow.cir")
        with open(net, "w") as f:
            subprocess.run([sys.executable, "-I", os.path.join(HERE, "crow_gen.py")] + gen_args,
                           stdout=f, check=True)
        env = dict(os.environ, SC_COPYRIGHT_MESSAGE="DISABLE", LC_ALL="C")
        r0 = resource.getrusage(resource.RUSAGE_CHILDREN)
        t0 = time.perf_counter()
        status = "ok"
        try:
            r = subprocess.run([specs, "-f", "crow.cir", "-o", "traces/crow"], cwd=td, env=env,
                               capture_output=True, text=True, timeout=timeout)
            rc = r.returncode
            out = r.stdout
        except subprocess.TimeoutExpired as e:
            rc, out, status = None, (e.stdout or b"").decode(errors="ignore") if isinstance(e.stdout, bytes) else (e.stdout or ""), "timeout"
        wall = time.perf_counter() - t0
        r1 = resource.getrusage(resource.RUSAGE_CHILDREN)
        rss_kb = r1.ru_maxrss  # peak over all children so far (monotonic); report per case below
        vcd = os.path.join(td, "traces", "crow.vcd")
        vcd_bytes = os.path.getsize(vcd) if os.path.exists(vcd) else 0
        m = re.search(r"Simulated (\S+ \S+)", out or "")
        simulated = m.group(1) if m else None
        return dict(label=label, gen_args=gen_args, status=status, rc=rc, wall_s=round(wall, 3),
                    maxrss_mb=round(rss_kb / 1024, 1), vcd_mb=round(vcd_bytes / 1e6, 2), simulated=simulated,
                    user_s=round(r1.ru_utime - r0.ru_utime, 3), sys_s=round(r1.ru_stime - r0.ru_stime, 3))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--specs", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--timeout", type=int, default=600)
    ap.add_argument("--quick", action="store_true")
    a = ap.parse_args()
    cases = []
    # A: paper-like scaling, 2 explicit probes, no auto-probes
    for n in ([3, 10, 30] if a.quick else [3, 10, 30, 100, 300]):
        cases.append(("A-scaling N=%d probes=2" % n, [str(n), "--traceall", "0", "--probes", "1"]))
    # B: every net auto-probed (default traceall=1)
    for n in [3, 10, 30]:
        cases.append(("B-traceall N=%d" % n, [str(n), "--traceall", "1", "--probes", "1"]))
    # C: no tracing at all (engine only)
    for n in [3, 30]:
        cases.append(("C-notrace N=%d" % n, [str(n), "--traceall", "0", "--probes", "0"]))
    # D: WDM channels, each wavelength is a separate event stream
    for n, nwl in [(3, 1), (3, 4), (3, 16), (10, 4)]:
        cases.append(("D-wdm N=%d nwl=%d" % (n, nwl), [str(n), "--traceall", "0", "--probes", "1", "--nwl", str(nwl)]))
    # E: time resolution 1 fs vs 100 fs (paper Fig. 3b)
    cases.append(("E-tscale N=3 1fs", ["3", "--traceall", "0", "--probes", "1", "--tscale", "-15"]))
    # F: tolerance sensitivity
    cases.append(("F-tol N=10 abstol=1e-10", ["10", "--traceall", "0", "--probes", "1", "--abstol", "1e-10", "--reltol", "1e-6"]))
    results = []
    for label, gen in cases:
        res = run_case(a.specs, gen, a.timeout, label)
        results.append(res)
        print(json.dumps(res), flush=True)
    with open(a.out, "w") as f:
        json.dump(results, f, indent=1)
    md = a.out.rsplit(".", 1)[0] + ".md"
    with open(md, "w") as f:
        f.write("| case | status | wall s | user s | peak RSS MB (cumulative max) | VCD MB | simulated |\n|---|---|---|---|---|---|---|\n")
        for r in results:
            f.write("| %s | %s | %s | %s | %s | %s | %s |\n" % (r["label"], r["status"], r["wall_s"], r["user_s"], r["maxrss_mb"], r["vcd_mb"], r["simulated"]))
    print("wrote", a.out, md)


if __name__ == "__main__":
    main()
