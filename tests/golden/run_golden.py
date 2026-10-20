#!/usr/bin/env python3
"""Golden-output regression harness for SPECS.

Build-system agnostic: it only needs the path to a built ``specs`` binary
(and optionally a ``unit_tests`` GoogleTest binary).  Python 3.9+, stdlib
only; intended to be run as ``python3 -I tests/golden/run_golden.py ...``.

Sub-commands
------------
record   Run every case from cases.json and store normalised outputs in --out.
check    Run every case and compare against a recorded --baseline directory.
compare  Compare two already-recorded directories (e.g. two `record` runs,
         to test determinism).
list     Print the case list.

See tests/golden/README.md for details.
"""

import argparse
import concurrent.futures
import fnmatch
import gzip
import hashlib
import json
import math
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from pathlib import Path

FORMAT_VERSION = 1
SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_REPO = SCRIPT_DIR.parent.parent
DEFAULT_CASES = SCRIPT_DIR / "cases.json"
GTEST_ID = "gtest/unit_tests"

# --------------------------------------------------------------------------
# Normalisation
# --------------------------------------------------------------------------

# SystemC copyright banner (also suppressed via SC_COPYRIGHT_MESSAGE=DISABLE).
BANNER_RE = re.compile(
    r"^\s*(?:SystemC \d+\.\d+\.\d+\S*\s+---.*"
    r"|Copyright \(c\) \d{4}-\d{4} by all Contributors,?"
    r"|ALL RIGHTS RESERVED)\s*$"
)
# SystemC report location: keep only the basename, drop directory and line.
IN_FILE_RE = re.compile(r"In file: (?:\S*/)?([^/\s:]+)(?::\d+)?")
# Wall-clock timings printed by `specs -t` (main.cpp do_test).
RUNTIME_RE = re.compile(r"Total runtime: \S+ ms")
NEG_NAN_RE = re.compile(r"-nan\b")

# Numbers embedded in text: not glued to identifiers (PS1, COL_0, uut2 ...).
NUM_RE = re.compile(
    r"(?<![A-Za-z0-9_.])[-+]?(?:\d+\.\d*|\.\d+|\d+)(?:[eE][-+]?\d+)?(?![A-Za-z0-9_.])"
)


def decode(b):
    if b is None:
        return ""
    if isinstance(b, str):
        return b
    return b.decode("utf-8", errors="replace")


class PathMap:
    """Replace volatile absolute paths by stable placeholders."""

    def __init__(self, pairs):
        seen = []
        for real, token in pairs:
            if not real:
                continue
            for variant in {str(real), os.path.realpath(str(real))}:
                if variant and variant not in [s for s, _ in seen] and variant != "/":
                    seen.append((variant, token))
        # Longest first so that nested paths are replaced correctly.
        self.pairs = sorted(seen, key=lambda p: -len(p[0]))

    def apply(self, line):
        for real, token in self.pairs:
            if real in line:
                line = line.replace(real, token)
        return line


def normalise_text(text, pathmap):
    text = text.replace("\r\n", "\n")
    out = []
    for line in text.split("\n"):
        if BANNER_RE.match(line):
            continue
        line = pathmap.apply(line)
        line = IN_FILE_RE.sub(r"In file: \1", line)
        line = RUNTIME_RE.sub("Total runtime: <DURATION> ms", line)
        line = NEG_NAN_RE.sub("nan", line)
        out.append(line.rstrip())
    while out and out[0] == "":
        out.pop(0)
    while len(out) > 1 and out[-1] == "" and out[-2] == "":
        out.pop()
    return "\n".join(out)


VCD_DROP_BLOCKS = ("$date", "$version")


def normalise_vcd(text):
    """Strip $date/$version blocks (volatile); keep everything else."""
    out = []
    skipping = False
    for line in text.replace("\r\n", "\n").split("\n"):
        stripped = line.strip()
        if not skipping and any(stripped.startswith(k) for k in VCD_DROP_BLOCKS):
            skipping = not stripped.endswith("$end")
            continue
        if skipping:
            if stripped.endswith("$end"):
                skipping = False
            continue
        out.append(line.rstrip())
    while out and out[0] == "":
        out.pop(0)
    return "\n".join(out)


def file_kind(rel):
    low = rel.lower()
    if low.endswith(".vcd"):
        return "vcd"
    if low.endswith(".json"):
        return "json"
    if low.endswith((".txt", ".csv", ".log", ".out", ".dat", ".cir")):
        return "text"
    return "auto"


def sha256(data):
    if isinstance(data, str):
        data = data.encode("utf-8")
    return hashlib.sha256(data).hexdigest()


# --------------------------------------------------------------------------
# Numeric comparison helpers
# --------------------------------------------------------------------------


class Tol:
    def __init__(self, rtol, atol, slack):
        self.rtol = rtol
        self.atol = atol
        self.slack = slack  # multiples of the last printed decimal digit

    def floats_close(self, a, b, extra=0.0):
        if a == b:
            return True
        if math.isnan(a) and math.isnan(b):
            return True
        if math.isinf(a) or math.isinf(b) or math.isnan(a) or math.isnan(b):
            return False
        return abs(a - b) <= self.atol + self.rtol * max(abs(a), abs(b)) + extra


def is_int_token(tok):
    return not any(c in tok for c in ".eE")


