#!/bin/bash
# Sprint 2026-10b P1: each failure mode the golden harness gained (B-126, B-127, B-128), shown by a perturbation.
# usage: bash p1-golden-perturbations.sh <repo-root> <builddir>
# Works on a scratch copy of the baseline; the repo is untouched. Prints one line per check:
# "ok" when the exit status is the expected one.
REPO=${1:?repo root}
B=${2:?build dir with specs and src/test/unit_tests}
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
G=(python3 -I "$REPO/tests/golden/run_golden.py")
S=(--specs "$B/specs")
fresh() { rm -rf "$W/base"; cp -r "$REPO/tests/golden/baseline" "$W/base"; }
expect() {  # expect <wanted rc> <label> <command...>
    local want=$1 label=$2; shift 2
    "$@" > "$W/out.txt" 2>&1; local rc=$?
    local verdict=ok; [ "$rc" = "$want" ] || verdict="UNEXPECTED"
    printf '%-10s rc=%s (want %s)  %s\n' "$verdict" "$rc" "$want" "$label"
    grep -E '^(FAIL|NEW|MISSING|SKIPPED)|^error:|ONLY THE LINE ORDER|^ +stdout:[0-9]|^ +- |^ +\+ ' "$W/out.txt" | head -n 4 | sed 's/^/           | /'
}

echo "== B-126: two chronological event blocks of tb/wg swapped in the baseline"
fresh
python3 -I - "$W/base/cases/tb/wg/stdout.txt" <<'PY'
import sys
p = sys.argv[1]; L = open(p).read().split('\n')
idx = [i for i, l in enumerate(L) if l.endswith((' s:', ' ms:', ' ps:'))]
a, b, c = idx[0], idx[1], idx[2]
L[a:c] = L[b:c] + L[a:b]
open(p, 'w').write('\n'.join(L))
PY
expect 1 "check (default)"         "${G[@]}" check "${S[@]}" --no-unit-tests --baseline "$W/base" --only '^tb/wg$'
expect 0 "check --lenient-order"   "${G[@]}" check "${S[@]}" --no-unit-tests --baseline "$W/base" --only '^tb/wg$' --lenient-order

echo "== B-127: baseline directory of tb/wg deleted (NEW)"
fresh; rm -rf "$W/base/cases/tb/wg"
expect 1 "check (default)"         "${G[@]}" check "${S[@]}" --no-unit-tests --baseline "$W/base" --only '^tb/wg$'
expect 0 "check --allow-new"       "${G[@]}" check "${S[@]}" --no-unit-tests --baseline "$W/base" --only '^tb/wg$' --allow-new

echo "== B-127: gtest selected but not run (no --unit-tests)"
fresh
expect 1 "check without --unit-tests"  "${G[@]}" check "${S[@]}" --baseline "$W/base" --only '^(tb/wg|gtest)'
expect 0 "check --no-unit-tests"       "${G[@]}" check "${S[@]}" --baseline "$W/base" --only '^(tb/wg|gtest)' --no-unit-tests
expect 0 "check --unit-tests"          "${G[@]}" check "${S[@]}" --unit-tests "$B/src/test/unit_tests" --baseline "$W/base" --only '^(tb/wg|gtest)'

echo "== B-127: gtest case missing from the candidate (compare)"
fresh; mkdir -p "$W/cand"; cp -r "$W/base/cases" "$W/base/manifest.json" "$W/cand/"
expect 1 "compare, candidate without gtest/"  "${G[@]}" compare --baseline "$W/base" --candidate "$W/cand"

echo "== B-127: --only matching nothing"
expect 2 "check --only no_such_case"   "${G[@]}" check "${S[@]}" --no-unit-tests --baseline "$W/base" --only no_such_case
expect 2 "compare --only no_such_case" "${G[@]}" compare --baseline "$W/base" --candidate "$W/base" --only no_such_case
