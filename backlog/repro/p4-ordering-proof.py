#!/usr/bin/env python3
"""P4 (F-01) proof that a golden re-record changes ordering only.

Usage: python3 -I backlog/repro/p4-ordering-proof.py OLD_DIR NEW_DIR [--only REGEX] [--noisy REGEX ...]

OLD_DIR/NEW_DIR are golden baseline directories (tests/golden/baseline, or a `check --keep` dir, or a git
checkout of the old baseline). For every case present in both, each output is classified:
  same       byte-identical
  reordered  text: same multiset of lines; VCD: same timescale and the same set of
             (hierarchical signal name, type, width, waveform) tuples, the waveform being the exact list of
             (time, value string) records; JSON: same parsed value
  DIFFERENT  anything else (also: exit status, set of output files)
Signals matching a --noisy regex (seeded detector noise) are only required to exist in both VCDs with the same
type and width. Exit status: 0 if nothing is DIFFERENT, 1 otherwise.
"""
import argparse
import json
import re
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests" / "golden"))
import run_golden as rg  # noqa: E402  (mask_ids, id_profiles)


def parse_vcd(text):
    scopes = []
    ids = {}  # id code -> list of (name, type, width)
    timescale = None
    lines = text.split("\n")
    header = True
    waves = {}
    t = None
    tokens = []
    for line in lines:
        tokens.extend(line.split())
    k = 0
    while k < len(tokens):
        tok = tokens[k]
        if header:
            if tok == "$timescale":
                j = tokens.index("$end", k)
                timescale = " ".join(tokens[k + 1:j])
                k = j + 1
                continue
            if tok == "$scope":
                j = tokens.index("$end", k)
                scopes.append(tokens[k + 2])
                k = j + 1
                continue
            if tok == "$upscope":
                scopes.pop()
                k = tokens.index("$end", k) + 1
                continue
            if tok == "$var":
                j = tokens.index("$end", k)
                vtype, width, code, ref = tokens[k + 1], tokens[k + 2], tokens[k + 3], tokens[k + 4]
                name = ".".join(scopes + [ref])
                ids.setdefault(code, []).append((name, vtype, width))
                k = j + 1
                continue
            if tok == "$enddefinitions":
                k = tokens.index("$end", k) + 1
                header = False
                continue
            if tok.startswith("$"):
                k = tokens.index("$end", k) + 1
                continue
            k += 1
            continue
        # value changes
        if tok.startswith("#"):
            t = int(tok[1:])
        elif tok in ("$dumpvars", "$end", "$dumpall", "$dumpon", "$dumpoff"):
            pass
        elif tok[0] in "rRbB":
            val, code = tok, tokens[k + 1]
            k += 1
            for (name, _, _) in ids.get(code, []):
                waves.setdefault(name, []).append((t, val))
        else:
            val, code = tok[0], tok[1:]
            for (name, _, _) in ids.get(code, []):
                waves.setdefault(name, []).append((t, val))
        k += 1
    decls = {}
    for code, lst in ids.items():
        for (name, vtype, width) in lst:
            decls[name] = (vtype, width)
    return timescale, decls, waves


def classify_vcd(a, b, noisy):
    if a == b:
        return "same", ""
    ta, da, wa = parse_vcd(a)
    tb, db, wb = parse_vcd(b)
    if ta != tb:
        return "DIFFERENT", "timescale %s != %s" % (ta, tb)
    if da != db:
        missing = sorted(set(da) ^ set(db))
        return "DIFFERENT", "declarations differ: %s" % missing[:5]
    bad = []
    noisy_changed = []
    for name in da:
        if wa.get(name, []) != wb.get(name, []):
            if any(r.search(name) for r in noisy):
                noisy_changed.append(name)
            else:
                bad.append(name)
    if bad:
        return "DIFFERENT", "waveforms differ: %s" % bad[:5]
    if noisy_changed:
        return "reordered+noise", "seeded noise signals changed: %s" % noisy_changed
    return "reordered", ""