def printed_ulp(tok):
    """Value of one unit in the last printed digit of a float token."""
    m = re.match(r"[-+]?(\d*)(?:\.(\d*))?(?:[eE]([-+]?\d+))?$", tok)
    if not m:
        return 0.0
    decimals = len(m.group(2) or "")
    exp = int(m.group(3) or 0)
    return 10.0 ** (exp - decimals)


def compare_text(a, b, tol, label, limit=10):
    """Numeric-aware, line-by-line comparison. Returns list of diff strings."""
    diffs = []
    la = a.split("\n")
    lb = b.split("\n")
    if len(la) != len(lb):
        diffs.append("%s: line count %d != %d" % (label, len(la), len(lb)))
    for i, (x, y) in enumerate(zip(la, lb)):
        if x == y:
            continue
        sx = NUM_RE.split(x)
        sy = NUM_RE.split(y)
        nx = NUM_RE.findall(x)
        ny = NUM_RE.findall(y)
        ok = sx == sy and len(nx) == len(ny)
        if ok:
            for tx, ty in zip(nx, ny):
                if is_int_token(tx) and is_int_token(ty):
                    if int(tx) != int(ty):
                        ok = False
                        break
                    continue
                fx, fy = float(tx), float(ty)
                extra = tol.slack * max(printed_ulp(tx), printed_ulp(ty))
                if not tol.floats_close(fx, fy, extra):
                    ok = False
                    break
        if not ok:
            diffs.append("%s:%d:\n      - %s\n      + %s" % (label, i + 1, x, y))
            if len(diffs) >= limit:
                diffs.append("%s: ... (further differences not shown)" % label)
                break
    return diffs


def compare_json_values(a, b, tol, path, diffs, limit=20):
    if len(diffs) >= limit:
        return
    if isinstance(a, bool) or isinstance(b, bool) or a is None or b is None:
        if a != b:
            diffs.append("%s: %r != %r" % (path, a, b))
        return
    if isinstance(a, (int, float)) and isinstance(b, (int, float)):
        if isinstance(a, int) and isinstance(b, int):
            if a != b:
                diffs.append("%s: %r != %r" % (path, a, b))
        elif not tol.floats_close(float(a), float(b)):
            diffs.append("%s: %r != %r" % (path, a, b))
        return
    if type(a) is not type(b):
        diffs.append("%s: type %s != %s" % (path, type(a).__name__, type(b).__name__))
        return
    if isinstance(a, dict):
        ka, kb = set(a), set(b)
        for k in sorted(ka - kb):
            diffs.append("%s: key %r missing in candidate" % (path, k))
        for k in sorted(kb - ka):
            diffs.append("%s: unexpected key %r in candidate" % (path, k))
        for k in sorted(ka & kb):
            compare_json_values(a[k], b[k], tol, "%s.%s" % (path, k), diffs, limit)
    elif isinstance(a, list):
        if len(a) != len(b):
            diffs.append("%s: list length %d != %d" % (path, len(a), len(b)))
        for i, (x, y) in enumerate(zip(a, b)):
            compare_json_values(x, y, tol, "%s[%d]" % (path, i), diffs, limit)
    elif a != b:
        diffs.append("%s: %r != %r" % (path, a, b))


# --------------------------------------------------------------------------
# VCD parsing / comparison
# --------------------------------------------------------------------------


class Vcd:
    def __init__(self):
        self.timescale = ""
        self.vars = {}  # full name -> "type/size"
        self.changes = {}  # time -> {full name: value}
        self.order = []  # times in file order


def parse_vcd(text):
    vcd = Vcd()
    toks = text.split()
    n = len(toks)
    i = 0
    scopes = []
    ids = {}  # id code -> [full names]

    def skip_to_end(j):
        while j < n and toks[j] != "$end":
            j += 1
        return j + 1

    # Header
    while i < n:
        t = toks[i]
        if t == "$enddefinitions":
            i = skip_to_end(i)
            break
        if t == "$timescale":
            j = i + 1
            parts = []
            while j < n and toks[j] != "$end":
                parts.append(toks[j])
                j += 1
            vcd.timescale = " ".join(parts)
            i = j + 1
        elif t == "$scope":
            scopes.append(toks[i + 2] if i + 2 < n else "?")
            i = skip_to_end(i)
        elif t == "$upscope":
            if scopes:
                scopes.pop()
            i = skip_to_end(i)
        elif t == "$var":
            j = i + 1
            parts = []
            while j < n and toks[j] != "$end":
                parts.append(toks[j])
                j += 1
            i = j + 1
            if len(parts) >= 4:
                vtype, size, code, ref = parts[0], parts[1], parts[2], " ".join(parts[3:])
                name = ".".join(scopes + [ref])
                base, k = name, 1
                while name in vcd.vars:
                    k += 1
                    name = "%s#%d" % (base, k)
                vcd.vars[name] = "%s/%s" % (vtype, size)
                ids.setdefault(code, []).append(name)
        elif t.startswith("$"):
            i = skip_to_end(i)
        else:
            i += 1

    # Body
    cur = 0
    vcd.changes[cur] = {}
    vcd.order.append(cur)
    while i < n:
        t = toks[i]
        if t.startswith("#"):
            try:
                cur = int(t[1:])
            except ValueError:
                cur = t[1:]
            if cur not in vcd.changes:
                vcd.changes[cur] = {}
                vcd.order.append(cur)
            i += 1
            continue
        if t == "$comment":
            i = skip_to_end(i)
            continue
        if t.startswith("$"):
            i += 1
            continue
        c = t[0]
        if c in "rRbB":
            val = t
            code = toks[i + 1] if i + 1 < n else ""
            i += 2
        else:
            val = c
            code = t[1:]
            i += 1
        for name in ids.get(code, ["<unknown:%s>" % code]):
            vcd.changes[cur][name] = val
    # Timestamps without any value change are ignored by the comparison.
    vcd.order = [t for t in vcd.order if vcd.changes.get(t)]
    return vcd


