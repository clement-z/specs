#!/bin/bash
# F-A7-01: the golden baseline is bit-exact only for a -march=native (FMA) build. A build without -march=native
# passes with the default tolerances (rtol 1e-9) but 12/44 cases differ in the last digit at rtol=atol=0.
# usage: a7-golden-native-vs-generic.sh <repo> <path-to-specs-built-WITHOUT-march=native>
# (build one with:  sed -i '/-march=native/d' CMakeLists.txt  in a scratch copy of CMakeLists.txt; cmake -DCMAKE_BUILD_TYPE=Release)
REPO=${1:?repo}; SPECS=${2:?specs binary}; cd "$REPO" || exit 1
python3 -I tests/golden/run_golden.py check --specs "$SPECS" --baseline tests/golden/baseline -j 2 --atol 0 | tail -1
python3 -I tests/golden/run_golden.py check --specs "$SPECS" --baseline tests/golden/baseline -j 2 --atol 0 --rtol 0 | grep -c '^FAIL'
objdump -d "$SPECS" | grep -c vfmadd   # 0 for the generic build, ~123 for the native one
