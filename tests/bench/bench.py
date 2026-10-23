#!/usr/bin/env python3
"""bench.py: time SPECS on generated CROW netlists (crow_gen.py) and record, per case, user/wall time,
peak RSS, VCD size and the event counters of --stats. Results go to stdout (a markdown table), to
--out (JSON) and, appended as one JSON line per run, to a local history file (default
tests/bench/history.jsonl, ignored by git: absolute times are machine-dependent and are not asserted
or published; test_bench.py asserts machine-independent ratios).

Usage: python3 -I bench.py --specs build/specs [--quick] [--repeat 3] [--out results.json]
                           [--history FILE | --no-history] [--only A-scaling]

Cases (label prefix): A-scaling (N = 3..300, 2 probes), B-traceall (every net probed), C-notrace (no
probe), D-wdm (WDM channels), E-tscale (1 fs resolution), F-tol (tighter tolerances): the matrix of the
scout's bench_crow.py (sprint 2026-10, discover/scout/bench/RESULTS.md).
"""
import argparse
import datetime
import json
import os
import platform
import statistics
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import crow_gen  # noqa: E402

DEFAULT_HISTORY = os.path.join(HERE, "history.jsonl")


def run_once(specs, netlist_text, timeout, trace=True):
    """Run one netlist in a fresh directory; returns a dict of measurements (this child's rusage only)."""
    with tempfile.TemporaryDirectory(prefix="specs-bench-") as td:
        os.mkdir(os.path.join(td, "traces"))
        with open(os.path.join(td, "crow.cir"), "w") as f:
            f.write(netlist_text)
        env = dict(os.environ, SC_COPYRIGHT_MESSAGE="DISABLE", LC_ALL="C")
        args = [specs, "-f", "crow.cir", "--stats-file", "traces/crow.stats.json"]
        args += ["-o", "traces/crow"] if trace else ["--no-trace"]
        t0 = time.perf_counter()
        with open(os.path.join(td, "out.txt"), "w") as out:
            p = subprocess.Popen(args, cwd=td, env=env, stdout=out, stderr=subprocess.STDOUT)
            status = "ok"
            deadline = t0 + timeout
            while True:
                pid, st, ru = os.wait4(p.pid, os.WNOHANG)
                if pid:
                    break
                if time.perf_counter() > deadline:
                    p.kill()
                    pid, st, ru = os.wait4(p.pid, 0)
                    status = "timeout"
                    break
                time.sleep(0.005)
        p.returncode = os.waitstatus_to_exitcode(st)
        wall = time.perf_counter() - t0
        res = dict(status=status if p.returncode == 0 or status == "timeout" else "error", rc=p.returncode,
                   wall_s=round(wall, 3), user_s=round(ru.ru_utime, 3), sys_s=round(ru.ru_stime, 3),
                   maxrss_mb=round(ru.ru_maxrss / 1024, 1))
        vcd = os.path.join(td, "traces", "crow.vcd")
        res["vcd_mb"] = round(os.path.getsize(vcd) / 1e6, 2) if os.path.exists(vcd) else 0.0
        st_path = os.path.join(td, "traces", "crow.stats.json")
        if os.path.exists(st_path):
            with open(st_path) as f:
                tot = json.load(f)["totals"]
            res.update(processed=tot["processed"], emitted=tot["emitted"], scheduled=tot["scheduled"])
        return res


def run_case(specs, label, gen_args, timeout, repeat=1, trace=True):
    """Median user/wall time over `repeat` runs; counters and sizes of the last run."""
    text = crow_gen.generate(crow_gen.parse_args(gen_args))
    runs = [run_once(specs, text, timeout, trace) for _ in range(repeat)]
    res = dict(runs[-1])
    for k in ("user_s", "wall_s", "sys_s"):
        res[k] = round(statistics.median(r[k] for r in runs), 3)
    res["maxrss_mb"] = max(r["maxrss_mb"] for r in runs)
    if any(r["status"] != "ok" for r in runs):
        res["status"] = next(r["status"] for r in runs if r["status"] != "ok")
    if res.get("processed") and res["user_s"] > 0:
        res["ns_per_processed"] = round(1e9 * res["user_s"] / res["processed"], 1)
    res.update(label=label, gen_args=gen_args, repeat=repeat)
    return res


