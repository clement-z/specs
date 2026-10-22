#!/usr/bin/env python3
"""Shows which edits tests/golden/run_golden.py (default --print-slack 1) lets through in text outputs.
usage: python3 -I a5-golden-slack-probe.py <repo-root>
Expected today: every line printed as PASS below is accepted although the number changed by 100 % or more."""
import importlib.util, sys
repo = sys.argv[1]
spec = importlib.util.spec_from_file_location("rg", repo + "/tests/golden/run_golden.py")
rg = importlib.util.module_from_spec(spec); spec.loader.exec_module(rg)
tol = rg.Tol(1e-9, 1e-12, 1.0)          # the defaults of `check`
pairs = [
    ("- abstol: 1e-08", "- abstol: 2e-08"),
    ("- abstol: 1e-08", "- abstol: 1e-07"),
    ("- abstol: 1e-08", "- abstol: 0"),
    ("- default reltol: 0.0001", "- default reltol: 0.0002"),
    ("- default reltol: 0.0001", "- default reltol: 0.001"),
    ("- default reltol: 0.0001", "- default reltol: 0.0000"),
    ("pulse 2.0 W", "pulse 2.1 W"),
]
bad = 0
for a, b in pairs:
    ok = not rg.compare_text(a, b, tol, "stdout")
    bad += ok
    print("%-4s %r -> %r" % ("PASS" if ok else "fail", a, b))
sys.exit(1 if bad else 0)
