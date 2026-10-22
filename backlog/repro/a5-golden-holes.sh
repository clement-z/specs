#!/bin/sh
# Demonstrates three ways tests/golden/run_golden.py can pass although the output regressed.
# usage: sh a5-golden-holes.sh <repo-root>      (works on a scratch copy of the baseline, repo untouched)
REPO=${1:?repo root}
G="python3 -I $REPO/tests/golden/run_golden.py"
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
cp -r "$REPO/tests/golden/baseline" "$W/base"; cp -r "$W/base" "$W/cand"

echo "== 1. chronological event blocks of tb/wg swapped (time goes backwards): default mode"
python3 -I - "$W/cand/cases/tb/wg/stdout.txt" <<'PY'
import sys
p = sys.argv[1]; L = open(p).read().split('\n')
idx = [i for i, l in enumerate(L) if l.endswith((' s:', ' ms:', ' ps:'))]
a, b, c = idx[0], idx[1], idx[2]
L[a:c] = L[b:c] + L[a:b]
open(p, 'w').write('\n'.join(L))
PY
set +e
$G compare --baseline "$W/base" --candidate "$W/cand" --only 'tb/wg$' > "$W/out.txt"; rc=$?; head -1 "$W/out.txt"
echo "exit=$rc (expected: nonzero; observed: 0, PASS with a 'reordered' note)"
$G compare --baseline "$W/base" --candidate "$W/cand" --only 'tb/wg$' --strict-order >/dev/null && echo "strict-order: pass" || echo "--strict-order: fails (but CLAUDE.md's check command does not pass it)"

echo "== 2. a baseline case directory deleted: NEW is not fatal without --strict"
rm -rf "$W/base/cases/tb/wg"
$G compare --baseline "$W/base" --candidate "$W/cand" --only 'tb/wg$' >/dev/null; echo "exit=$? (expected: nonzero; observed: 0, 'NEW tb/wg (not in baseline)')"

echo "== 3. --only matching nothing"
$G compare --baseline "$W/base" --candidate "$W/cand" --only 'no_such_case' >/dev/null; echo "exit=$? (expected: nonzero; observed: 0, '0 passed')"
