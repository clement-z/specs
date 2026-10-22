#!/usr/bin/env bash
# A6 repro: netlist lines equivalent to the utils/specs.kicad_sym templates fail in SPECS.
# usage: a6-kicad-templates.sh   (env SPECS=path to binary)
SPECS=${SPECS:-/tmp/ws-review-build/specs}; W=$(mktemp -d); mkdir -p "$W/traces"; cd "$W"
t() { printf '* t\n%s\n.tran 1e-9\n' "$2" > "$1.cir"; echo "== $1"; timeout 30 "$SPECS" -n -f "$1.cir" -o "traces/$1" 2>&1 | grep -E "Parsing error|offending|Unknown keyword|Variable passed|Nx2" | head -3; }
t adrop  'adrop1 a b c d neff=1 ng=1 k1=0.5 k2=0.5 radius=10e-6 att=0'   # ADD_DROP_FILTER (Ref ADROP): no lexer rule
t apass  'apass1 a b neff=1 ng=1 k=0.5 radius=10e-6 att=0'                # ALL_PASS_FILTER (Ref APASS): no lexer rule
t pcm    'pcm1 a b levels=63'                                             # PCM (Ref PCM): no lexer rule
t pcmcell 'pcmcell1 a b levels=63'                                        # even as PCMCELL: levels is not a kwarg
t evl3   'evlsrc1 a values=[[0,1,1550e-9],[1e-9,0,1550e-9]]'   # ELECTRIC_V_SOURCE_LIST template shape (Nx3, numbers substituted); Nx2 required
t mzi4   'mzi1 a b c d length=1e-6 length_ref=1e-6 neff=2 ng=2 att=0 il_dc=0 il_phaseshifter=0'  # MZI: Sim.Pins maps 4 of 5 pins
