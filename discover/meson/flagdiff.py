#!/usr/bin/env python3
"""Compare per-TU compile flags of a CMake and a Meson build of SPECS (PLAN.md §5 step 2b).

Usage: python3 -I discover/meson/flagdiff.py CMAKE_BUILD/compile_commands.json MESON_BUILD/compile_commands.json

TUs are matched by (product, source file). Flags are compared as sets after normalising build
roots, the generated-parser directory and relative paths; -o/-c/-MD/-MF/-MQ are dropped.
Prints, per product, the flags only CMake has and the flags only Meson has. Exit 1 if a TU is
present in one build only.
"""
import json
import os
import shlex
import sys
from collections import defaultdict

SKIP_WITH_ARG = {"-o", "-MF", "-MQ", "-MT", "-c"}
SKIP = {"-MD", "-MMD", "-MP"}


def product(out):
    """Map an object path to a link product."""
    if "googletest" in out or "gtest" in out or "gmock" in out:
        return None  # third-party test framework: built by each system its own way
    for key, name in (
        ("specs_parser", "specs/parser"),
        ("specs_tb.dir", "specs/tb"),
        ("unit_tests", "unit_tests"),
        ("spx", "spx"),
        ("specs", "specs"),
    ):
        if key in out:
            return name
    return "?"


def load(path):
    root = os.path.dirname(os.path.abspath(path))
    tus = {}
    for e in json.load(open(path)):
        args = e.get("arguments") or shlex.split(e["command"])
        d = e["directory"]
        out, flags, it = None, [], iter(args[1:])
        for a in it:
            if a in SKIP_WITH_ARG:
                v = next(it)
                if a == "-o":
                    out = v
                continue
            if a in SKIP:
                continue
            flags.append(a)
        src = os.path.normpath(os.path.join(d, e["file"]))
        prod = product(out or "")
        if prod is None:
            continue
        # Tb objects are linked into specs in both builds; Meson has no separate tb product.
        if prod == "specs/tb":
            prod = "specs"
        flags = [f for f in flags if os.path.normpath(os.path.join(d, f)) != src]
        norm = []
        it = iter(flags)
        for f in it:
            if f in ("-iquote", "-isystem", "-I") :
                f = f + next(it)
            for p in ("-iquote", "-isystem", "-I"):
                if f.startswith(p) and len(f) > len(p):
                    path = os.path.normpath(os.path.join(d, f[len(p):]))
                    path = path.replace(root + "/generated/parser", "<GEN>")
                    path = path.replace(root + "/src/parser", "<GEN>")
                    path = path.replace(root, "<BUILD>")
                    f = p + " " + path
                    break
            norm.append(f)
        gen = os.path.basename(src) in ("parser.cpp", "scanner.cpp") and root in src
        key = (prod, "<GEN>/" + os.path.basename(src) if gen else src)
        tus[key] = norm
    return tus


def main():
    cm, me = load(sys.argv[1]), load(sys.argv[2])
    status = 0
    for k in sorted(set(cm) ^ set(me)):
        print(f"ONLY IN {'CMAKE' if k in cm else 'MESON'}: {k[0]}: {k[1]}")
        status = 1
    diffs = defaultdict(list)
    for k in sorted(set(cm) & set(me)):
        a, b = set(cm[k]), set(me[k])
        diffs[(k[0], tuple(sorted(a - b)), tuple(sorted(b - a)))].append(k[1])
    for (prod, only_c, only_m), files in sorted(diffs.items()):
        print(f"== {prod}: {len(files)} TU(s)")
        print(f"   only CMake: {' '.join(only_c) or '-'}")
        print(f"   only Meson: {' '.join(only_m) or '-'}")
    return status


if __name__ == "__main__":
    sys.exit(main())
