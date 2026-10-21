#!/usr/bin/env python3
"""F-05 item 1 check: a re-recorded case differs from the old baseline only by
(a) the auto-probe scopes SystemC.PROBE{...} removed from its VCDs,
(b) SystemC.trace_all_optical_nets 1 -> 0 (VCD) and '- trace all optical nets: 1' -> 0 (stdout),
(c) stdout lines naming PROBE{...} (--vci) removed;
every other signal is bit-identical (tolerance 0) and must include at least one explicit probe.
Usage: verify_traceall.py OLD_BASELINE NEW_DIR REPO case_id...
"""
import difflib, sys
from pathlib import Path
sys.path.insert(0, sys.argv[3] + "/tests/golden")
import run_golden as rg

old, new = Path(sys.argv[1]), Path(sys.argv[2])
ok_all = True
for cid in sys.argv[4:]:
    a, b = rg.load_stored(old, cid), rg.load_stored(new, cid)
    errs = []
    if a["args"] != b["args"] or a["exit"] != b["exit"]:
        errs.append("args/exit differ")
    if a["stderr"] != b["stderr"]:
        errs.append("stderr differs")
    la, lb = a["stdout"].split("\n"), b["stdout"].split("\n")
    for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, la, lb, autojunk=False).get_opcodes():
        if tag == "equal":
            continue
        rem, add = la[i1:i2], lb[j1:j2]
        for l in rem:
            if not ("PROBE{" in l or l == "- trace all optical nets: 1"):
                errs.append("stdout: unexpected removed line %r" % l)
        for l in add:
            if l != "- trace all optical nets: 0":
                errs.append("stdout: unexpected added line %r" % l)
    if set(a["files"]) != set(b["files"]):
        errs.append("file sets differ")
    nexplicit = 0
    for rel in sorted(set(a["files"]) & set(b["files"])):
        fa, fb = a["files"][rel], b["files"][rel]
        if fa["kind"] != "vcd":
            if fa["sha256"] != fb["sha256"]:
                errs.append("%s differs" % rel)
            continue
        va, vb = rg.parse_vcd(fa["content"]), rg.parse_vcd(fb["content"])
        auto = {n for n in va.vars if n.startswith("SystemC.PROBE{")}
        if va.timescale != vb.timescale:
            errs.append("%s: timescale" % rel)
        if set(vb.vars) != set(va.vars) - auto:
            errs.append("%s: var set is not old minus auto-probes: %s" % (rel, sorted(set(vb.vars) ^ (set(va.vars) - auto))[:5]))
        ta, tb = rg.timelines(va), rg.timelines(vb)
        for name in sorted(set(vb.vars) & set(va.vars)):
            ra, rb = ta.get(name, {}), tb.get(name, {})
            if name == "SystemC.trace_all_optical_nets":
                if not (set(ra.values()) == {"1"} and set(rb.values()) == {"0"}):
                    errs.append("%s: trace_all_optical_nets %s -> %s" % (rel, ra, rb))
                continue
            if name.split(".")[1].split(" ")[0] not in ("abstol", "reltol", "resolution_multiplier", "engine_timescale",
                                          "simulation_mode", "analysis_type", "verbose_component_initialization"):
                nexplicit += 1
            ca = cb = None
            for t in sorted(set(ra) | set(rb), key=rg.time_key):
                ca = ra.get(t, ca)
                cb = rb.get(t, cb)
                if ca != cb:
                    errs.append("%s: %s differs at #%s: %s != %s" % (rel, name, t, ca, cb))
                    break
    has_vcd = any(f["kind"] == "vcd" for f in a["files"].values())
    if has_vcd and nexplicit == 0:
        errs.append("no explicit probe signal compared")
    status = "OK  " if not errs else "BAD "
    ok_all &= not errs
    print(status, cid, "(%d probed/other signals bit-identical)" % nexplicit if has_vcd else "(no VCD: stdout config echo only)", "; ".join(errs[:5]))
sys.exit(0 if ok_all else 1)