def vcd_value_close(va, vb, tol):
    if va == vb:
        return True
    if va[:1] in "rR" and vb[:1] in "rR":
        try:
            return tol.floats_close(float(va[1:]), float(vb[1:]))
        except ValueError:
            return False
    return False


def compare_vcd(ta, tb_, tol, label, ignore_value_res, limit=12):
    a = parse_vcd(ta)
    b = parse_vcd(tb_)
    diffs = []
    if a.timescale != b.timescale:
        diffs.append("%s: timescale %r != %r" % (label, a.timescale, b.timescale))
    va, vb = set(a.vars), set(b.vars)
    for name in sorted(va - vb)[:limit]:
        diffs.append("%s: var %s missing in candidate" % (label, name))
    for name in sorted(vb - va)[:limit]:
        diffs.append("%s: unexpected var %s in candidate" % (label, name))
    for name in sorted(va & vb):
        if a.vars[name] != b.vars[name]:
            diffs.append("%s: var %s type %s != %s" % (label, name, a.vars[name], b.vars[name]))
    # Semantic comparison: every signal is a piecewise-constant function of
    # time.  SystemC only emits a VCD record when a value changes bit-exactly,
    # so ulp-level float differences (e.g. -O0 vs -O3, FMA contraction) can
    # add or drop records without any meaningful change.  We therefore compare
    # the *current* value of each signal at every time where either side has a
    # record, with tolerance.
    tla, tlb = timelines(a), timelines(b)
    nbad_vars = 0
    for name in sorted(va & vb):
        if any(r.search(name) for r in ignore_value_res):
            continue
        ra, rb = tla.get(name, {}), tlb.get(name, {})
        if ra == rb:
            continue
        cur_a = cur_b = None
        first = None
        npoints = 0
        for t in sorted(set(ra) | set(rb), key=time_key):
            if t in ra:
                cur_a = ra[t]
            if t in rb:
                cur_b = rb[t]
            if cur_a is None or cur_b is None or not vcd_value_close(cur_a, cur_b, tol):
                npoints += 1
                if first is None:
                    first = (t, cur_a, cur_b)
        if npoints:
            nbad_vars += 1
            if nbad_vars <= limit:
                diffs.append(
                    "%s: %s differs at %d time point(s); first at #%s: %s != %s"
                    % (label, name, npoints, first[0], first[1], first[2])
                )
    if nbad_vars > limit:
        diffs.append("%s: ... %d signals differ in total" % (label, nbad_vars))
    return diffs


def time_key(t):
    return (0, t, "") if isinstance(t, int) else (1, 0, str(t))


def timelines(vcd):
    out = {}
    for t in vcd.order:
        for name, val in vcd.changes.get(t, {}).items():
            out.setdefault(name, {})[t] = val
    return out


def vcd_summary(text):
    """Lossy fingerprint used only for files too large to store even gzipped.

    Only statistics that do not depend on the number of VCD records are kept
    (min / max / last value per signal), because redundant records come and go
    with ulp-level float differences (see compare_vcd).
    """
    v = parse_vcd(text)
    per = {}
    for t in v.order:
        for name, val in v.changes.get(t, {}).items():
            s = per.setdefault(name, {})
            if val[:1] in "rR":
                try:
                    f = float(val[1:])
                except ValueError:
                    s["last"] = val
                    continue
                if math.isfinite(f):
                    s["min"] = min(s.get("min", f), f)
                    s["max"] = max(s.get("max", f), f)
                s["last"] = f if math.isfinite(f) else str(f)
            else:
                s["last"] = val
    return {"timescale": v.timescale, "vars": v.vars, "per_var": per}


# --------------------------------------------------------------------------
# Cases
# --------------------------------------------------------------------------


def load_cases(path):
    with open(path, encoding="utf-8") as f:
        spec = json.load(f)
    defaults = spec.get("defaults", {})
    cases = []
    for c in spec["cases"]:
        merged = dict(defaults)
        merged.update(c)
        cases.append(merged)
    ids = [c["id"] for c in cases]
    dup = {x for x in ids if ids.count(x) > 1}
    if dup:
        raise SystemExit("duplicate case ids in %s: %s" % (path, sorted(dup)))
    gtest = dict(spec.get("gtest", {}))
    return spec, cases, gtest


def select(cases, only):
    if not only:
        return cases
    rx = re.compile(only)
    return [c for c in cases if rx.search(c["id"])]


def safe_rel(case_id):
    if ".." in case_id.split("/") or case_id.startswith("/"):
        raise SystemExit("invalid case id %r" % case_id)
    return case_id


def make_env():
    env = dict(os.environ)
    env.update({"LC_ALL": "C", "LANG": "C", "TZ": "UTC", "SC_COPYRIGHT_MESSAGE": "DISABLE"})
    return env