def cases(quick):
    out = []
    # A: paper-like scaling, 2 explicit probes, no auto-probes
    for n in ([3, 10, 30] if quick else [3, 10, 30, 100, 300]):
        out.append(("A-scaling N=%d probes=2" % n, [str(n), "--traceall", "0", "--probes", "1"]))
    # B: every net auto-probed
    for n in [3, 10, 30]:
        out.append(("B-traceall N=%d" % n, [str(n), "--traceall", "1", "--probes", "1"]))
    # C: no tracing at all (engine only)
    for n in [3, 30]:
        out.append(("C-notrace N=%d" % n, [str(n), "--traceall", "0", "--probes", "0"]))
    # D: WDM channels, each wavelength is a separate event stream
    for n, nwl in [(3, 1), (3, 4), (3, 16), (10, 4)]:
        out.append(("D-wdm N=%d nwl=%d" % (n, nwl), [str(n), "--traceall", "0", "--probes", "1", "--nwl", str(nwl)]))
    # E: time resolution 1 fs vs 100 fs (paper Fig. 3b)
    out.append(("E-tscale N=3 1fs", ["3", "--traceall", "0", "--probes", "1", "--tscale", "-15"]))
    # F: tolerance sensitivity
    out.append(("F-tol N=10 abstol=1e-10", ["10", "--traceall", "0", "--probes", "1", "--abstol", "1e-10",
                                            "--reltol", "1e-6"]))
    return out


def machine():
    cpu = platform.processor() or platform.machine()
    try:
        with open("/proc/cpuinfo") as f:
            for line in f:
                if line.startswith("model name"):
                    cpu = line.split(":", 1)[1].strip()
                    break
    except OSError:
        pass
    return dict(cpu=cpu, nproc=os.cpu_count(), system=platform.system(), python=platform.python_version())


def specs_version(specs):
    try:
        r = subprocess.run([specs, "--version"], capture_output=True, text=True, timeout=30,
                           env=dict(os.environ, SC_COPYRIGHT_MESSAGE="DISABLE"))
        return r.stdout.strip().splitlines()[0] if r.stdout.strip() else None
    except (OSError, subprocess.SubprocessError):
        return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--specs", required=True)
    ap.add_argument("--out", help="write the results as JSON")
    ap.add_argument("--history", default=DEFAULT_HISTORY, help="append the run here (default: %(default)s)")
    ap.add_argument("--no-history", action="store_true")
    ap.add_argument("--timeout", type=int, default=600)
    ap.add_argument("--repeat", type=int, default=1, help="runs per case (median time)")
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--only", action="append", help="label prefix (repeatable)")
    ap.add_argument("--note", help="free text stored in the history entry")
    a = ap.parse_args()
    specs = os.path.abspath(a.specs)
    results = []
    for label, gen in cases(a.quick):
        if a.only and not any(label.startswith(o) for o in a.only):
            continue
        res = run_case(specs, label, gen, a.timeout, a.repeat)
        results.append(res)
        print(json.dumps(res), file=sys.stderr, flush=True)
    print("| case | status | user s | wall s | peak RSS MB | VCD MB | processed | ns / processed |")
    print("|---|---|---|---|---|---|---|---|")
    for r in results:
        print("| %s | %s | %s | %s | %s | %s | %s | %s |" % (r["label"], r["status"], r["user_s"], r["wall_s"],
                                                            r["maxrss_mb"], r["vcd_mb"], r.get("processed"),
                                                            r.get("ns_per_processed")))
    if a.out:
        with open(a.out, "w") as f:
            json.dump(results, f, indent=1)
    if not a.no_history:
        entry = dict(date_utc=datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
                     specs_version=specs_version(specs), machine=machine(), repeat=a.repeat, note=a.note,
                     cases=results)
        with open(a.history, "a") as f:
            f.write(json.dumps(entry) + "\n")
        print("appended to %s" % a.history, file=sys.stderr)
    sys.exit(0 if all(r["status"] == "ok" for r in results) else 1)


if __name__ == "__main__":
    main()
