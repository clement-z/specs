#!/bin/bash
# Netlist inputs that hit an assert() on user input (rc 134 in Debug builds; silently
# accepted, mis-handled or crashing differently under NDEBUG/Release).
# usage: a4-asserts.sh <path/to/specs>      (run from any directory)
SPECS=${1:?usage: $0 <specs>}
D=$(mktemp -d); mkdir -p $D/traces; cd $D || exit 1
CW='cwsrc1 a wl=1.55e-6 power=1e-3'
cases=(
  "tran2|$CW\nprobe1 a\n.tran 1e-9 2e-9"
  "dc_bare|$CW\nprobe1 a\n.dc"
  "options_pos|$CW\nprobe1 a\n.options 5\n.tran 1e-11"
  "pshift|$CW\nevlsrc1 e values=[[0,1]]\npshift1 a b e 1 2 3\nprobe1 b\n.tran 1e-11"
  "mzi|$CW\nevlsrc1 e values=[[0,1]]\nmzi1 a c b d e 1 2\nprobe1 b\n.tran 1e-11"
  "crossing|$CW\ncrossing1 a c b d 1 2 3\nprobe1 b\n.tran 1e-11"
  "cwsrc|cwsrc1 a 7 1.3e-6 0.5 1\nprobe1 a\n.tran 1e-11"
  "vlsrc|vlsrc1 a \"f1\" \"f2\"\nprobe1 a\n.tran 1e-11"
  "evlsrc|evlsrc1 e \"f1\" \"f2\"\npdet1 a e\n$CW\n.tran 1e-11"
  "probe|$CW\nprobe1 a 1 1 1 1 1\n.tran 1e-11"
  "mlprobe|$CW\nmlprobe1 a [1.55e-6] [1.3e-6]\n.tran 1e-11"
  "pwr|$CW\npwr1 a 1\n.tran 1e-11"
  "pcmcell|$CW\npcmcell1 a b 1e-9 10 0.1 0.2\nprobe1 b\n.tran 1e-11"
  "pdet|$CW\npdet1 a e 1e-12 1 3\n.tran 1e-11"
)
for c in "${cases[@]}"; do
  name=${c%%|*}; printf "* assert case $name\n${c#*|}\n" > $name.cir
  timeout 60 "$SPECS" -f $name.cir -o traces/x > $name.log 2>&1; rc=$?
  printf "%-12s rc=%-4s %s\n" "$name" "$rc" "$(grep -m1 -i 'assert\|error\|too many\|Simulated' $name.log)"
done