def exit_repr(rc):
    if isinstance(rc, int) and rc < 0:
        try:
            return "signal:%s" % signal.Signals(-rc).name
        except ValueError:
            return "signal:%d" % -rc
    return rc


def snapshot(root):
    snap = {}
    for dirpath, _, files in os.walk(root):
        for fn in files:
            p = Path(dirpath) / fn
            rel = p.relative_to(root).as_posix()
            try:
                with open(p, "rb") as f:
                    snap[rel] = sha256(f.read())
            except OSError:
                snap[rel] = None
    return snap


def run_case(case, specs, repo, tmp_root):
    """Run one case; returns a dict with normalised outputs (in memory)."""
    wd = Path(tempfile.mkdtemp(prefix="specs-golden-", dir=tmp_root))
    try:
        setup = case.get("setup", {})
        for d in setup.get("copy", []):
            src = repo / d
            if src.is_dir():
                shutil.copytree(src, wd / d)
            elif src.exists():
                (wd / d).parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, wd / d)
        for d in setup.get("mkdir", []):
            (wd / d).mkdir(parents=True, exist_ok=True)
        before = snapshot(wd)
        argv = ["specs"] + list(case.get("args", []))
        timeout = case.get("timeout", 120)
        t0 = time.monotonic()
        try:
            p = subprocess.run(
                argv,
                executable=str(specs),
                cwd=str(wd),
                env=make_env(),
                stdin=subprocess.DEVNULL,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                timeout=timeout,
            )
            rc, out, err = exit_repr(p.returncode), p.stdout, p.stderr
        except subprocess.TimeoutExpired as e:
            rc, out, err = "timeout", e.stdout, e.stderr
        elapsed = time.monotonic() - t0
        after = snapshot(wd)
        pathmap = PathMap(
            [
                (str(wd), "<CWD>"),
                (str(Path(specs).resolve().parent), "<BUILD>"),
                (str(repo), "<REPO>"),
                (os.environ.get("HOME", ""), "<HOME>"),
            ]
        )
        files = {}
        for rel in sorted(after):
            if rel in before and before[rel] == after[rel]:
                continue
            if any(fnmatch.fnmatch(rel, g) for g in case.get("ignore_files", [])):
                continue
            with open(wd / rel, "rb") as f:
                raw = f.read()
            kind = file_kind(rel)
            if kind == "auto":
                try:
                    raw.decode("utf-8")
                    kind = "text"
                except UnicodeDecodeError:
                    kind = "binary"
            if kind == "vcd":
                content = normalise_vcd(decode(raw))
            elif kind in ("text", "json"):
                content = normalise_text(decode(raw), pathmap) if kind == "text" else decode(raw)
            else:
                content = raw
            files[rel] = {"kind": kind, "content": content}
        return {
            "id": case["id"],
            "args": list(case.get("args", [])),
            "exit": rc,
            "elapsed": elapsed,
            "stdout": normalise_text(decode(out), pathmap),
            "stderr": normalise_text(decode(err), pathmap),
            "files": files,
        }
    finally:
        shutil.rmtree(wd, ignore_errors=True)


GTEST_LINE_RE = re.compile(r"^\[\s+(OK|FAILED|SKIPPED)\s+\] (\S+) \(")


def run_gtest(unit_tests, gspec, tmp_root):
    wd = Path(tempfile.mkdtemp(prefix="specs-golden-gtest-", dir=tmp_root))
    try:
        listed = []
        try:
            p = subprocess.run(
                [str(unit_tests), "--gtest_list_tests"],
                cwd=str(wd), env=make_env(), stdin=subprocess.DEVNULL,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60,
            )
            suite = None
            for line in decode(p.stdout).split("\n"):
                if not line.strip() or line.startswith("Running main()"):
                    continue
                if not line.startswith(" "):
                    suite = line.split("#")[0].strip()
                elif suite:
                    listed.append(suite + line.split("#")[0].strip())
        except (OSError, subprocess.TimeoutExpired):
            pass
        jpath = wd / "gtest.json"
        args = [str(unit_tests), "--gtest_output=json:%s" % jpath]
        if gspec.get("filter"):
            args.append("--gtest_filter=%s" % gspec["filter"])
        try:
            p = subprocess.run(
                args, cwd=str(wd), env=make_env(), stdin=subprocess.DEVNULL,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                timeout=gspec.get("timeout", 600),
            )
            rc, out = exit_repr(p.returncode), decode(p.stdout)
        except subprocess.TimeoutExpired as e:
            rc, out = "timeout", decode(e.stdout)
        tests = {name: "NOT_RUN" for name in listed}
        if jpath.exists():
            with open(jpath, encoding="utf-8") as f:
                data = json.load(f)
            for s in data.get("testsuites", []):
                for t in s.get("testsuite", []):
                    name = "%s.%s" % (s["name"], t["name"])
                    if t.get("failures"):
                        tests[name] = "FAIL"
                    elif t.get("result") == "SKIPPED" or t.get("status") == "NOTRUN":
                        tests[name] = "SKIP"
                    else:
                        tests[name] = "PASS"
        else:
            for line in out.split("\n"):
                m = GTEST_LINE_RE.match(line)
                if m:
                    tests[m.group(2)] = {"OK": "PASS", "FAILED": "FAIL", "SKIPPED": "SKIP"}[m.group(1)]
        return {"id": GTEST_ID, "exit": rc, "tests": dict(sorted(tests.items()))}
    finally:
        shutil.rmtree(wd, ignore_errors=True)


