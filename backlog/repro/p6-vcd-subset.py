#!/usr/bin/env python3
"""vcd_subset.py NEW.vcd OLD.vcd REPO: every probe variable of NEW must exist in OLD with a bit-identical
waveform (piecewise constant, tolerance 0). Prints the compared variables."""
import sys
sys.path.insert(0, sys.argv[3] + "/tests/golden")
import run_golden as rg
a = rg.parse_vcd(rg.normalise_vcd(open(sys.argv[1]).read()))
b = rg.parse_vcd(rg.normalise_vcd(open(sys.argv[2]).read()))
ta, tb = rg.timelines(a), rg.timelines(b)
bad, n = [], 0
for name in sorted(a.vars):
    if name.split(".")[1].split(" ")[0] in ("abstol", "reltol", "resolution_multiplier", "engine_timescale", "simulation_mode",
                              "analysis_type", "trace_all_optical_nets", "verbose_component_initialization"):
        continue
    if name not in b.vars:
        bad.append("missing in old: " + name); continue
    ra, rb = ta.get(name, {}), tb.get(name, {})
    ca = cb = None
    for t in sorted(set(ra) | set(rb), key=rg.time_key):
        ca, cb = ra.get(t, ca), rb.get(t, cb)
        if ca != cb:
            bad.append("%s differs at #%s: %s != %s" % (name, t, ca, cb)); break
    n += 1
print("%d variables, %s" % (n, "bit-identical" if not bad else "DIFFER: " + "; ".join(bad[:5])), [v for v in sorted(a.vars) if "{" in v] if len(sys.argv) > 4 else "")
sys.exit(1 if bad else 0)
