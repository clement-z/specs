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

echo "== B-128: one digit of a tolerance printout changed in the baseline (cir/add_drop: '- abstol: 1e-12')"
fresh; sed -i 's/^- abstol: 1e-12$/- abstol: 2e-12/' "$W/base/cases/cir/add_drop/stdout.txt"
grep -q '^- abstol: 2e-12$' "$W/base/cases/cir/add_drop/stdout.txt" || echo "UNEXPECTED: perturbation not applied"
expect 1 "check (default print slack)" "${G[@]}" check "${S[@]}" --no-unit-tests --baseline "$W/base" --only '^cir/add_drop$'
fresh; sed -i 's/^- default reltol: 1e-06$/- default reltol: 2e-06/' "$W/base/cases/cir/add_drop/stdout.txt"
expect 1 "check, reltol 1e-06 -> 2e-06" "${G[@]}" check "${S[@]}" --no-unit-tests --baseline "$W/base" --only '^cir/add_drop$'

echo "== B-128 control: last digit of a long (5+ significant digits) computed token changed (within the print slack)"
fresh
python3 -I - "$W/base/cases/tb/wg/stdout.txt" <<'PY'
import re, sys
p = sys.argv[1]; s = open(p).read()
m = re.search(r"(?<![\d.])(\d\.\d{5})(\d)(?= )", s)   # first token with 6 decimals, such as 0.707107
assert m, "no 6-digit token"
d = int(m.group(2)); s = s[:m.start(2)] + str(d + 1 if d < 9 else d - 1) + s[m.end(2):]
open(p, "w").write(s)
print("           | changed %s%s by one unit" % (m.group(1), m.group(2)))
PY
expect 0 "check (default print slack)" "${G[@]}" check "${S[@]}" --no-unit-tests --baseline "$W/base" --only '^tb/wg$'
expect 1 "check --print-slack 0"       "${G[@]}" check "${S[@]}" --no-unit-tests --baseline "$W/base" --only '^tb/wg$' --print-slack 0