def classify_text(a, b):
    # Printed OpticalSignal IDs are stored as first-appearance ranks [#k] (B-129): a reorder permutes the
    # ranks, so compare like the harness does: lines with IDs masked, plus the ID structure (id_profiles)
    if a == b:
        return "same", ""
    if Counter(a.split("\n")) == Counter(b.split("\n")):
        return "reordered", ""
    if (Counter(rg.mask_ids(a).split("\n")) == Counter(rg.mask_ids(b).split("\n"))
            and rg.id_profiles(a) == rg.id_profiles(b)):
        return "reordered", "same lines with IDs masked, same ID structure"
    return "DIFFERENT", "line multisets differ"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("old")
    ap.add_argument("new")
    ap.add_argument("--only", default=None)
    ap.add_argument("--noisy", action="append", default=[])
    args = ap.parse_args()
    noisy = [re.compile(r) for r in args.noisy]
    old = Path(args.old) / "cases"
    new = Path(args.new) / "cases"
    sel = re.compile(args.only) if args.only else None
    counts = Counter()
    failed = False
    for cj in sorted(new.glob("*/*/case.json")):
        cid = str(cj.parent.relative_to(new))
        if sel and not sel.search(cid):
            continue
        ocj = old / cid / "case.json"
        if not ocj.exists():
            print("NEW        %s" % cid)
            continue
        mo = json.loads(ocj.read_text())
        mn = json.loads(cj.read_text())
        results = []
        if mo.get("exit") != mn.get("exit"):
            results.append(("exit", "DIFFERENT", "%s != %s" % (mo.get("exit"), mn.get("exit"))))
        fo = sorted(mo.get("files", {}))
        fn = sorted(mn.get("files", {}))
        if fo != fn:
            results.append(("files", "DIFFERENT", "%s != %s" % (fo, fn)))
        for stream in ("stdout.txt", "stderr.txt"):
            a = (old / cid / stream).read_text() if (old / cid / stream).exists() else ""
            b = (new / cid / stream).read_text() if (new / cid / stream).exists() else ""
            results.append((stream,) + classify_text(a, b))
        for f in sorted((new / cid / "files").rglob("*")) if (new / cid / "files").exists() else []:
            if not f.is_file():
                continue
            rel = f.relative_to(new / cid / "files")
            of = old / cid / "files" / rel
            if not of.exists():
                continue
            a, b = of.read_bytes(), f.read_bytes()
            name = str(rel)
            if name.endswith(".gz"):
                import gzip
                a, b = gzip.decompress(a), gzip.decompress(b)
                name = name[:-3]
            if name.endswith(".vcd"):
                results.append((str(rel),) + classify_vcd(a.decode(), b.decode(), noisy))
            elif name.endswith(".json"):
                if a == b:
                    results.append((str(rel), "same", ""))
                elif json.loads(a) == json.loads(b):
                    results.append((str(rel), "reordered", "same parsed JSON"))
                else:
                    results.append((str(rel), "DIFFERENT", "JSON differs"))
            else:
                results.append((str(rel),) + classify_text(a.decode(errors="replace"), b.decode(errors="replace")))
        worst = "same"
        for (_, c, _) in results:
            if c == "DIFFERENT":
                worst = "DIFFERENT"
            elif c.startswith("reordered") and worst != "DIFFERENT":
                worst = c if worst in ("same", "reordered") else worst
        counts[worst] += 1
        if worst != "same":
            detail = "; ".join("%s: %s%s" % (s, c, (" (" + d + ")") if d else "") for (s, c, d) in results if c != "same")
            print("%-10s %s  %s" % (worst.upper() if worst == "DIFFERENT" else worst, cid, detail))
        if worst == "DIFFERENT":
            failed = True
    print("Summary: " + ", ".join("%d %s" % (v, k) for k, v in sorted(counts.items())))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
