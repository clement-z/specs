#!/usr/bin/env bash
# A6 repro driver for the pyspecs findings. Usage: a6-pyspecs-repro.sh <repo-root> <python-with-pandas+pyDigitalWaveTools>
# Runs specs on the three a6-pyspecs-*.cir netlists + circuits/add_drop.cir (a .dc sweep) in a scratch dir and
# parses the VCDs with pyspecs. Expected on the unfixed tree:
#   add_drop.vcd            -> "Type of simulation: TD" (wrong: analysis_type is b1 = CW sweep)
#   unpowered-first.vcd     -> TD as well (wrong); with only the b01->b1 fix: 59 rows instead of 30
#   wl-first.vcd            -> UnboundLocalError current_signal_df (specsparse.py:82)
#   nowl-fd.vcd             -> (with b01->b1 fixed) KeyError 'wavelength' (specsparse.py:92)
set -e
REPO=${1:?repo root}; PY=${2:-python3}; SPECS=${SPECS:-/tmp/ws-review-build/specs}
HERE=$(cd "$(dirname "$0")" && pwd); W=$(mktemp -d); mkdir -p "$W/traces"; cp -r "${CIRCUITS:-$REPO/circuits}" "$W/circuits"; cd "$W"
for n in unpowered-first wl-first nowl-fd; do cp "$HERE/a6-pyspecs-$n.cir" "$n.cir"; timeout 120 "$SPECS" -f "$n.cir" -o "traces/$n" >/dev/null 2>&1; done
timeout 120 "$SPECS" -f circuits/add_drop.cir -o traces/add_drop >/dev/null 2>&1
"$PY" -I "$HERE/a6-pyspecs-parse-vcds.py" "$REPO" traces/add_drop.vcd traces/unpowered-first.vcd traces/wl-first.vcd traces/nowl-fd.vcd