# --------------------------------------------------------------------------
# Storage
# --------------------------------------------------------------------------


def store_result(outdir, res, max_file_bytes):
    cdir = outdir / "cases" / safe_rel(res["id"])
    if cdir.exists():
        shutil.rmtree(cdir)
    cdir.mkdir(parents=True)
    (cdir / "stdout.txt").write_text(res["stdout"] + "\n", encoding="utf-8")
    (cdir / "stderr.txt").write_text(res["stderr"] + "\n", encoding="utf-8")
    meta = {"id": res["id"], "args": res["args"], "exit": res["exit"], "files": {}}
    for rel, f in sorted(res["files"].items()):
        content = f["content"]
        data = content.encode("utf-8") if isinstance(content, str) else content
        entry = {"kind": f["kind"], "bytes": len(data), "sha256": sha256(data)}
        gz = None
        if len(data) > max_file_bytes:
            # Deterministic gzip (mtime=0) so that re-recording is reproducible.
            gz = gzip.compress(data, compresslevel=9, mtime=0)
        if len(data) <= max_file_bytes or len(gz) <= max_file_bytes:
            dest = cdir / "files" / (rel if gz is None else rel + ".gz")
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(data if gz is None else gz)
            entry["stored"] = "files/" + rel + ("" if gz is None else ".gz")
        else:
            entry["stored"] = None
            if f["kind"] == "vcd":
                entry["summary"] = vcd_summary(content)
            elif f["kind"] in ("text", "json"):
                entry["summary"] = {"lines": content.count("\n") + 1}
        meta["files"][rel] = entry
    (cdir / "case.json").write_text(json.dumps(meta, indent=1, sort_keys=True) + "\n", encoding="utf-8")


def load_stored(dirpath, case_id):
    cdir = Path(dirpath) / "cases" / safe_rel(case_id)
    if not (cdir / "case.json").exists():
        return None
    meta = json.loads((cdir / "case.json").read_text(encoding="utf-8"))
    meta["stdout"] = (cdir / "stdout.txt").read_text(encoding="utf-8")
    meta["stderr"] = (cdir / "stderr.txt").read_text(encoding="utf-8")
    for rel, entry in meta["files"].items():
        if entry.get("stored"):
            data = (cdir / entry["stored"]).read_bytes()
            if entry["stored"].endswith(".gz"):
                data = gzip.decompress(data)
            entry["content"] = data if entry["kind"] == "binary" else data.decode("utf-8")
    return meta


def store_gtest(outdir, res):
    gdir = outdir / "gtest"
    gdir.mkdir(parents=True, exist_ok=True)
    (gdir / "unit_tests.json").write_text(json.dumps(res, indent=1, sort_keys=True) + "\n", encoding="utf-8")


def load_gtest(dirpath):
    p = Path(dirpath) / "gtest" / "unit_tests.json"
    if not p.exists():
        return None
    return json.loads(p.read_text(encoding="utf-8"))


def dir_size(path):
    total = 0
    for dirpath, _, files in os.walk(path):
        for fn in files:
            total += (Path(dirpath) / fn).stat().st_size
    return total


# --------------------------------------------------------------------------
# Comparison
# --------------------------------------------------------------------------


def compare_case(base, cand, case, tol, strict_order=False):
    """Both base/cand are 'stored' dicts (see load_stored).

    Returns (diffs, notes): diffs make the case fail, notes are informational.
    """
    diffs, notes = [], []
    if base["args"] != cand["args"]:
        diffs.append("args changed: %s -> %s (re-record?)" % (base["args"], cand["args"]))
    if base["exit"] != cand["exit"]:
        diffs.append("exit status %r != %r" % (base["exit"], cand["exit"]))
    ignore_streams = case.get("ignore_streams", [])
    unordered = case.get("unordered_streams", [])
    for stream in ("stdout", "stderr"):
        if stream in ignore_streams:
            continue
        ta, tb_ = base[stream].rstrip("\n"), cand[stream].rstrip("\n")
        d = compare_text(ta, tb_, tol, stream)
        if not d:
            continue
        sa = "\n".join(sorted(ta.split("\n")))
        sb = "\n".join(sorted(tb_.split("\n")))
        if not compare_text(sa, sb, tol, stream):
            # Same multiset of lines (within tolerance): only the order differs.
            # SPECS iterates SystemC objects in pointer order (std::set<T*>),
            # so listing order depends on memory layout (compiler, flags,
            # allocation pattern).  Accepted unless --strict-order.
            if stream in unordered or not strict_order:
                notes.append("%s: line order differs (reordered)" % stream)
                continue
            d = [
                "%s: ONLY THE LINE ORDER DIFFERS (SPECS iterates objects in pointer order, "
                "see README 'Known nondeterminism')" % stream
            ] + d[:4]
        diffs += d
    ign = [re.compile(r) for r in case.get("vcd_ignore_values", [])]
    bf, cf = base["files"], cand["files"]
    for rel in sorted(set(bf) - set(cf)):
        diffs.append("output file %s missing in candidate" % rel)
    for rel in sorted(set(cf) - set(bf)):
        diffs.append("unexpected output file %s in candidate" % rel)
    for rel in sorted(set(bf) & set(cf)):
        eb, ec = bf[rel], cf[rel]
        if eb["kind"] != ec["kind"]:
            diffs.append("%s: kind %s != %s" % (rel, eb["kind"], ec["kind"]))
            continue
        if eb["sha256"] == ec["sha256"]:
            continue
        kind = eb["kind"]
        if "content" in eb and "content" in ec:
            if kind == "vcd":
                diffs += compare_vcd(eb["content"], ec["content"], tol, rel, ign)
            elif kind == "json":
                try:
                    ja, jb = json.loads(eb["content"]), json.loads(ec["content"])
                    jd = []
                    compare_json_values(ja, jb, tol, rel, jd)
                    diffs += jd
                except ValueError:
                    diffs += compare_text(eb["content"], ec["content"], tol, rel)
            elif kind == "text":
                diffs += compare_text(eb["content"], ec["content"], tol, rel)
            else:
                diffs.append("%s: binary content differs" % rel)
        else:
            # At least one side only has a summary.
            sa = eb.get("summary")
            sb = ec.get("summary")
            if sa is None and kind == "vcd" and "content" in eb:
                sa = vcd_summary(eb["content"])
            if sb is None and kind == "vcd" and "content" in ec:
                sb = vcd_summary(ec["content"])
            if sa is None or sb is None:
                diffs.append("%s: content differs (sha256)" % rel)
                continue
            if ign and "per_var" in sa and "per_var" in sb:
                for s in (sa, sb):
                    for name in list(s["per_var"]):
                        if any(r.search(name) for r in ign):
                            s["per_var"][name] = {}
            jd = []
            compare_json_values(json.loads(json.dumps(sa)), json.loads(json.dumps(sb)), tol, rel + "{summary}", jd)
            diffs += jd
    return diffs, notes


