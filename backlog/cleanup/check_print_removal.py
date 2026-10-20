#!/usr/bin/env python3
"""Prove that a change only removed known debug-print lines (ws-cleanup C-08).

Compares two golden recordings (`run_golden.py record --out DIR` or `check --keep DIR`) case by case:
  * exit status and the set of output files (with their sha256) must be identical;
  * stdout/stderr may only lose lines, and every lost line must match one of the --allow regexes
    (multiset difference; a pure deletion, i.e. NEW being a subsequence of OLD, is reported separately);
  * no line may be added.
Exit 0 if every case satisfies this, 1 otherwise. Run with `python3 -I`.
"""
import argparse
import json
import re
import sys
from collections import Counter
from pathlib import Path


def is_subsequence(short, long):
    it = iter(long)
    return all(any(x == y for y in it) for x in short)


def read_lines(path):
    return path.read_text().splitlines() if path.exists() else []


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('old', type=Path, help='recording made with the binary before the change')
    ap.add_argument('new', type=Path, help='recording made with the binary after the change')
    ap.add_argument('--allow', action='append', default=[], help='regex (fullmatch) for a line that may disappear')
    args = ap.parse_args()
    allow = [re.compile(a) for a in args.allow]

    old_cases = {p.parent.relative_to(args.old / 'cases').as_posix()
                 for p in (args.old / 'cases').glob('*/*/case.json')}
    new_cases = {p.parent.relative_to(args.new / 'cases').as_posix()
                 for p in (args.new / 'cases').glob('*/*/case.json')}
    ok = True
    if old_cases != new_cases:
        print(f'case sets differ: only old {sorted(old_cases - new_cases)}, only new {sorted(new_cases - old_cases)}')
        ok = False

    changed = []
    for case in sorted(old_cases & new_cases):
        o, n = args.old / 'cases' / case, args.new / 'cases' / case
        oj, nj = json.loads((o / 'case.json').read_text()), json.loads((n / 'case.json').read_text())
        problems = []
        if oj.get('exit') != nj.get('exit'):
            problems.append(f"exit {oj.get('exit')} -> {nj.get('exit')}")
        of = {k: v.get('sha256') for k, v in oj.get('files', {}).items()}
        nf = {k: v.get('sha256') for k, v in nj.get('files', {}).items()}
        if of != nf:
            problems.append(f'output files differ: {sorted(set(of.items()) ^ set(nf.items()))}')
        removed = {}
        for stream in ('stdout.txt', 'stderr.txt'):
            ol, nl = read_lines(o / stream), read_lines(n / stream)
            if ol == nl:
                continue
            gone, added = Counter(ol) - Counter(nl), Counter(nl) - Counter(ol)
            if added:
                problems.append(f'{stream}: added lines {dict(added)}')
            bad = {line: c for line, c in gone.items() if not any(r.fullmatch(line) for r in allow)}
            if bad:
                problems.append(f'{stream}: removed lines not allowed {bad}')
            order = 'pure deletion' if is_subsequence(nl, ol) else 'deletion + reorder of kept lines'
            removed[stream] = (sum(gone.values()), order)
        if problems:
            ok = False
            print(f'FAIL {case}: ' + '; '.join(problems))
        elif removed:
            changed.append(case)
            print(f'OK   {case}: ' + ', '.join(f'{s} -{k} lines ({how})' for s, (k, how) in removed.items()))
    print(f'\n{len(changed)} cases lost only allowed lines; files and exit codes identical in all '
          f'{len(old_cases & new_cases)} cases' if ok else '\nNOT a pure removal of the allowed lines')
    if changed:
        print('changed: ' + ' '.join(changed))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
