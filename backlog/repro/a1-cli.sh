#!/bin/sh
# F-A1 CLI repros. Usage: a1-cli.sh <specs-binary> <repro-dir>   (run from a scratch dir)
S=${1:-specs}; R=${2:-.}
mkdir -p traces
echo "== -o '' (expected: no VCD or an error; observed: SIGSEGV, rc=139)"
$S -o "" -f "$R/a1-tran-no-analysis.cir" >/dev/null 2>&1; echo "rc=$?"
echo "== --json to an unwritable path (observed: 'Exported ...', rc=0, no file)"
$S -n --json /nonexistent/x.json -f "$R/a1-tran-no-analysis.cir" > json.out 2>&1; echo "rc=$?"; grep -i json json.out
echo "== -R -16 (observed: accepted, then 'E705 invalid tracing timescale unit', rc=1)"
$S -R -16 -f "$R/a1-tran-no-analysis.cir" -o traces/r 2>&1 | grep -E "E705|Invalid"
echo "== --reltol prints its value on stdout before validation (observed: '1e-05' as first line)"
$S --reltol 1e-5 -n -f "$R/a1-tran-no-analysis.cir" 2>/dev/null | grep -x '1e-05'
echo "== -T ignores -f and exits 0 without doing anything"
$S -T -f "$R/a1-tran-no-analysis.cir" > t.out 2>&1; echo "rc=$?"; echo "SIMULATION banners: $(grep -c SIMULATION t.out)"
echo "== -o x.vcd writes x.vcd.vcd"
$S -o traces/y.vcd -f "$R/a1-tran-no-analysis.cir" >/dev/null 2>&1; ls traces | grep '^y'
echo "== -m td is overridden by .op ('simulation mode: Event-driven frequency-domain')"
$S -m td -f "$R/a1-op-no-wavelength.cir" -o traces/m 2>&1 | grep -E "mode"
echo "== -t detector prints Failure! but exits 0"
$S -t detector 2>&1 | grep -c "Failure!"; $S -t detector >/dev/null 2>&1; echo "rc=$?"
