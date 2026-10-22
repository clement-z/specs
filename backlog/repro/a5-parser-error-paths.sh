#!/bin/sh
# Error-path netlists with the exit status and distinctive message observed on d9eb1e6+ (Release build).
# usage: sh a5-parser-error-paths.sh /path/to/specs        (prints one line per case: name expected-rc observed-rc)
SPECS=${1:?specs binary}
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT; cd "$W"; mkdir traces
w() { printf '%b' "$2" > "$1.cir"; }
w e01_unknown_element       'foo1 a b\n.op\n'                                                                    # rc 1  'offending token: "foo1"'
w e02_unknown_param         'cwsrc1 in power=1 wl=1.55e-6 bogus=3\nprobe1 in\n.op\n'                              # rc 1  'Unknown keyword: bogus'
w e03_undefined_assign      'cwsrc1 in power={nosuch} wl=1.55e-6\nprobe1 in\n.op\n'                              # rc 1  'Variable not found: nosuch'
w e04_missing_ends          '.subckt A 1 2\nwg1 1 2 length=1e-3 neff=1 ng=1\nx1 a b A\n.op\n'                  # rc 1  'Reached end of file before end of subcircuit definition'
w e05_undefined_subckt      'cwsrc1 in power=1 wl=1.55e-6\nx1 in out NOSUCH\n.op\n'                            # rc 1  'Subcircuit definition not found: NOSUCH'
w e06_duplicate_name        'cwsrc1 in power=1 wl=1.55e-6\nwg1 in out length=1e-3 neff=1 ng=1\nwg1 out o2 length=1e-3 neff=1 ng=1\nprobe1 o2\n.op\n'  # rc 1 "a module with name 'ROOT/WG1' already exists"
w e09_two_analyses          'cwsrc1 in power=1 wl=1.55e-6\nprobe1 in\n.op\n.tran 1e-9\n'                       # rc 1  'Attempted to register a new analysis (TRAN)'
w e10_bad_number            'cwsrc1 in power=1.2.3 wl=1.55e-6\nprobe1 in\n.op\n'                               # rc 1  'offending token: ".3"'
w e11_ic_directive          '.ic x=1\n.op\n'                                                                      # rc 1  (lexer exit(1), no message)
w e12_wg_missing_port       'cwsrc1 in power=1 wl=1.55e-6\nwg1 in length=1e-3 neff=1 ng=1\n.op\n'               # rc 1  'offending token: "length"'
w e13_dc_unknown_target     'cwsrc1 in power=1 wl=1.55e-6\nprobe1 in\n.dc /nosuch/WL 1e-6 2e-6 1e-7\n'         # rc 1  'Element not found: ROOT/NOSUCH'
w e14_dc_negative_step      'cwsrc1 in power=1 wl=1.55e-6\nprobe1 in\n.dc /cwsrc1/WL 1.6e-6 1.5e-6 -1e-8\n'    # rc 1  'offending token: "-"'
w e15_tran_negative         'cwsrc1 in power=1 wl=1.55e-6\nprobe1 in\n.tran -1\n'                              # rc 1  'offending token: "-"'
w e16_unterminated_string   'vlsrc1 in values={"abc\n.op\n'                                                       # rc 1  'offending token: """'
# accepted silently today (exit 0) -- decide whether these should be errors:
w s07_no_analysis           'cwsrc1 in power=1 wl=1.55e-6\nprobe1 in\n'                                           # rc 0  prints DONE, simulates nothing
w s08_empty_file            ''                                                                                     # rc 0
w s17_power_div_by_zero     'cwsrc1 in power=1/0 wl=1.55e-6\nprobe1 in\n.op\n'                                    # rc 0  CWSRC emits (inf V/m, -nan W)
w s18_coupler_k_gt_1        'cwsrc1 in power=1 wl=1.55e-6\ncoupler1 in 0 out 0 k=2\nprobe1 out\n.op\n'          # rc 0  power gain accepted
for f in e*.cir s*.cir; do
  b=${f%.cir}; timeout 20 "$SPECS" -f "$f" -o "traces/$b" > "$b.out" 2>&1; rc=$?
  printf '%-26s observed rc=%s  %s\n' "$b" "$rc" "$(grep -v '^ *$' "$b.out" | tail -1 | cut -c1-90)"
done
