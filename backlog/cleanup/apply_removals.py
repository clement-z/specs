#!/usr/bin/env python3
# ws-cleanup C-08: usage: apply_removals.py OLD_RECORDING NEW_RECORDING BASELINE CASE...
# Rebuild baseline stdout/stderr as "old baseline minus exactly the lines C-08 removed" (multiset from the proof recordings).
import sys
from collections import Counter
from pathlib import Path
old_rec, new_rec, baseline = map(Path, sys.argv[1:4])
for case in sys.argv[4:]:
    for stream in ('stdout.txt', 'stderr.txt'):
        o = (old_rec / 'cases' / case / stream).read_text().splitlines()
        n = (new_rec / 'cases' / case / stream).read_text().splitlines()
        gone = Counter(o) - Counter(n)
        assert not (Counter(n) - Counter(o)), case
        if not gone:
            continue
        b = baseline / 'cases' / case / stream
        text = b.read_text()
        out = []
        for line in text.splitlines():
            if gone[line] > 0:
                gone[line] -= 1
            else:
                out.append(line)
        assert not +gone, (case, stream, +gone)
        b.write_text('\n'.join(out) + ('\n' if text.endswith('\n') else ''))
        print(case, stream, len(text.splitlines()) - len(out), 'lines removed')