def compare_gtest(base, cand):
    """Returns (fatal diffs, informational notes)."""
    fatal, notes = [], []
    bt, ct = base.get("tests", {}), cand.get("tests", {})
    for name in sorted(set(bt) - set(ct)):
        fatal.append("test %s missing in candidate" % name)
    for name in sorted(set(ct) - set(bt)):
        notes.append("new test %s: %s" % (name, ct[name]))
    for name in sorted(set(bt) & set(ct)):
        if bt[name] == ct[name]:
            continue
        msg = "test %s: %s -> %s" % (name, bt[name], ct[name])
        if bt[name] in ("FAIL", "NOT_RUN") and ct[name] == "PASS":
            notes.append(msg + " (improvement; re-record the baseline)")
        else:
            fatal.append(msg)
    if base.get("exit") != cand.get("exit") and not fatal:
        notes.append("exit status %r -> %r" % (base.get("exit"), cand.get("exit")))
    return fatal, notes


# --------------------------------------------------------------------------
# Commands
# --------------------------------------------------------------------------


def discover_uncovered(cases, specs, repo):
    """Warn about circuits / testbenches that have no case."""
    warnings = []
    covered_args = " ".join(" ".join(c.get("args", [])) for c in cases)
    for cir in sorted((repo / "circuits").glob("*.cir")):
        if "circuits/%s" % cir.name not in covered_args:
            warnings.append("circuit %s has no golden case" % cir.name)
    try:
        p = subprocess.run(
            ["specs", "-p"], executable=str(specs), env=make_env(), stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, timeout=60, cwd=tempfile.gettempdir(),
        )
        tbs = [l.strip() for l in decode(p.stdout).split("\n") if l.startswith("\t") and "#" not in l]
        have = {tuple(c.get("args", [])) for c in cases}
        for tb in tbs:
            if ("-t", tb) not in have:
                warnings.append("testbench %s has no golden case" % tb)
    except (OSError, subprocess.TimeoutExpired) as e:
        warnings.append("could not list testbenches: %s" % e)
    return warnings


def record_into(outdir, cases, gspec, args, verbose=True):
    outdir.mkdir(parents=True, exist_ok=True)
    specs = Path(args.specs).resolve()
    repo = Path(args.repo).resolve()
    tmp_root = args.tmpdir
    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as ex:
        futs = {ex.submit(run_case, c, specs, repo, tmp_root): c for c in cases}
        for fut in concurrent.futures.as_completed(futs):
            res = fut.result()
            store_result(outdir, res, args.max_file_bytes)
            results.append(res)
            if verbose:
                print("  ran %-40s exit=%-14s %6.2fs  files=%d" % (res["id"], res["exit"], res["elapsed"], len(res["files"])))
    gres = None
    if args.unit_tests:
        gres = run_gtest(Path(args.unit_tests).resolve(), gspec, tmp_root)
        store_gtest(outdir, gres)
        if verbose:
            summary = {}
            for v in gres["tests"].values():
                summary[v] = summary.get(v, 0) + 1
            print("  ran %-40s exit=%-14s %s" % (GTEST_ID, gres["exit"], summary))
    return results, gres


def write_manifest(outdir, cases, args, gres, note):
    manifest = {
        "format": FORMAT_VERSION,
        "recorded_at_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "specs": os.path.basename(args.specs),
        "unit_tests": os.path.basename(args.unit_tests) if args.unit_tests else None,
        "max_file_bytes": args.max_file_bytes,
        "cases": sorted(c["id"] for c in cases),
        "gtest": GTEST_ID if gres else None,
        "note": note,
    }
    (outdir / "manifest.json").write_text(json.dumps(manifest, indent=1) + "\n", encoding="utf-8")


