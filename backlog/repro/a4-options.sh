#!/bin/bash
# .options values that are not validated (UB cast of negative/huge resolution, out-of-range
# timescale, non-positive tolerances). Release: silently accepted or SystemC E705; Debug: abort (134).
# usage: a4-options.sh <path/to/specs>
SPECS=${1:?usage: $0 <specs>}
D=$(mktemp -d); mkdir -p $D/traces; cd $D || exit 1
for v in resolution=-1 resolution=0.5 resolution=1e30 timescale=3 timescale=-20 timescale=-12.7 abstol=-1 reltol=0; do
  printf "* .options $v\ncwsrc1 a wl=1.55e-6 power=1e-3\nwg1 a b length=1e-4\nprobe1 b\n.options $v\n.tran 1e-11\n" > o.cir
  timeout 60 "$SPECS" -f o.cir -o traces/x > o.log 2>&1; rc=$?
  printf "%-18s rc=%-4s %s | %s\n" "$v" "$rc" "$(grep -h "resolution multiplier\|engine timescale" o.log | tr -s " " | tr "\n" ";")" "$(grep -m1 -i 'E705\|assert\|Simulated' o.log)"
done
