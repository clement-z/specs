#!/usr/bin/env python3
"""Test of pyspecs/runtable.py (F-02): reads the run tables that specs writes for the .step/.mc plans of
tests/golden/netlists/f02-step-mc.cir and f02-step-failures.cir, and compares the rows of a run with the
.npz of `specs --run K`. Needs numpy and pandas (skipped, exit 77, without them).

    python3 -I tests/tools/test_run_table.py --specs build/specs --repo .
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile

FAILURES = []


def check(cond, msg):
    if not cond:
        FAILURES.append(msg)
        print("FAIL: " + msg)


def run(cmd, cwd):
    return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--specs", required=True)
    ap.add_argument("--repo", required=True)
    args = ap.parse_args()
    try:
        import numpy as np
        import pandas  # noqa: F401
    except ImportError as e:
        print("SKIP: %s" % e)
        return 77
    sys.path.insert(0, os.path.abspath(args.repo))
    from pyspecs.runtable import RunTable

    specs = os.path.abspath(args.specs)
    netlists = os.path.join(os.path.abspath(args.repo), "tests", "golden", "netlists")
    with tempfile.TemporaryDirectory() as tmp:
        os.mkdir(os.path.join(tmp, "traces"))
        mc = os.path.join(netlists, "f02-step-mc.cir")
        r = run([specs, "-j", "3", "-o", "traces/mc", "-f", mc], tmp)
        check(r.returncode == 0, "plan f02-step-mc: exit %d: %s" % (r.returncode, r.stderr))
        t = RunTable(os.path.join(tmp, "traces", "mc"))
        check(t.params == ["n2"] and t.drawn == ["dL"], "parameters %r %r" % (t.params, t.drawn))
        runs = t.runs
        check(list(runs["run"]) == list(range(6)), "runs %r" % list(runs["run"]))
        check((runs["status"] == "ok").all(), "statuses %r" % list(runs["status"]))
        check(list(runs["n2"]) == [2.35] * 3 + [2.36] * 3, "n2 %r" % list(runs["n2"]))
        # Monte-Carlo run k: the same seed and draws at both step points
        check(list(runs["seed"][:3]) == list(runs["seed"][3:]), "seeds")
        check(list(runs["dL"][:3]) == list(runs["dL"][3:]), "dL")
        check(runs["seed"].dtype == np.uint64, "seed dtype %s" % runs["seed"].dtype)
        # columns are memory-mapped
        check(isinstance(t.column("power_W"), np.memmap), "power_W is not memory-mapped")

        sel = t.select(n2=2.36)
        check(list(sel["run"]) == [3, 4, 5], "select n2=2.36: %r" % list(sel["run"]))
        sel = t.select(dL=lambda x: x > 0)
        check(list(sel["run"]) == [0, 3], "select dL > 0: %r" % list(sel["run"]))
        df = t.data(n2=2.36)
        check(len(df) == 3 and list(df["run"]) == [3, 4, 5], "data n2=2.36: %r" % df)
        check(list(df.columns[:5]) == ["run", "seed", "mc_run", "n2", "dL"], "columns %r" % list(df.columns))
        check((df["signal"] == "ROOT/PROBE1").all(), "signal names %r" % list(df["signal"]))
        check(len(t.data(runs=[1], signals="ROOT/NONE")) == 0, "signal filter")
        n = sum(1 for _ in t.iter_runs())
        check(n == 6, "iter_runs: %d runs" % n)
        draws = t.draws(runs=[4])
        check(len(draws) == 2 and set(draws["name"]) == {"ROOT/.param/dL#0", "ROOT/X1/.param/Lr#0"},
              "draws of run 4: %r" % draws)
        check(draws["value"][0] == runs.loc[4, "dL"], "draw of dL == runs.csv dL")
        # every drawn value of runs.csv is the double of the draw table, bit for bit
        all_draws = t.draws()
        dl = all_draws[all_draws["name"] == "ROOT/.param/dL#0"].set_index("run")["value"]
        check((dl == runs["dL"]).all(), "runs.csv dL != draw_value.npy")
        check(runs["dL"].dtype == np.float64, "dL dtype %s" % runs["dL"].dtype)

        # the rows of run 4 are those of `specs --run 4`, bit for bit
        r = run([specs, "--run", "4", "--format", "npz", "-o", "traces/r4", "-f", mc], tmp)
        check(r.returncode == 0, "--run 4: exit %d: %s" % (r.returncode, r.stderr))
        single = np.load(os.path.join(tmp, "traces", "r4.npz"))
        rows = t.run_data(4)
        for c in ["time_s", "power_W", "field_re", "field_im", "wavelength_m"]:
            check(np.array_equal(single[c], rows[c].to_numpy()), "--run 4 column %s differs" % c)

        # a failed run: recorded, the others ok; select(status=None) sees it
        fail = os.path.join(netlists, "f02-step-failures.cir")
        r = run([specs, "-o", "traces/fail", "-f", fail], tmp)
        check(r.returncode == 1, "plan f02-step-failures: exit %d" % r.returncode)
        tf = RunTable(os.path.join(tmp, "traces", "fail.runs"))
        check(list(tf.runs["status"]) == ["ok", "failed", "ok"], "statuses %r" % list(tf.runs["status"]))
        check(tf.runs.loc[1, "message"].startswith("Error: .op: the operating point did not converge"),
              "message %r" % tf.runs.loc[1, "message"])
        check(list(tf.select(status=None)["run"]) == [0, 1, 2], "select status=None")
        check(len(tf.data()) == 0, "an .op plan has no table rows")
        # no .mc: mc_run is empty, read as NaN (numeric)
        check(tf.runs["mc_run"].isna().all() and tf.runs["M"].dtype.kind in "iuf",
              "mc_run %r M %s" % (list(tf.runs["mc_run"]), tf.runs["M"].dtype))
        check(len(tf.select(M=2000)) == 2, "select M=2000")
        shutil.rmtree(os.path.join(tmp, "traces"))

    if FAILURES:
        print("%d failure(s)" % len(FAILURES))
        return 1
    print("OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
