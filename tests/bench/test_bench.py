#!/usr/bin/env python3
"""test_bench.py: machine-independent checks on the CROW benchmark (Meson suite `bench`): ratios only,
no absolute time (decisions of 2026-10-23: absolute times wait for a fixed benchmark rig; bench.py
records them locally).

Checks (paper-style CROW from crow_gen.py, 2 pulses, no trace file):
    plateau-events  processed(N = 300) / processed(N = 30) <= 1.05: beyond about 30 rings the light is
                    below the tolerances, so the extra rings get no event (measured 1.000)
    wdm-events      processed(N = 3, 4 channels) / (4 x processed(N = 3, 1 channel)) in [0.9, 1.25]: the
                    channels are independent event streams (measured 1.016)
    plateau-time    user time t(N = 300) / t(N = 30) <= 2.0, median of 3 runs each (measured 1.1-1.3 on a
                    release build); skipped with --no-timing (debug and sanitizer builds, where the
                    elaboration of 300 rings weighs differently)

Usage: python3 -I test_bench.py --specs build/specs [--no-timing]
"""
import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import bench  # noqa: E402

BASE = ["--pulses", "2", "--probes", "0", "--traceall", "0"]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--specs", required=True)
    ap.add_argument("--no-timing", action="store_true", help="only the event-count ratios")
    ap.add_argument("--timeout", type=int, default=600)
    a = ap.parse_args()
    specs = os.path.abspath(a.specs)
    repeat = 1 if a.no_timing else 3
    failed = []

    def case(args, rep=1):
        r = bench.run_case(specs, " ".join(args), args + BASE, a.timeout, repeat=rep, trace=False)
        if r["status"] != "ok" or "processed" not in r:
            sys.exit("FAIL specs on crow_gen.py %s: %s (rc %s)" % (" ".join(args), r["status"], r["rc"]))
        return r

    def report(name, ok, text):
        print("%s %s: %s" % ("PASS" if ok else "FAIL", name, text))
        if not ok:
            failed.append(name)

    n30, n300 = case(["30"], repeat), case(["300"], repeat)
    ratio = n300["processed"] / n30["processed"]
    report("plateau-events", ratio <= 1.05, "processed %d (N=300) / %d (N=30) = %.4f (limit 1.05)"
           % (n300["processed"], n30["processed"], ratio))

    w1, w4 = case(["3", "--nwl", "1"]), case(["3", "--nwl", "4"])
    ratio = w4["processed"] / (4 * w1["processed"])
    report("wdm-events", 0.9 <= ratio <= 1.25, "processed %d (4 channels) / (4 x %d) = %.4f (limits 0.9, 1.25)"
           % (w4["processed"], w1["processed"], ratio))

    if a.no_timing:
        print("SKIP plateau-time: --no-timing")
    else:
        ratio = n300["user_s"] / max(n30["user_s"], 1e-3)
        report("plateau-time", ratio <= 2.0, "user %.3f s (N=300) / %.3f s (N=30) = %.2f (limit 2.0, median of %d)"
               % (n300["user_s"], n30["user_s"], ratio, repeat))
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