def cmd_record(args):
    spec, cases, gspec = load_cases(args.cases)
    cases = select(cases, args.only)
    outdir = Path(args.out).resolve()
    if outdir.exists() and not args.only:
        # Full re-record: start from a clean tree so stale cases disappear.
        shutil.rmtree(outdir / "cases", ignore_errors=True)
        shutil.rmtree(outdir / "gtest", ignore_errors=True)
    print("Recording %d case(s) into %s" % (len(cases), outdir))
    for w in discover_uncovered(load_cases(args.cases)[1], Path(args.specs).resolve(), Path(args.repo).resolve()):
        print("  WARNING: " + w)
    _, gres = record_into(outdir, cases, gspec, args)
    if args.only and (outdir / "manifest.json").exists():
        old = json.loads((outdir / "manifest.json").read_text(encoding="utf-8"))
        ids = sorted(set(old.get("cases", [])) | {c["id"] for c in cases})
        old["cases"] = ids
        # Keep the note of a partial re-record (the top-level note describes the full record).
        if args.note:
            old.setdefault("rerecords", []).append({
                "recorded_at_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                "only": args.only,
                "gtest": bool(gres),
                "note": args.note,
            })
        (outdir / "manifest.json").write_text(json.dumps(old, indent=1) + "\n", encoding="utf-8")
    else:
        write_manifest(outdir, cases, args, gres, args.note)
    size = dir_size(outdir)
    print("Baseline size: %.1f kB" % (size / 1024.0))
    rc = 0
    if args.repeat > 1:
        rc = determinism_check(cases, gspec, args, outdir)
    return rc


def determinism_check(cases, gspec, args, first_dir):
    tol = Tol(args.rtol, args.atol, args.print_slack)
    bad = 0
    for k in range(2, args.repeat + 1):
        with tempfile.TemporaryDirectory(prefix="specs-golden-rep-", dir=args.tmpdir) as td:
            print("Determinism: repeat %d/%d" % (k, args.repeat))
            record_into(Path(td), cases, gspec, args, verbose=False)
            res = compare_dirs(first_dir, Path(td), cases, tol, verbose=False, strict_order=True)
            for cid, diffs in res["failed"].items():
                bad += 1
                print("  NONDETERMINISTIC %s" % cid)
                for d in diffs[:5]:
                    print("      " + d.replace("\n", "\n      "))
    if not bad:
        print("Determinism: all cases identical (within tolerance) across %d runs" % args.repeat)
    return 1 if bad else 0


def compare_dirs(base_dir, cand_dir, cases, tol, verbose=True, strict_order=False):
    out = {"passed": [], "failed": {}, "new": [], "missing": [], "notes": {}}
    for c in cases:
        cid = c["id"]
        b = load_stored(base_dir, cid)
        k = load_stored(cand_dir, cid)
        if b is None and k is None:
            continue
        if b is None:
            out["new"].append(cid)
            if verbose:
                print("NEW      %s (not in baseline)" % cid)
            continue
        if k is None:
            out["missing"].append(cid)
            if verbose:
                print("MISSING  %s (not run)" % cid)
            continue
        diffs, notes = compare_case(b, k, c, tol, strict_order)
        if notes:
            out["notes"][cid] = notes
        if diffs:
            out["failed"][cid] = diffs
            if verbose:
                print("FAIL     %s" % cid)
                for d in diffs[:15]:
                    print("    " + d.replace("\n", "\n    "))
        else:
            out["passed"].append(cid)
            if verbose:
                print("PASS     %s%s" % (cid, ("  (" + "; ".join(notes) + ")") if notes else ""))
    gb, gk = load_gtest(base_dir), load_gtest(cand_dir)
    if gb and gk:
        fatal, notes = compare_gtest(gb, gk)
        if fatal:
            out["failed"][GTEST_ID] = fatal
        else:
            out["passed"].append(GTEST_ID)
        if notes:
            out["notes"][GTEST_ID] = notes
        if verbose:
            print("%-8s %s  %s" % ("FAIL" if fatal else "PASS", GTEST_ID, dict(sorted(gk["tests"].items()))))
            for d in fatal + notes:
                print("    " + d)
    elif gb and not gk:
        out["missing"].append(GTEST_ID)
        if verbose:
            print("SKIPPED  %s (no --unit-tests given / not run)" % GTEST_ID)
    elif gk and not gb:
        out["new"].append(GTEST_ID)
        if verbose:
            print("NEW      %s (not in baseline)" % GTEST_ID)
    return out


def summarise(out, strict):
    nfail = len(out["failed"])
    nnotes = len([c for c in out["notes"] if c not in out["failed"]])
    print(
        "\nSummary: %d passed (%d with notes), %d failed, %d new, %d missing/skipped"
        % (len(out["passed"]), nnotes, nfail, len(out["new"]), len(out["missing"]))
    )
    if strict and (out["new"] or out["missing"]):
        return 1
    return 1 if nfail else 0


