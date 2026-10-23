#!/usr/bin/env python3
"""Test of utils/activity_map.py: DOT content (always) and the SVG/PNG rendering (when numpy, matplotlib
and dot are available).

    python3 -I tests/tools/test_activity_map.py --specs build/specs --repo .
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

FAILURES = []


def check(cond, msg):
    if not cond:
        FAILURES.append(msg)
        print("FAIL: " + msg)


def short(name):
    return name[5:].rsplit("/", 1)[-1] if name.startswith("ROOT/") else name.rsplit("/", 1)[-1]


def run(cmd, cwd):
    return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--specs", required=True)
    ap.add_argument("--repo", required=True)
    args = ap.parse_args()
    specs = os.path.abspath(args.specs)
    repo = os.path.abspath(args.repo)
    tool = os.path.join(repo, "utils", "activity_map.py")
    netlist = os.path.join(repo, "tests", "tools", "activity_small.cir")

    with tempfile.TemporaryDirectory() as tmp:
        shutil.copy(netlist, tmp)
        os.mkdir(os.path.join(tmp, "traces"))
        env_cmd = [specs, "-f", "activity_small.cir", "-o", "traces/run", "--json", "traces/run.json",
                   "--stats=ports,devices"]
        env = dict(os.environ, SC_COPYRIGHT_MESSAGE="DISABLE")
        r = subprocess.run(env_cmd, cwd=tmp, capture_output=True, text=True, env=env)
        check(r.returncode == 0, "specs exit code %d: %s" % (r.returncode, r.stderr[-500:]))
        if FAILURES:
            return finish()
        cj, sj = "traces/run.json", "traces/run.stats.json"
        with open(os.path.join(tmp, cj)) as f:
            names = [e["name"] for e in json.load(f)["elements"]]
        with open(os.path.join(tmp, sj)) as f:
            stats = json.load(f)

        # DOT part (standard library only).
        r = run([sys.executable, "-I", tool, cj, sj, "-o", "traces/map", "--dot-only"], tmp)
        check(r.returncode == 0, "activity_map --dot-only exit code %d: %s" % (r.returncode, r.stderr))
        dot = open(os.path.join(tmp, "traces/map.dot")).read()
        check(not os.path.exists(os.path.join(tmp, "traces/map.svg")), "--dot-only wrote an SVG")
        node_of = {}
        for n in names:
            m = re.search(r'^\s*(n\d+) \[label="%s\\n([^"]*)".*?fillcolor="(#[0-9a-f]{6})"'
                          % re.escape(short(n)), dot, re.M)
            check(m is not None, "no DOT node for %s" % n)
        # Node ids follow the element order of the circuit file.
        fills = dict(re.findall(r'^\s*(n\d+) \[.*?fillcolor="(#[0-9a-f]{6})"', dot, re.M))
        check(len(fills) == len(names), "expected %d device nodes, found %d" % (len(names), len(fills)))
        for cl in ("ROOT/X1", "ROOT/X2", "ROOT/X1/X1", "ROOT/X1/X2", "ROOT/X2/X1", "ROOT/X2/X2"):
            check('subgraph "cluster_%s"' % cl in dot, "missing cluster %s" % cl)
        # Nesting: the cluster of X1/X1 is inside the one of X1.
        i_outer, i_inner = dot.find('"cluster_ROOT/X1"'), dot.find('"cluster_ROOT/X1/X1"')
        i_next = dot.find('"cluster_ROOT/X2"')
        check(0 <= i_outer < i_inner < i_next, "cluster ROOT/X1/X1 is not nested inside ROOT/X1")
        wg = fills["n%d" % names.index("ROOT/WG0")]
        probe = fills["n%d" % names.index("ROOT/PROBE1")]
        busiest = max(stats["ports"], key=lambda p: p["processed"])["processed"]
        check(busiest > 0, "no processed events in the stats")
        check(probe == "#ffffff", "probe (0 activity) is not white: %s" % probe)
        check(wg != "#ffffff", "waveguide with light is white")

        # Error exit: stats without the ports section.
        bad = dict(stats)
        del bad["ports"]
        with open(os.path.join(tmp, "bad.stats.json"), "w") as f:
            json.dump(bad, f)
        r = run([sys.executable, "-I", tool, cj, "bad.stats.json", "-o", "traces/bad", "--dot-only"], tmp)
        check(r.returncode == 2, "stats without ports: exit code %d, expected 2" % r.returncode)
        check("--stats=ports" in r.stderr, "no hint about --stats=ports in: %r" % r.stderr)
        r = run([sys.executable, "-I", tool, cj, "missing.json", "-o", "traces/bad", "--dot-only"], tmp)
        check(r.returncode == 2, "missing file: exit code %d, expected 2" % r.returncode)

        # Rendering part.
        why = []
        for mod in ("numpy", "matplotlib"):
            try:
                __import__(mod)
            except ImportError:
                why.append(mod + " not importable")
        if shutil.which("dot") is None:
            why.append("dot not on PATH")
        if why:
            print("SKIP render: " + ", ".join(why))
        else:
            r = run([sys.executable, "-I", tool, cj, sj, "-o", "traces/map", "--suppression"], tmp)
            check(r.returncode == 0, "render exit code %d: %s" % (r.returncode, r.stderr))
            svg = os.path.join(tmp, "traces/map.svg")
            png = os.path.join(tmp, "traces/map.png")
            check(os.path.exists(svg) and os.path.exists(png), "SVG or PNG missing")
            if os.path.exists(svg):
                try:
                    ET.parse(svg)
                except ET.ParseError as e:
                    check(False, "SVG does not parse: %s" % e)
                text = open(svg).read()
                for n in names:
                    check(">%s<" % short(n) in text, "SVG has no text %s" % short(n))
                check("processed: %s" % stats["totals"]["processed"] in text, "SVG has no totals text")
                check("suppressed / processed" in text, "SVG has no suppression colorbar label")
            if os.path.exists(png):
                check(open(png, "rb").read(8) == b"\x89PNG\r\n\x1a\n", "PNG signature wrong")
    return finish()


def finish():
    if FAILURES:
        print("%d check(s) failed" % len(FAILURES))
        return 1
    print("activity-map: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
