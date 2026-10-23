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


def check_field(tool):
    """Background field: close to a device's activity next to it, 0 far from every device, and a point
    between two close devices lies between the two activities."""
    import importlib.util
    import numpy as np
    spec = importlib.util.spec_from_file_location("activity_map_tool", tool)
    am = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(am)
    width = 100.0
    sigma = am.field_sigma([width, width, width], 0.5)
    check(abs(sigma - 50.0) < 1e-9, "field_sigma(width 100, spread 0.5) = %g, expected 50" % sigma)
    # Two devices (activities 10 and 0) 150 apart, a third (activity 10) 1500 further on.
    centers, acts = [(0.0, 0.0), (150.0, 0.0), (1650.0, 0.0)], [10.0, 0.0, 10.0]
    bbox = (-100.0, -50.0, 1750.0, 50.0)
    grid = am.field_grid(centers, acts, bbox, sigma, 10.0, np)
    ny, nx = grid.shape

    def at(x):
        return grid[ny // 2, int(round((x - bbox[0]) / (bbox[2] - bbox[0]) * (nx - 1)))]

    check(at(0.0) > 0.8, "field next to a device is %.3f, expected close to 1" % at(0.0))
    check(0.0 < at(75.0) < at(0.0), "field between close devices is %.3f" % at(75.0))
    check(at(900.0) < 1e-6, "field far from every device is %.2g, expected 0" % at(900.0))
    check(float(grid.max()) <= 1.0, "field above 1 on the colour scale")


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
            # Option --field-spread: accepted, and a non-positive value is a usage error (exit 2).
            r = run([sys.executable, "-I", tool, cj, sj, "-o", "traces/spread", "--format", "png",
                     "--field-spread", "1.5"], tmp)
            check(r.returncode == 0, "--field-spread 1.5: exit code %d: %s" % (r.returncode, r.stderr))
            r = run([sys.executable, "-I", tool, cj, sj, "-o", "traces/spread", "--field-spread", "0"], tmp)
            check(r.returncode == 2, "--field-spread 0: exit code %d, expected 2" % r.returncode)
            check_field(tool)
    return finish()


def finish():
    if FAILURES:
        print("%d check(s) failed" % len(FAILURES))
        return 1
    print("activity-map: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