def cmd_check(args):
    spec, cases, gspec = load_cases(args.cases)
    cases = select(cases, args.only)
    base = Path(args.baseline).resolve()
    if not (base / "manifest.json").exists():
        raise SystemExit("no manifest.json in baseline %s" % base)
    manifest = json.loads((base / "manifest.json").read_text(encoding="utf-8"))
    if manifest.get("format") != FORMAT_VERSION:
        raise SystemExit("baseline format %r unsupported" % manifest.get("format"))
    tol = Tol(args.rtol, args.atol, args.print_slack)
    stale = sorted(set(manifest.get("cases", [])) - {c["id"] for c in load_cases(args.cases)[1]})
    keep = Path(args.keep).resolve() if args.keep else None
    td = None
    if keep:
        shutil.rmtree(keep, ignore_errors=True)
        cand = keep
    else:
        td = tempfile.TemporaryDirectory(prefix="specs-golden-cand-", dir=args.tmpdir)
        cand = Path(td.name)
    try:
        print("Running %d case(s) against baseline %s" % (len(cases), base))
        record_into(cand, cases, gspec, args, verbose=False)
        write_manifest(cand, cases, args, load_gtest(cand), "candidate")
        out = compare_dirs(base, cand, cases, tol, strict_order=args.strict_order)
    finally:
        if td:
            td.cleanup()
    for s in stale:
        print("STALE    %s (in baseline but not in cases.json)" % s)
    if args.report:
        Path(args.report).write_text(json.dumps(out, indent=1) + "\n", encoding="utf-8")
    return summarise(out, args.strict)


def cmd_compare(args):
    spec, cases, gspec = load_cases(args.cases)
    cases = select(cases, args.only)
    tol = Tol(args.rtol, args.atol, args.print_slack)
    out = compare_dirs(Path(args.baseline), Path(args.candidate), cases, tol, strict_order=args.strict_order)
    if args.report:
        Path(args.report).write_text(json.dumps(out, indent=1) + "\n", encoding="utf-8")
    return summarise(out, args.strict)


def cmd_list(args):
    spec, cases, gspec = load_cases(args.cases)
    for c in select(cases, args.only):
        extra = []
        if c.get("expect"):
            extra.append("expect=" + c["expect"])
        if c.get("vcd_ignore_values"):
            extra.append("vcd_ignore_values")
        print("%-40s specs %s %s" % (c["id"], " ".join(c.get("args", [])), " ".join(extra)))
    print("%-40s (only with --unit-tests)" % GTEST_ID)
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    def common(p, need_specs=True):
        p.add_argument("--cases", default=str(DEFAULT_CASES), help="case list (default: %(default)s)")
        p.add_argument("--only", help="regex: only run case ids matching it")
        p.add_argument("--rtol", type=float, default=1e-9, help="relative float tolerance (default 1e-9)")
        p.add_argument("--atol", type=float, default=1e-12, help="absolute float tolerance (default 1e-12)")
        p.add_argument(
            "--print-slack", type=float, default=1.0,
            help="extra tolerance for floats in text, in units of the last printed digit (default 1; 0 disables)",
        )
        p.add_argument("--strict", action="store_true", help="NEW/MISSING/SKIPPED cases also fail")
        p.add_argument(
            "--strict-order", action="store_true",
            help="fail when stdout/stderr lines only differ in order (default: accepted, reported as 'reordered')",
        )
        p.add_argument("--report", help="write a JSON report to this file")
        if need_specs:
            p.add_argument("--specs", required=True, help="path to the specs binary")
            p.add_argument("--unit-tests", help="path to the unit_tests (GoogleTest) binary")
            p.add_argument("--repo", default=str(DEFAULT_REPO), help="repo root (circuits/ is copied from here)")
            p.add_argument("--jobs", "-j", type=int, default=1, help="parallel cases (default 1)")
            p.add_argument("--tmpdir", default=None, help="where to create per-case working dirs")
            p.add_argument(
                "--max-file-bytes", type=int, default=1_000_000,
                help="files larger than this are stored gzipped, or as hash+summary if still too large (default 1e6)",
            )

    p = sub.add_parser("record", help="record a baseline")
    common(p)
    p.add_argument("--out", required=True, help="output (baseline) directory")
    p.add_argument("--repeat", type=int, default=1, help="run N times and report nondeterministic cases")
    p.add_argument("--note", default="", help="free text stored in manifest.json")
    p.set_defaults(func=cmd_record)

    p = sub.add_parser("check", help="run cases and compare against a baseline")
    common(p)
    p.add_argument("--baseline", required=True, help="baseline directory")
    p.add_argument("--keep", help="keep the candidate recording in this directory")
    p.set_defaults(func=cmd_check)

    p = sub.add_parser("compare", help="compare two recorded directories")
    common(p, need_specs=False)
    p.add_argument("--baseline", required=True)
    p.add_argument("--candidate", required=True)
    p.set_defaults(func=cmd_compare)

    p = sub.add_parser("list", help="list cases")
    p.add_argument("--cases", default=str(DEFAULT_CASES))
    p.add_argument("--only")
    p.set_defaults(func=cmd_list)

    args = ap.parse_args(argv)
    if getattr(args, "specs", None) and not os.access(args.specs, os.X_OK):
        raise SystemExit("specs binary not executable: %s" % args.specs)
    if getattr(args, "unit_tests", None) and not os.access(args.unit_tests, os.X_OK):
        raise SystemExit("unit_tests binary not executable: %s" % args.unit_tests)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
