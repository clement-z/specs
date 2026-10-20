#!/usr/bin/env bash
# A6 repro: statements of doc/resources/cheatsheet.pdf that SPECS rejects.
# usage: a6-cheatsheet-examples.sh   (env SPECS=path to binary). Each case prints the first error line, or OK.
SPECS=${SPECS:-/tmp/ws-review-build/specs}; W=$(mktemp -d); mkdir -p "$W/traces"; cd "$W"
t() { printf '* t\ncwsrc1 1 power=1 wl=1.55e-6\n%s\n' "$2" > "$1.cir"; r=$(timeout 30 "$SPECS" -n -f "$1.cir" -o "traces/$1" 2>&1 | grep -E "Parsing error|offending|Unknown keyword" | head -2 | tr '\n' ' '); echo "$1: ${r:-OK}"; }
t dc_wl_fn     $'probe1 1\n.dc WL(cwsrc1) 1.5e-6 1.6e-6 1e-9'
t dc_slash     $'probe1 1\n.dc /cwsrc1/WL 1.5e-6 1.6e-6 1e-9'
t pshift_comma $'evlsrc1 3 values=[[0,1]]\npshift1 1 2 3 att=0, gain=1'
t pshift_ok    $'evlsrc1 3 values=[[0,1]]\npshift1 1 2 3 att=0 gain=1'
t merger_loss  $'cwsrc2 2 power=1 wl=1.55e-6\nmerger1 1 2 3 loss=0'
t pd_prefix    'pd1 1 2 r=1 ts=1e-12'
t pdet_prefix  'pdet1 1 2 r=1 ts=1e-12'
t plus_indent  $'probe1 1\n.options timescale=-12 reltol=1e-4\n  + abstol=1e-8 traceall=1\n.tran 1e-9'
t plus_col0    $'probe1 1\n.options timescale=-12 reltol=1e-4\n+ abstol=1e-8 traceall=1\n.tran 1e-9'
