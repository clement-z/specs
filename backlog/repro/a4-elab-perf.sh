#!/bin/bash
# Elaboration time of a chain of N waveguides (dry run): super-linear in N.
# usage: a4-elab-perf.sh <path/to/specs> [N...]   (default N = 1000 4000 8000)
SPECS=${1:?usage: $0 <specs> [N...]}; shift; Ns=${@:-1000 4000 8000}
D=$(mktemp -d); mkdir -p $D/traces; cd $D || exit 1
for n in $Ns; do
  { echo "* chain of $n waveguides"; echo "cwsrc1 n0 wl=1.55e-6 power=1e-3"
    for i in $(seq 1 $n); do echo "wg$i n$((i-1)) n$i length=1e-5"; done
    echo "probe1 n$n"; echo ".tran 1e-12"; } > chain.cir
  s=$(date +%s.%N); timeout 600 "$SPECS" -n -f chain.cir -o traces/x > /dev/null 2>&1; rc=$?
  echo "N=$n rc=$rc $(echo "$(date +%s.%N) - $s" | bc) s"
done
