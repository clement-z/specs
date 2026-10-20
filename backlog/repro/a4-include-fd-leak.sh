#!/bin/bash
# .include never fclose()s the included file: with the default 1024 fd limit, the
# 1021st include fails with a misleading "included file not found".
# usage: a4-include-fd-leak.sh <path/to/specs> <path/to/repo>
SPECS=${1:?usage: $0 <specs> <repo>}; REPO=${2:?usage: $0 <specs> <repo>}
D=$(mktemp -d); mkdir -p $D/traces; cp -r "$REPO/circuits" $D/; cd $D || exit 1
{ echo "* 1100 top-level includes of the same file"
  for i in $(seq 1 1100); do echo ".include circuits/common_definitions.def"; done
  echo "cwsrc1 a wl=1.55e-6 power=1e-3"; echo "probe1 a"; echo ".tran 1e-12"; } > many.cir
(ulimit -n 1024; timeout 300 "$SPECS" -n -f many.cir -o traces/x > many.log 2>&1; echo "rc=$?")
grep -m1 -i error many.log; echo "includes done: $(grep -c Including many.log)"
